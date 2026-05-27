#include "GameLogic.h"
#include "utils/Log.h"
#include "resources/ResourceData.h"
#include "CMenuManager.h"
#include "CRadar.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CStreaming.h"
#include "CVehicle.h"
#include "CAutomobile.h"
#include "CBike.h"
#include "CTrain.h"
#include "CPed.h"
#include "CPools.h"
#include "CModelInfo.h"
#include "CWeaponInfo.h"
#include "CPickups.h"
#include "extensions/ScriptCommands.h"
#include "Patch.h"
#include "CTimer.h"
#include "CClock.h"
#include "CHud.h"
#include "CWeather.h"
#include "CCutsceneMgr.h"
#include "CTheScripts.h"
#include "rw/skeleton.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <windows.h>
#include <ctime>
#include <string>

namespace {
template <std::size_t Size>
bool PatchBytesIfExpected(const char* label, std::uintptr_t address, const std::array<unsigned char, Size>& original, const std::array<unsigned char, Size>& patched, bool enable) {
    auto* target = reinterpret_cast<unsigned char*>(address);

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
        (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
        reinterpret_cast<std::uintptr_t>(target) + Size > reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize) {
        Log::Warn(std::string(label) + " 回放补丁地址不可用，已跳过");
        return false;
    }

    const auto& desired = enable ? patched : original;
    if (std::memcmp(target, desired.data(), Size) == 0) {
        return true;
    }

    const auto& acceptableCurrent = enable ? original : patched;
    if (std::memcmp(target, acceptableCurrent.data(), Size) != 0) {
        Log::Warn(std::string(label) + " 回放补丁位置与预期不符，已跳过");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, Size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log::Warn(std::string(label) + " 回放补丁无法修改内存权限，已跳过");
        return false;
    }

    std::memcpy(target, desired.data(), Size);
    FlushInstructionCache(GetCurrentProcess(), target, Size);

    DWORD unusedProtect = 0;
    VirtualProtect(target, Size, oldProtect, &unusedProtect);
    return true;
}

    bool IsSpawnPositionClear(const CVector& pos) {
        for (CVehicle* vehicle : CPools::ms_pVehiclePool) {
            if (!vehicle) {
                continue;
            }

            const CVector other = vehicle->GetPosition();
            const float dx = other.x - pos.x;
            const float dy = other.y - pos.y;
            const float dz = other.z - pos.z;
            if (dx * dx + dy * dy + dz * dz < 64.0f) {
                return false;
            }
        }

        return !plugin::Command<plugin::Commands::IS_POINT_OBSCURED_BY_A_MISSION_ENTITY>(
            pos.x - 3.0f, pos.y - 3.0f, pos.z - 1.0f,
            pos.x + 3.0f, pos.y + 3.0f, pos.z + 3.0f
        );
    }

    CVector FindSideSpawnPosition(CPlayerPed* player, const CVector& origin) {
        if (!player) {
            return origin;
        }

        const float heading = player->GetHeading() * 0.01745329252f;
        const float forwardX = std::sin(heading);
        const float forwardY = std::cos(heading);
        const float rightX = forwardY;
        const float rightY = -forwardX;

        const float offsets[][2] = {
            { 8.0f, 2.0f },
            { -8.0f, 2.0f },
            { 10.0f, -2.0f },
            { -10.0f, -2.0f },
            { 0.0f, 12.0f },
            { 0.0f, -12.0f },
            { 14.0f, 0.0f },
            { -14.0f, 0.0f }
        };

        for (const auto& offset : offsets) {
            CVector candidate(
                origin.x + rightX * offset[0] + forwardX * offset[1],
                origin.y + rightY * offset[0] + forwardY * offset[1],
                origin.z + 3.0f
            );

            float groundZ = candidate.z;
            if (plugin::Command<plugin::Commands::GET_GROUND_Z_FOR_3D_COORD>(candidate.x, candidate.y, candidate.z + 20.0f, &groundZ)) {
                candidate.z = groundZ + 1.0f;
            }

            if (IsSpawnPositionClear(candidate)) {
                return candidate;
            }
        }

        return CVector(origin.x + rightX * 14.0f, origin.y + rightY * 14.0f, origin.z + 1.0f);
    }
}

namespace GameLogic {

void Init() {
}

void Process() {
}

ProofState GetPlayerProofState(CPlayerPed* player) {
    ProofState state;
    if (!player) return state;
    state.bullet = player->bBulletProof;
    state.collision = player->bCollisionProof;
    state.explosion = player->bExplosionProof;
    state.fire = player->bFireProof;
    state.melee = player->bMeleeProof;
    return state;
}

void SetPlayerProofState(CPlayerPed* player, const ProofState& state) {
    if (!player) return;
    player->bBulletProof = state.bullet;
    player->bCollisionProof = state.collision;
    player->bExplosionProof = state.explosion;
    player->bFireProof = state.fire;
    player->bMeleeProof = state.melee;
}

void ApplyGodMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    plugin::patch::Set<bool>(0x96916D, enable, false);
    player->bBulletProof = enable;
    player->bCollisionProof = enable;
    player->bExplosionProof = enable;
    player->bFireProof = enable;
    player->bMeleeProof = enable;
}

void SetInfiniteSprint(bool enable) {
    bool* infSprintAddr = reinterpret_cast<bool*>(0xB7CEE4);
    *infSprintAddr = enable;
}

void SetFreeHealthcare(bool enable) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_bGetOutOfHospitalFree = enable;
}

bool GetFreeHealthcare() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_bGetOutOfHospitalFree;
}

void SetFreeJail(bool enable) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_bGetOutOfJailFree = enable;
}

bool GetFreeJail() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_bGetOutOfJailFree;
}

void GiveMoney(int amount) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_nMoney += amount;
}

int GetMoney() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_nMoney;
}

void SetMoney(int amount) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_nMoney = amount;
}

float GetHealth(CPlayerPed* player) {
    return player ? player->m_fHealth : 0.0f;
}

void SetHealth(CPlayerPed* player, float value) {
    if (player) player->m_fHealth = value;
}

float GetArmour(CPlayerPed* player) {
    return player ? player->m_fArmour : 0.0f;
}

void SetArmour(CPlayerPed* player, float value) {
    if (player) player->m_fArmour = value;
}

CVector GetPlayerPosition(CPlayerPed* player) {
    return player ? player->GetPosition() : CVector(0.0f, 0.0f, 0.0f);
}

void MovePlayerRelative(CPlayerPed* player, float forward, float right, float up) {
    if (!player) {
        return;
    }

    CVector pos = player->GetPosition();
    const float angle = player->m_fHeadingCurrent;
    const float forwardX = -std::sin(angle);
    const float forwardY = std::cos(angle);
    const float rightX = forwardY;
    const float rightY = -forwardX;
    pos.x += forwardX * forward + rightX * right;
    pos.y += forwardY * forward + rightY * right;
    pos.z += up;
    TeleportPlayer(pos);
}

bool IsPlayerDead(CPlayerPed* player) {
    return player ? player->m_fHealth <= 0.0f : false;
}

void SetKeepStuff(bool enable) {
    plugin::Command<plugin::Commands::SWITCH_ARREST_PENALTIES>(enable);
    plugin::Command<plugin::Commands::SWITCH_DEATH_PENALTIES>(enable);
}

void HealPlayer(CPlayerPed* player) {
    if (player) player->m_fHealth = 100.0f;
}

void GiveArmour(CPlayerPed* player) {
    if (player) player->m_fArmour = 100.0f;
}

void ProcessAutoHeal(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    static unsigned int lastTimer = 0;
    unsigned int timer = CTimer::m_snTimeInMilliseconds;
    if (timer - lastTimer > 1000) {
        if (player->m_fHealth < player->m_fMaxHealth) {
            player->m_fHealth += 2.0f;
            if (player->m_fHealth > player->m_fMaxHealth) player->m_fHealth = player->m_fMaxHealth;
        } else if (player->m_fArmour < 100.0f && player->m_fArmour > 0.0f) {
            player->m_fArmour += 2.0f;
            if (player->m_fArmour > 100.0f) player->m_fArmour = 100.0f;
        }
        lastTimer = timer;
    }
}

void SetWantedLevel(CPlayerPed* player, int level) {
    if (player) player->SetWantedLevel(level);
}

int GetWantedLevel(CPlayerPed* player) {
    return player ? player->GetWantedLevel() : 0;
}

void ProcessHardMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    if (player->m_fHealth > 50.0f) player->m_fHealth = 50.0f;
    player->m_fArmour = 0.0f;
}

void ProcessRespawnAtDeathPosition(CPlayerPed* player, bool enable) {
    static CVector deathPos(0.0f, 0.0f, 0.0f);
    static bool hasDeathPos = false;
    if (!enable || !player) {
        hasDeathPos = false;
        return;
    }

    if (IsPlayerDead(player)) {
        deathPos = player->GetPosition();
        hasDeathPos = true;
        return;
    }

    const CVector current = player->GetPosition();
    if (hasDeathPos && deathPos.x != 0.0f && deathPos.y != 0.0f && (deathPos.x != current.x || deathPos.y != current.y)) {
        player->Teleport(deathPos, false);
        hasDeathPos = false;
    }
}

void ProcessFreezeWantedLevel(CPlayerPed* player, bool enable, int level) {
    if (!enable || !player) return;
    SetWantedLevel(player, level);
}

void SetManualPlayerProof(CPlayerPed* player, const ProofState& state) {
    SetPlayerProofState(player, state);
}

bool RequestSaveGame() {
    plugin::Command<plugin::Commands::ACTIVATE_SAVE_MENU>();
    Log::Info("已请求打开存档菜单");
    return true;
}

ProofState GetVehicleProofState(CVehicle* vehicle) {
    ProofState state;
    if (!vehicle) return state;
    state.bullet = vehicle->bBulletProof;
    state.collision = vehicle->bCollisionProof;
    state.explosion = vehicle->bExplosionProof;
    state.fire = vehicle->bFireProof;
    state.melee = vehicle->bMeleeProof;
    return state;
}

void SetVehicleProofState(CVehicle* vehicle, const ProofState& state) {
    if (!vehicle) return;
    vehicle->bBulletProof = state.bullet;
    vehicle->bCollisionProof = state.collision;
    vehicle->bExplosionProof = state.explosion;
    vehicle->bFireProof = state.fire;
    vehicle->bMeleeProof = state.melee;
}

void RepairVehicle(CVehicle* vehicle) {
    if (!vehicle) return;
    vehicle->Fix();
    vehicle->m_fHealth = 1000.0f;
}

void StopVehicle(CVehicle* vehicle) {
    if (!vehicle) return;
    vehicle->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
    vehicle->m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);
}

void UnflipVehicle(CVehicle* vehicle) {
    if (!vehicle || !vehicle->IsUpsideDown()) return;
    int handle = CPools::GetVehicleRef(vehicle);
    float roll = 0.0f;
    plugin::Command<plugin::Commands::GET_CAR_ROLL>(handle, &roll);
    roll += 180.0f;
    plugin::Command<plugin::Commands::SET_CAR_ROLL>(handle, roll);
    plugin::Command<plugin::Commands::SET_CAR_ROLL>(handle, roll);
    StopVehicle(vehicle);
}

void SetVehicleForwardSpeed(CVehicle* vehicle, float speed) {
    if (!vehicle) return;
    const CVector forward = vehicle->GetForward();
    const float velocity = speed / 50.0f;
    vehicle->m_vecMoveSpeed = CVector(forward.x * velocity, forward.y * velocity, forward.z * velocity);
}

void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed) {
    if (!enable || !vehicle) return;
    SetVehicleForwardSpeed(vehicle, speed);
}

void SetVehicleEngine(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bEngineBroken = !enable;
    vehicle->bEngineOn = enable;
}

void SetVehicleInvincible(CVehicle* vehicle, bool enable) {
    if (!enable || !vehicle) return;
    vehicle->bBulletProof = enable;
    vehicle->bExplosionProof = enable;
    vehicle->bFireProof = enable;
    vehicle->bCollisionProof = enable;
    vehicle->bMeleeProof = enable;
}

void SetVehicleHeavy(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::SET_CAR_HEAVY>(CPools::GetVehicleRef(vehicle), enable);
}

void SetVehicleWatertight(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::SET_CAR_WATERTIGHT>(CPools::GetVehicleRef(vehicle), enable);
}

float GetVehicleHealth(CVehicle* vehicle) {
    return vehicle ? vehicle->m_fHealth : 0.0f;
}

void SetVehicleHealth(CVehicle* vehicle, float health) {
    if (!vehicle) return;
    vehicle->m_fHealth = health;
}

bool GetVehicleLights(CVehicle* vehicle) {
    return vehicle ? vehicle->bLightsOn : false;
}

void SetVehicleLights(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bLightsOn = enable;
    vehicle->m_nOverrideLights = enable ? 2 : 1;
}

bool GetVehicleLocked(CVehicle* vehicle) {
    return vehicle ? vehicle->m_eDoorLock == DOORLOCK_LOCKED_PLAYER_INSIDE : false;
}

void SetVehicleLocked(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->m_eDoorLock = enable ? DOORLOCK_LOCKED_PLAYER_INSIDE : DOORLOCK_UNLOCKED;
}

bool GetVehicleVisible(CVehicle* vehicle) {
    return vehicle ? vehicle->bIsVisible : false;
}

void SetVehicleVisible(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bIsVisible = enable;
}

bool GetVehicleAlwaysSkidMarks(CVehicle* vehicle) {
    return vehicle ? vehicle->bAlwaysSkidMarks : false;
}

void SetVehicleAlwaysSkidMarks(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bAlwaysSkidMarks = enable;
}

bool GetVehicleDisableParticles(CVehicle* vehicle) {
    return vehicle ? vehicle->bDisableParticles : false;
}

void SetVehicleDisableParticles(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bDisableParticles = enable;
}

bool GetVehicleDriverTargetable(CVehicle* vehicle) {
    return vehicle ? vehicle->bVehicleCanBeTargetted : false;
}

void SetVehicleDriverTargetable(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bVehicleCanBeTargetted = enable;
}

bool GetVehicleHeatSeekingTargetable(CVehicle* vehicle) {
    return vehicle ? vehicle->bVehicleCanBeTargettedByHS : false;
}

void SetVehicleHeatSeekingTargetable(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bVehicleCanBeTargettedByHS = enable;
}

bool GetVehiclePetrolTankWeakPoint(CVehicle* vehicle) {
    return vehicle ? vehicle->bPetrolTankIsWeakPoint : false;
}

void SetVehiclePetrolTankWeakPoint(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bPetrolTankIsWeakPoint = enable;
}

bool GetVehicleSirenOrAlarm(CVehicle* vehicle) {
    return vehicle ? vehicle->bSirenOrAlarm : false;
}

void SetVehicleSirenOrAlarm(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bSirenOrAlarm = enable;
}

bool GetVehicleTakeLessDamage(CVehicle* vehicle) {
    return vehicle ? vehicle->bTakeLessDamage : false;
}

void SetVehicleTakeLessDamage(CVehicle* vehicle, bool enable) {
    if (!vehicle) return;
    vehicle->bTakeLessDamage = enable;
}

void BlowUpAllVehicles() {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;

    for (CVehicle* vehicle : CPools::ms_pVehiclePool) {
        if (vehicle) {
            vehicle->BlowUpCar(player, false);
        }
    }
}

void DeleteVehicle(CVehicle* vehicle) {
    if (!vehicle) return;

    CPlayerPed* player = FindPlayerPed();
    if (player) {
        const int hplayer = CPools::GetPedRef(player);
        if (plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer) && player->m_pVehicle == vehicle) {
            Log::Warn("SA 载具清理跳过：玩家正在使用目标载具");
            return;
        }
    }

    const int hveh = CPools::GetVehicleRef(vehicle);
    if (hveh == 0) return;

    if (CModelInfo::IsTrainModel(vehicle->m_nModelIndex)) {
        plugin::Command<plugin::Commands::MARK_MISSION_TRAIN_AS_NO_LONGER_NEEDED>(hveh);
        plugin::Command<plugin::Commands::DELETE_MISSION_TRAIN>(hveh);
    } else {
        plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
        plugin::Command<plugin::Commands::DELETE_CAR>(hveh);
    }
}

bool IsAircraftModel(int model) {
    return CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)
        || model == 417 || model == 425 || model == 447 || model == 460 || model == 476
        || model == 487 || model == 488 || model == 511 || model == 512 || model == 513
        || model == 519 || model == 520 || model == 548 || model == 553 || model == 563
        || model == 577 || model == 592 || model == 593;
}

bool IsValidVehicleModel(unsigned int modelId) {
    if (!Resources::IsKnownVehicleModel(modelId)) {
        return false;
    }

    const int model = static_cast<int>(modelId);
    switch (model) {
    case 435: // Articulated trailer
    case 450: // Articulated trailer
    case 569: // Freight flatbed carriage
    case 570: // Streak carriage
    case 584: // Petrol trailer
    case 590: // Freight boxcar
    case 591: // Articulated trailer
    case 606: // Baggage box
    case 607: // Baggage box
    case 608: // Tug stairs
    case 610: // Farm trailer
    case 611: // Utility trailer
        Log::Warn("SA 载具生成被拒绝：模型 ID " + std::to_string(model) + " 不适合通过普通刷车入口生成");
        return false;
    default:
        break;
    }

    return CModelInfo::IsVehicleModel(model) && !CModelInfo::IsTrailerModel(model);
}

int GetTrainConfigForModel(int model) {
    static constexpr int freightConfigs[] = { 8, 9 };
    static constexpr int freightBoxConfigs[] = { 1, 5, 15 };
    static constexpr int streakConfigs[] = { 0, 3, 6, 10, 12, 13 };

    switch (model) {
    case 449:
        return freightConfigs[CTimer::m_snTimeInMilliseconds % (sizeof(freightConfigs) / sizeof(freightConfigs[0]))];
    case 537:
        return streakConfigs[CTimer::m_snTimeInMilliseconds % (sizeof(streakConfigs) / sizeof(streakConfigs[0]))];
    case 538:
        return freightBoxConfigs[CTimer::m_snTimeInMilliseconds % (sizeof(freightBoxConfigs) / sizeof(freightBoxConfigs[0]))];
    default:
        return -1;
    }
}

bool IsValidPedModel(unsigned int modelId) {
    const int model = static_cast<int>(modelId);
    return model >= 0 && model < CModelInfo::ms_modelInfoCount && CModelInfo::IsPedModel(model);
}

bool SetPlayerSkin(unsigned int modelId) {
    if (!IsValidPedModel(modelId)) {
        Log::Warn("SA 玩家皮肤切换被拒绝：无效模型 ID " + std::to_string(modelId));
        return false;
    }

    CPlayerPed* player = FindPlayerPed();
    if (!player) return false;

    const int model = static_cast<int>(modelId);
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);
    plugin::Command<plugin::Commands::SET_PLAYER_MODEL>(0, model);
    plugin::Command<plugin::Commands::BUILD_PLAYER_MODEL>(0);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
    return true;
}

bool ApplyPlayerClothes(int textureId, int modelId, int bodyPart) {
    plugin::Command<plugin::Commands::GIVE_PLAYER_CLOTHES>(0, textureId, modelId, bodyPart);
    plugin::Command<plugin::Commands::BUILD_PLAYER_MODEL>(0);
    return true;
}

bool SetPlayerStat(int statId, float value) {
    plugin::Command<plugin::Commands::SET_FLOAT_STAT>(statId, value);
    return true;
}

CVehicle* SpawnVehicle(unsigned int modelId, const SpawnVehicleOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return nullptr;

    const int model = static_cast<int>(modelId);
    if (!IsValidVehicleModel(modelId)) {
        Log::Warn("SA 载具生成被拒绝：无效模型 ID " + std::to_string(model));
        return nullptr;
    }

    const int hplayer = CPools::GetPedRef(player);
    CVector pos = player->GetPosition();
    float speed = 0.0f;

    if (options.asDriver && options.cleanupPrevious && plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer)) {
        CVehicle* currentVehicle = player->m_pVehicle;
        if (currentVehicle) {
            const int hveh = CPools::GetVehicleRef(currentVehicle);
            pos = currentVehicle->GetPosition();
            plugin::Command<plugin::Commands::GET_CAR_SPEED>(hveh, &speed);
            plugin::Command<plugin::Commands::WARP_CHAR_FROM_CAR_TO_COORD>(hplayer, pos.x, pos.y, pos.z);
            if (CModelInfo::IsTrainModel(currentVehicle->m_nModelIndex)) {
                plugin::Command<plugin::Commands::MARK_MISSION_TRAIN_AS_NO_LONGER_NEEDED>(hveh);
            } else {
                plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
            }
        }
    } else if (!options.cleanupPrevious) {
        pos = FindSideSpawnPosition(player, pos);
    }

    if (player->m_nAreaCode == 0) {
        if (options.aircraftInAir && IsAircraftModel(model)) {
            pos.z = 400.0f;
        } else if (options.cleanupPrevious) {
            pos.z -= 5.0f;
        }
    }

    if (CModelInfo::IsTrainModel(model)) {
        const int trainConfig = GetTrainConfigForModel(model);
        if (trainConfig == -1) {
            Log::Warn("SA 火车生成被拒绝：未知火车型号 " + std::to_string(model));
            return nullptr;
        }

        CStreaming::RequestModel(590, PRIORITY_REQUEST);
        CStreaming::RequestModel(538, PRIORITY_REQUEST);
        CStreaming::RequestModel(570, PRIORITY_REQUEST);
        CStreaming::RequestModel(569, PRIORITY_REQUEST);
        CStreaming::RequestModel(537, PRIORITY_REQUEST);
        CStreaming::RequestModel(449, PRIORITY_REQUEST);
        CStreaming::LoadAllRequestedModels(false);

        int track = CTimer::m_snTimeInMilliseconds % 2;
        const int node = CTrain::FindClosestTrackNode(pos, &track);
        CTrain* train = nullptr;
        CTrain* carriage = nullptr;
        CTrain::CreateMissionTrain(pos, (CTimer::m_snTimeInMilliseconds & 1) != 0, trainConfig, &train, &carriage, node, track, false);

        auto* vehicle = reinterpret_cast<CVehicle*>(train);
        if (!vehicle) {
            Log::Error("SA 火车生成失败：CreateMissionTrain 未返回有效载具，模型 ID " + std::to_string(model));
            return nullptr;
        }

        const int hveh = CPools::GetVehicleRef(vehicle);
        if (vehicle->m_pDriver) {
            plugin::Command<plugin::Commands::DELETE_CHAR>(CPools::GetPedRef(vehicle->m_pDriver));
        }

        if (options.asDriver) {
            plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(hplayer, hveh);
            SetVehicleForwardSpeed(vehicle, speed);
        }

        plugin::Command<plugin::Commands::MARK_MISSION_TRAIN_AS_NO_LONGER_NEEDED>(hveh);
        plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
        CStreaming::SetModelIsDeletable(590);
        CStreaming::SetModelIsDeletable(538);
        CStreaming::SetModelIsDeletable(570);
        CStreaming::SetModelIsDeletable(569);
        CStreaming::SetModelIsDeletable(537);
        CStreaming::SetModelIsDeletable(449);
        vehicle->bHasBeenOwnedByPlayer = true;
        return vehicle;
    }

    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);

    int hveh = 0;
    if (options.asDriver) {
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + (options.cleanupPrevious ? 4.0f : 1.0f), &hveh);
    } else {
        if (options.cleanupPrevious) {
            player->TransformFromObjectSpace(pos, CVector(0.0f, 10.0f, 0.0f));
        }
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + 1.0f, &hveh);
    }

    CVehicle* vehicle = CPools::GetVehicle(hveh);
    if (!vehicle) {
        CStreaming::SetModelIsDeletable(model);
        Log::Error("SA 载具生成失败：脚本命令 CREATE_CAR 未返回有效载具，模型 ID " + std::to_string(model));
        return nullptr;
    }

    vehicle->SetHeading(player->GetHeading() + (options.asDriver ? 0.0f : 55.0f));
    vehicle->m_eDoorLock = DOORLOCK_UNLOCKED;
    vehicle->m_nAreaCode = player->m_nAreaCode;
    vehicle->bHasBeenOwnedByPlayer = true;

    if (options.asDriver) {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(hplayer, hveh);
        SetVehicleForwardSpeed(vehicle, speed);
    }

    plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
    CStreaming::SetModelIsDeletable(model);
    return vehicle;
}

void TeleportPlayer(CVector pos, int interiorID) {
    CPlayerPed* pPlayer = FindPlayerPed();
    if (!pPlayer) return;
    CVehicle* pVeh = pPlayer->m_pVehicle;
    const int hplayer = CPools::GetPedRef(pPlayer);
    const bool jetpack = plugin::Command<plugin::Commands::IS_PLAYER_USING_JETPACK>(0);

    CStreaming::LoadScene(&pos);
    CStreaming::LoadSceneCollision(&pos);
    CStreaming::LoadAllRequestedModels(false);

    if (pVeh && pPlayer->bInVehicle) {
        if (CModelInfo::IsTrainModel(pVeh->m_nModelIndex)) {
            const CVector vehPos = pVeh->GetPosition();
            plugin::Command<plugin::Commands::WARP_CHAR_FROM_CAR_TO_COORD>(hplayer, vehPos.x, vehPos.y, vehPos.z + 2.0f);
            if ((pos - vehPos).Magnitude() > 100.0f) {
                plugin::Command<plugin::Commands::DELETE_ALL_TRAINS>();
            }
            pPlayer->Teleport(pos, false);
        } else {
            pVeh->Teleport(pos, false);
            if (pVeh->m_nVehicleClass == VEHICLE_BIKE) {
                reinterpret_cast<CBike*>(pVeh)->PlaceOnRoadProperly();
            } else if (pVeh->m_nVehicleClass != VEHICLE_BOAT) {
                reinterpret_cast<CAutomobile*>(pVeh)->PlaceOnRoadProperly();
            }
            pVeh->m_nAreaCode = interiorID;
        }
    } else {
        pPlayer->Teleport(pos, false);
    }

    if (jetpack) {
        plugin::Command<plugin::Commands::TASK_JETPACK>(hplayer);
    }

    pPlayer->m_nAreaCode = interiorID;
    plugin::Command<plugin::Commands::SET_AREA_VISIBLE>(interiorID);
}

void TeleportMapPosition(CVector pos, bool spawnUnderwater) {
    CStreaming::LoadScene(&pos);
    CStreaming::LoadSceneCollision(&pos);
    CStreaming::LoadAllRequestedModels(false);

    float water = 0.0f;
    CEntity* playerEntity = FindPlayerEntity(-1);
    const float ground = CWorld::FindGroundZFor3DCoord(pos.x, pos.y, 1000.0f, nullptr, &playerEntity) + 1.0f;
    if (spawnUnderwater) {
        pos.z = ground;
    } else {
        plugin::Command<plugin::Commands::GET_WATER_HEIGHT_AT_COORDS>(pos.x, pos.y, true, &water);
        pos.z = ground > water ? ground : water;
    }
    TeleportPlayer(pos);
}

bool TeleportMarker(bool spawnUnderwater) {
    const auto index = static_cast<unsigned short>(FrontEndMenuManager.m_nTargetBlipIndex);
    const tRadarTrace& targetBlip = CRadar::ms_RadarTrace[index];
    if (targetBlip.m_nRadarSprite != RADAR_SPRITE_WAYPOINT) {
        return false;
    }

    TeleportMapPosition(targetBlip.m_vecPos, spawnUnderwater);
    return true;
}

void ApplyVehicleAppearance(CVehicle* vehicle, const VehicleAppearanceOptions& options) {
    if (!vehicle) return;
    const int hveh = CPools::GetVehicleRef(vehicle);
    plugin::Command<plugin::Commands::CHANGE_CAR_COLOUR>(hveh, options.primaryColor, options.secondaryColor);
    if (options.paintjob >= 0) {
        plugin::Command<plugin::Commands::GIVE_VEHICLE_PAINTJOB>(hveh, options.paintjob);
    }
    if (options.modId > 0) {
        plugin::Command<plugin::Commands::REQUEST_VEHICLE_MOD>(options.modId);
        if (plugin::Command<plugin::Commands::HAS_VEHICLE_MOD_LOADED>(options.modId)) {
            plugin::Command<plugin::Commands::ADD_VEHICLE_MOD>(hveh, options.modId);
        }
    }
}

void OpenVehicleDoor(CVehicle* vehicle, int doorIndex) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::OPEN_CAR_DOOR>(CPools::GetVehicleRef(vehicle), doorIndex);
}

void PopVehicleDoor(CVehicle* vehicle, int doorIndex) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::POP_CAR_DOOR>(CPools::GetVehicleRef(vehicle), doorIndex);
}

void WarpPlayerToVehicleSeat(CVehicle* vehicle, int seatIndex) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !vehicle) return;
    const int hplayer = CPools::GetPedRef(player);
    const int hveh = CPools::GetVehicleRef(vehicle);
    if (seatIndex <= 0) {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(hplayer, hveh);
        return;
    }
    plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR_AS_PASSENGER>(hplayer, hveh, seatIndex - 1);
}

void ProcessAutoDrive(CVehicle* vehicle, bool enable, float speed) {
    CPlayerPed* player = FindPlayerPed();
    if (!enable || !player || !vehicle) return;
    const CVector pos = player->GetPosition();
    plugin::Command<plugin::Commands::TASK_CAR_DRIVE_TO_COORD>(CPools::GetPedRef(player), CPools::GetVehicleRef(vehicle), pos.x + 80.0f, pos.y, pos.z, speed, 0, vehicle->m_nModelIndex, 2, 10.0f);
}

void SetTrafficDensity(float density) {
    plugin::Command<plugin::Commands::SET_CAR_DENSITY_MULTIPLIER>(density);
    plugin::Command<plugin::Commands::SET_PED_DENSITY_MULTIPLIER>(density);
}

void SetFlyingCars(bool enable) {
    Log::Warn(std::string("SA 飞车开关暂未接入安全接口，已跳过：") + (enable ? "开启" : "关闭"));
}

CPed* SpawnPedNearPlayer(const PedSpawnOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !IsValidPedModel(options.modelId)) return nullptr;
    const int model = static_cast<int>(options.modelId);
    CVector pos = player->TransformFromObjectSpace(CVector(0.0f, 3.0f, 0.0f));
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);
    int hped = 0;
    plugin::Command<plugin::Commands::CREATE_CHAR>(options.pedType, model, pos.x, pos.y, pos.z, &hped);
    CPed* ped = CPools::GetPed(hped);
    ApplyPedOptions(ped, options);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
    return ped;
}

CPed* SpawnPedAtMarker(const PedSpawnOptions& options) {
    const auto index = static_cast<unsigned short>(FrontEndMenuManager.m_nTargetBlipIndex);
    const tRadarTrace& targetBlip = CRadar::ms_RadarTrace[index];
    if (targetBlip.m_nRadarSprite != RADAR_SPRITE_WAYPOINT || !IsValidPedModel(options.modelId)) return nullptr;
    const int model = static_cast<int>(options.modelId);
    CVector pos = targetBlip.m_vecPos;
    pos.z = CWorld::FindGroundZFor3DCoord(pos.x, pos.y, 1000.0f, nullptr, nullptr) + 1.0f;
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);
    int hped = 0;
    plugin::Command<plugin::Commands::CREATE_CHAR>(options.pedType, model, pos.x, pos.y, pos.z, &hped);
    CPed* ped = CPools::GetPed(hped);
    ApplyPedOptions(ped, options);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
    return ped;
}

void DeletePed(CPed* ped) {
    if (!ped) return;
    const int hped = CPools::GetPedRef(ped);
    plugin::Command<plugin::Commands::DELETE_CHAR>(hped);
}

void ApplyPedOptions(CPed* ped, const PedSpawnOptions& options) {
    if (!ped) return;
    const int hped = CPools::GetPedRef(ped);
    plugin::Command<plugin::Commands::SET_CHAR_HEALTH>(hped, static_cast<int>(options.health));
    plugin::Command<plugin::Commands::SET_CHAR_ARMOUR>(hped, static_cast<int>(options.armour));
    plugin::Command<plugin::Commands::FREEZE_CHAR_POSITION>(hped, options.freeze);
    plugin::Command<plugin::Commands::SET_CHAR_PROOFS>(hped, false, false, false, false, false);
    if (options.weaponModel > 0) {
        plugin::Command<plugin::Commands::GIVE_WEAPON_TO_CHAR>(hped, options.weaponModel, 9999);
    }
}

bool PlayPlayerAnimation(const char* group, const char* name, bool loop) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !group || !name || group[0] == '\0' || name[0] == '\0') return false;
    const int hplayer = CPools::GetPedRef(player);
    plugin::Command<plugin::Commands::REQUEST_ANIMATION>(group);
    if (!plugin::Command<plugin::Commands::HAS_ANIMATION_LOADED>(group)) {
        CStreaming::LoadAllRequestedModels(false);
    }
    plugin::Command<plugin::Commands::TASK_PLAY_ANIM>(hplayer, name, group, 4.0f, loop, false, false, false, -1);
    return true;
}

void StopPlayerAnimation() {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    plugin::Command<plugin::Commands::CLEAR_CHAR_TASKS>(CPools::GetPedRef(player));
}

bool SpawnParticleAtPlayer(const char* name) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !name || name[0] == '\0') return false;
    const CVector pos = player->GetPosition();
    int fx = 0;
    plugin::Command<plugin::Commands::CREATE_FX_SYSTEM>(name, pos.x, pos.y, pos.z + 1.0f, 0, &fx);
    plugin::Command<plugin::Commands::PLAY_AND_KILL_FX_SYSTEM>(fx);
    return true;
}

bool StartCutscene(const char* name) {
    if (!name || name[0] == '\0') return false;
    CCutsceneMgr::DeleteCutsceneData();
    CCutsceneMgr::LoadCutsceneData(name);
    CCutsceneMgr::StartCutscene();
    return true;
}

void StopCutscene() {
    CCutsceneMgr::DeleteCutsceneData();
}

bool IsCutsceneRunning() {
    return CCutsceneMgr::ms_running || CCutsceneMgr::ms_cutsceneProcessing;
}

const char* GetMissionStatus() {
    static char status[160];
    std::snprintf(status, sizeof(status), "commands=%u missionFlag=%d activeScripts=%s", CTheScripts::CommandsExecuted, CTheScripts::OnAMissionFlag, CTheScripts::pActiveScripts ? "yes" : "no");
    return status;
}

void DisplayHud(bool enable) {
    plugin::Command<plugin::Commands::DISPLAY_HUD>(enable);
    CHud::m_Wants_To_Draw_Hud = enable;
}

void DisplayRadar(bool enable) {
    plugin::Command<plugin::Commands::DISPLAY_RADAR>(enable);
    CHud::bScriptDontDisplayRadar = !enable;
}

void SetVisualFilter(bool enable, int filterId, float) {
    if (!enable) {
        return;
    }

    if (filterId < 0) {
        filterId = 0;
    } else if (filterId > 22) {
        filterId = 22;
    }

    CWeather::OldWeatherType = static_cast<short>(filterId);
    CWeather::NewWeatherType = static_cast<short>(filterId);
}

void TeleportForward(float distance) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    CVector pos = player->GetPosition();
    float angle = player->m_fHeadingCurrent;
    pos.x -= sin(angle) * distance;
    pos.y += cos(angle) * distance;
    TeleportPlayer(pos);
}

void GiveAllWeapons(CPlayerPed* player) {
    if (!player) return;
    const unsigned int weapons[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46 };
    for (const unsigned int weapon : weapons) {
        GiveWeapon(player, weapon, 99999);
    }
}

void GiveWeapon(CPlayerPed* player, unsigned int weaponType, unsigned int ammo) {
    if (!player) return;
    const unsigned int jetpack = static_cast<unsigned int>(-1);
    const unsigned int cellPhone = static_cast<unsigned int>(-2);
    const int hplayer = CPools::GetPedRef(player);

    if (weaponType == jetpack) {
        plugin::Command<plugin::Commands::TASK_JETPACK>(hplayer);
        return;
    }

    if (weaponType == cellPhone) {
        CStreaming::RequestModel(330, PRIORITY_REQUEST);
        CStreaming::LoadAllRequestedModels(false);
        player->ClearWeaponTarget();
        player->SetCurrentWeapon(WEAPONTYPE_UNARMED);
        player->AddWeaponModel(330);
        plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(330);
        return;
    }

    int model = 0;
    plugin::Command<plugin::Commands::GET_WEAPONTYPE_MODEL>(weaponType, &model);
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    if (model == 363) {
        CStreaming::RequestModel(364, PRIORITY_REQUEST);
    }
    CStreaming::LoadAllRequestedModels(false);
    plugin::Command<plugin::Commands::GIVE_WEAPON_TO_CHAR>(hplayer, weaponType, ammo);
    if (model == 363) {
        plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(364);
    }
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
}

void GiveWeaponModel(CPlayerPed* player, unsigned int weaponModel, unsigned int ammo) {
    GiveWeapon(player, weaponModel, ammo);
}

void DropWeapon(CPlayerPed* player) {
    if (!player) return;
    const int hplayer = CPools::GetPedRef(player);
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    plugin::Command<plugin::Commands::GET_OFFSET_FROM_CHAR_IN_WORLD_COORDS>(hplayer, 0.0f, 3.0f, 0.0f, &x, &y, &z);

    const eWeaponType weaponType = player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType;
    if (weaponType == WEAPONTYPE_UNARMED) return;

    int model = 0;
    int pickup = 0;
    plugin::Command<plugin::Commands::GET_WEAPONTYPE_MODEL>(weaponType, &model);
    plugin::Command<plugin::Commands::CREATE_PICKUP_WITH_AMMO>(model, 3, 999, x, y, z, &pickup);
    plugin::Command<plugin::Commands::REMOVE_WEAPON_FROM_CHAR>(hplayer, weaponType);
}

void DropCurrentWeapon(CPlayerPed* player) {
    if (!player) return;
    const int hplayer = CPools::GetPedRef(player);
    plugin::Command<plugin::Commands::REMOVE_WEAPON_FROM_CHAR>(hplayer, player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType);
}

void ClearWeapons(CPlayerPed* player) {
    if (player) player->ClearWeapons();
}

int RemoveTrackedPickups() {
    int removed = 0;
    for (unsigned int i = 0; i < MAX_NUM_PICKUPS; ++i) {
        CPickup& pickup = CPickups::aPickUps[i];
        if (pickup.m_nPickupType == PICKUP_NONE || pickup.m_nFlags.bDisabled) {
            continue;
        }

        pickup.Remove();
        ++removed;
    }
    return removed;
}

void ProcessInfiniteAmmo(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    for (int i = 0; i < 13; i++) {
        player->m_aWeapons[i].m_nAmmoTotal = 9999;
    }
}

void SetFastReload(CPlayerPed* player, bool enable) {
    if (!player) return;
    plugin::Command<plugin::Commands::SET_PLAYER_FAST_RELOAD>(CPools::GetPedRef(player), enable);
}

void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange, bool rapidFire, bool dualWield, bool moveAim, bool moveFire, bool noSpread) {
    if (!player || (!hugeDamage && !longRange && !rapidFire && !dualWield && !moveAim && !moveFire && !noSpread)) return;

    CWeapon& weapon = player->m_aWeapons[player->m_nSelectedWepSlot];
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType, player->GetWeaponSkill(weapon.m_eWeaponType));
    if (!info) return;

    if (hugeDamage) {
        info->m_nDamage = 1000;
    }
    if (longRange) {
        info->m_fTargetRange = 1000.0f;
        info->m_fWeaponRange = 1000.0f;
        info->m_fAccuracy = 1.0f;
        info->m_nFlags.bReload2Start = true;
    }
    if (rapidFire && weapon.m_eWeaponType != WEAPONTYPE_FTHROWER && weapon.m_eWeaponType != WEAPONTYPE_MINIGUN) {
        info->m_nFlags.bContinuosFire = true;
    }
    if (dualWield && (weapon.m_eWeaponType == WEAPONTYPE_PISTOL || weapon.m_eWeaponType == WEAPONTYPE_MICRO_UZI || weapon.m_eWeaponType == WEAPONTYPE_TEC9 || weapon.m_eWeaponType == WEAPONTYPE_SAWNOFF)) {
        info->m_nFlags.bTwinPistol = true;
    }
    if (moveAim) {
        info->m_nFlags.bMoveAim = true;
    }
    if (moveFire) {
        info->m_nFlags.bMoveFire = true;
    }
    if (noSpread) {
        info->m_fAccuracy = 100.0f;
    }
}

void ResetWeaponStats() {
    CWeaponInfo::LoadWeaponData();
}

void SetTime(int hour, int minute) {
    CClock::ms_nGameClockHours = hour;
    CClock::ms_nGameClockMinutes = minute;
}

void GetTime(int& hour, int& minute) {
    hour = CClock::ms_nGameClockHours;
    minute = CClock::ms_nGameClockMinutes;
}

void SyncTimeWithSystemClock() {
    const std::time_t nowTime = std::time(nullptr);
    const std::tm* now = std::localtime(&nowTime);
    if (!now) return;

    CClock::ms_nGameClockHours = now->tm_hour;
    CClock::ms_nGameClockMinutes = now->tm_min;
    CClock::ms_nGameClockSeconds = now->tm_sec;
    CClock::ms_nGameClockMonth = now->tm_mon + 1;
    CClock::ms_nGameClockDays = now->tm_mday;
    CClock::CurrentDay = now->tm_wday + 1;
}

void SetGameSpeed(float speed) {
    CTimer::ms_fTimeScale = speed;
}

float GetGameSpeed() {
    return CTimer::ms_fTimeScale;
}

int GetFpsLimit() {
    return RsGlobal.maxFPS;
}

void SetFpsLimit(int limit) {
    RsGlobal.maxFPS = limit;
}

void SetDisableReplay(bool enable) {
    static constexpr std::array<unsigned char, 1> original = { 0x80 };
    static constexpr std::array<unsigned char, 1> patched = { 0xC3 };
    PatchBytesIfExpected("SA", 0x460500, original, patched, enable);
}

void SetDisableCheats(bool enable) {
    if (enable) {
        plugin::patch::Set<unsigned char>(0x4384D0, 0xE9, false);
        plugin::patch::SetInt(0x4384D1, 0xD0, false);
        plugin::patch::Nop(0x4384D5, 4, false);
        return;
    }

    plugin::patch::Set<unsigned char>(0x4384D0, 0x83, false);
    plugin::patch::SetInt(0x4384D1, -0x7DF0F908, false);
    plugin::patch::SetInt(0x4384D5, 0xCC, false);
}

void SetForbiddenAreaWanted(bool enable) {
    plugin::patch::Set<unsigned char>(0x441770, enable ? 0x83 : 0xC3, false);
}

void SetFreePayNSpray(bool enable) {
    plugin::patch::Set<bool>(0x96C009, enable, false);
}

void SetFasterClock(bool enable) {
    plugin::patch::Set<bool>(0x96913B, enable, false);
}

void SetFreezeTime(bool enable) {
    Log::Info(std::string("SA 冻结时间使用控制器锁定：") + (enable ? "开启" : "关闭"));
}

int GetDaysPassed() {
    return *reinterpret_cast<int*>(0xB79038);
}

void SetDaysPassed(int days) {
    *reinterpret_cast<int*>(0xB79038) = days;
}

float GetGravity() {
    return *reinterpret_cast<float*>(0x863984);
}

void SetGravity(float gravity) {
    *reinterpret_cast<float*>(0x863984) = gravity;
}

namespace {
    int lastXMenuPickupHandle = -1;
    CVector lastXMenuPickupPosition;

    unsigned char NormalizePickupType(unsigned int type) {
        if (type > PICKUP_ONCE_FOR_MISSION) {
            return PICKUP_ONCE;
        }
        return static_cast<unsigned char>(type);
    }

    int CreateXMenuPickup(const GameLogic::PickupOptions& options, const CVector& pos) {
        const int model = static_cast<int>(options.modelId);
        if (model <= 0) {
            return -1;
        }

        CStreaming::RequestModel(model, PRIORITY_REQUEST);
        CStreaming::LoadAllRequestedModels(false);
        const int handle = CPickups::GenerateNewOne(
            pos,
            static_cast<unsigned int>(model),
            NormalizePickupType(options.type),
            options.quantity,
            options.moneyPerDay,
            options.empty,
            nullptr
        );
        plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
        return handle;
    }
}

int SpawnPickupNearPlayer(const PickupOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) {
        return -1;
    }

    const int hplayer = CPools::GetPedRef(player);
    CVector pos;
    plugin::Command<plugin::Commands::GET_OFFSET_FROM_CHAR_IN_WORLD_COORDS>(hplayer, 0.0f, 2.0f, 0.0f, &pos.x, &pos.y, &pos.z);
    pos.z += 0.2f;

    const int handle = CreateXMenuPickup(options, pos);
    if (handle >= 0) {
        lastXMenuPickupHandle = handle;
        lastXMenuPickupPosition = pos;
    }
    return handle;
}

bool UpdateLastPickup(const PickupOptions& options) {
    if (lastXMenuPickupHandle < 0) {
        return false;
    }

    CPickups::RemovePickUp(lastXMenuPickupHandle);
    const int handle = CreateXMenuPickup(options, lastXMenuPickupPosition);
    if (handle < 0) {
        lastXMenuPickupHandle = -1;
        return false;
    }

    lastXMenuPickupHandle = handle;
    return true;
}

bool RemoveLastPickup() {
    if (lastXMenuPickupHandle < 0) {
        return false;
    }

    CPickups::RemovePickUp(lastXMenuPickupHandle);
    lastXMenuPickupHandle = -1;
    return true;
}

} // namespace GameLogic
