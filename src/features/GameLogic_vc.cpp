#include "GameLogic.h"
#include "utils/Log.h"
#include "resources/ResourceData.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CStreaming.h"
#include "CVehicle.h"
#include "CAutomobile.h"
#include "CPed.h"
#include "CModelInfo.h"
#include "CWeaponInfo.h"
#include "CPickups.h"
#include "ePickupType.h"
#include "extensions/ScriptCommands.h"
#include "Patch.h"
#include "CTimer.h"
#include "CClock.h"
#include "CHud.h"
#include "CWeather.h"
#include "CCutsceneMgr.h"
#include "CTheScripts.h"
#include "ui/MenuState.h"
#include "ePedType.h"
#include "utils/StdExtras.h"
#include "rw/skeleton.h"
#include <cmath>
#include <cstdint>
#include <windows.h>
#include <string>
#include <ctime>
#include <string>

namespace {
std::vector<int> g_trackedPickupHandles;
bool g_weaponTweaksNeedReapply = true;

bool PatchByteIfExpected(const char* label, std::uintptr_t address, unsigned char original, unsigned char patched, bool enable) {
    auto* target = reinterpret_cast<unsigned char*>(address);

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
        (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
        reinterpret_cast<std::uintptr_t>(target) + 1 > reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize) {
        Log::Warn(std::string(label) + " 回放补丁地址不可用，已跳过");
        return false;
    }

    const unsigned char desired = enable ? patched : original;
    if (*target == desired) {
        return true;
    }

    const unsigned char acceptableCurrent = enable ? original : patched;
    if (*target != acceptableCurrent) {
        Log::Warn(std::string(label) + " 回放补丁位置与预期不符，已跳过");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log::Warn(std::string(label) + " 回放补丁无法修改内存权限，已跳过");
        return false;
    }

    *target = desired;
    FlushInstructionCache(GetCurrentProcess(), target, 1);

    DWORD unusedProtect = 0;
    VirtualProtect(target, 1, oldProtect, &unusedProtect);
    return true;
}

bool PatchBytesIfExpected(const char* label, std::uintptr_t address, const unsigned char* original, const unsigned char* patched, std::size_t size, bool enable) {
    auto* target = reinterpret_cast<unsigned char*>(address);

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
        (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
        reinterpret_cast<std::uintptr_t>(target) + size > reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize) {
        Log::Warn(std::string(label) + " 补丁地址不可用，已跳过");
        return false;
    }

    const unsigned char* desired = enable ? patched : original;
    if (std::memcmp(target, desired, size) == 0) {
        return true;
    }

    const unsigned char* acceptableCurrent = enable ? original : patched;
    if (std::memcmp(target, acceptableCurrent, size) != 0) {
        Log::Warn(std::string(label) + " 补丁位置与预期不符，已跳过");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log::Warn(std::string(label) + " 补丁无法修改内存权限，已跳过");
        return false;
    }

    std::memcpy(target, desired, size);
    FlushInstructionCache(GetCurrentProcess(), target, size);

    DWORD unusedProtect = 0;
    VirtualProtect(target, size, oldProtect, &unusedProtect);
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

bool IsWorldReady() {
    // VC 与 SA 类似：玩家 ped 就绪即可；III 的严格门闩在 GameLogic_iii
    return CWorld::Players && CWorld::Players[CWorld::PlayerInFocus].m_pPed != nullptr;
}

void NotifyGameInit() {
}

void Init() {
}

ProofState GetPlayerProofState(CPlayerPed* player) {
    ProofState state;
    if (!player) return state;
    state.bullet = player->bBulletProof;
    state.collision = player->bCollisionProof;
    state.explosion = player->bExplosionProof;
    state.fire = player->bFireProof;
    state.melee = player->bMeleeProof;
    state.nonPlayer = player->bImmuneToNonPlayerDamage;
    return state;
}

void SetPlayerProofState(CPlayerPed* player, const ProofState& state) {
    if (!player) return;
    player->bBulletProof = state.bullet;
    player->bCollisionProof = state.collision;
    player->bExplosionProof = state.explosion;
    player->bFireProof = state.fire;
    player->bMeleeProof = state.melee;
    player->bImmuneToNonPlayerDamage = state.nonPlayer;
}

void ApplyGodMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    player->bBulletProof = enable;
    player->bCollisionProof = enable;
    player->bExplosionProof = enable;
    player->bFireProof = enable;
    player->bMeleeProof = enable;
    player->bImmuneToNonPlayerDamage = enable;
}

void SetInfiniteSprint(bool enable) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_bInfiniteSprint = enable;
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
    pInfo->m_nDisplayMoney += amount;
}

int GetMoney() {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    return pInfo->m_nMoney;
}

void SetMoney(int amount) {
    CPlayerInfo* pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    pInfo->m_nMoney = amount;
    pInfo->m_nDisplayMoney = amount;
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
    if (enable) {
        plugin::patch::Nop(0x42C184, 5);
        plugin::patch::Nop(0x42C068, 5);
        plugin::patch::Nop(0x42BC7B, 5);
    } else {
        plugin::patch::SetRaw(0x42C184, (void*)"\xE8\xB7\x35\x0D\x00", 5);
        plugin::patch::SetRaw(0x42C068, (void*)"\xE8\xD3\x36\x0D\x00", 5);
        plugin::patch::SetRaw(0x42BC7B, (void*)"\xE8\xC0\x3A\x0D\x00", 5);
    }
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
        if (player->m_fHealth < 100.0f) {
            player->m_fHealth += 2.0f;
            if (player->m_fHealth > 100.0f) player->m_fHealth = 100.0f;
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
        player->Teleport(deathPos);
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
    state.nonPlayer = vehicle->bImmuneToNonPlayerDamage;
    return state;
}

void SetVehicleProofState(CVehicle* vehicle, const ProofState& state) {
    if (!vehicle) return;
    vehicle->bBulletProof = state.bullet;
    vehicle->bCollisionProof = state.collision;
    vehicle->bExplosionProof = state.explosion;
    vehicle->bFireProof = state.fire;
    vehicle->bMeleeProof = state.melee;
    vehicle->bImmuneToNonPlayerDamage = state.nonPlayer;
}

void RepairVehicle(CVehicle* vehicle) {
    if (!vehicle) return;
    if (vehicle->m_nVehicleClass == 0) {
        reinterpret_cast<CAutomobile*>(vehicle)->Fix();
    }
    vehicle->m_fHealth = 1000.0f;
}

void StopVehicle(CVehicle* vehicle) {
    if (!vehicle) return;
    vehicle->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
    vehicle->m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);
}

void UnflipVehicle(CVehicle* vehicle) {
    if (!vehicle || !vehicle->IsUpsideDown()) return;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    vehicle->GetOrientation(x, y, z);
    vehicle->SetOrientation(x, y + 135.0f, z);
    StopVehicle(vehicle);
}

void SetVehicleForwardSpeed(CVehicle* vehicle, float speed) {
    if (!vehicle) return;

    CPlayerPed* player = FindPlayerPed();
    if (!player || player->m_pVehicle != vehicle) return;

    const CVector forward = vehicle->GetForward();
    const float velocity = speed / 50.0f;
    vehicle->m_vecMoveSpeed = CVector(forward.x * velocity, forward.y * velocity, forward.z * velocity);
}

void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed) {
    if (!enable || !vehicle) return;
    
    CPad* pad = CPad::GetPad(0);
    if (!pad || pad->GetAccelerate() <= 0) return;

    SetVehicleForwardSpeed(vehicle, speed);
}

void SetVehicleEngine(CVehicle* vehicle, bool enable) {
    if (vehicle) vehicle->bEngineOn = enable;
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

bool GetVehicleAlwaysSkidMarks(CVehicle*) {
    return false;
}

void SetVehicleAlwaysSkidMarks(CVehicle*, bool) {
}

bool GetVehicleDisableParticles(CVehicle*) {
    return false;
}

void SetVehicleDisableParticles(CVehicle*, bool) {
}

bool GetVehicleDriverTargetable(CVehicle*) {
    return false;
}

void SetVehicleDriverTargetable(CVehicle*, bool) {
}

bool GetVehicleHeatSeekingTargetable(CVehicle*) {
    return false;
}

void SetVehicleHeatSeekingTargetable(CVehicle*, bool) {
}

bool GetVehiclePetrolTankWeakPoint(CVehicle*) {
    return false;
}

void SetVehiclePetrolTankWeakPoint(CVehicle*, bool) {
}

bool GetVehicleSirenOrAlarm(CVehicle*) {
    return false;
}

void SetVehicleSirenOrAlarm(CVehicle*, bool) {
}

bool GetVehicleTakeLessDamage(CVehicle*) {
    return false;
}

void SetVehicleTakeLessDamage(CVehicle*, bool) {
}

void BlowUpAllVehicles() {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;

    for (CVehicle* vehicle : CPools::ms_pVehiclePool) {
        if (vehicle) {
            vehicle->BlowUpCar(player);
        }
    }
}

void DeleteVehicle(CVehicle* vehicle) {
    if (!vehicle) return;

    CPlayerPed* player = FindPlayerPed();
    if (player) {
        const int hplayer = CPools::GetPedRef(player);
        if (plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer) && player->m_pVehicle == vehicle) {
            Log::Warn("VC 载具清理跳过：玩家正在使用目标载具");
            return;
        }
    }

    const int hveh = CPools::GetVehicleRef(vehicle);
    if (hveh == 0) return;

    plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
    plugin::Command<plugin::Commands::DELETE_CAR>(hveh);
}

bool IsAircraftModel(int model) {
    return CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)
        || model == 155 || model == 165 || model == 177 || model == 180 || model == 181
        || model == 190 || model == 199 || model == 217 || model == 218 || model == 227;
}

bool IsValidVehicleModel(unsigned int modelId) {
    if (!Resources::IsKnownVehicleModel(modelId)) {
        return false;
    }

    const int model = static_cast<int>(modelId);
    switch (model) {
    case 180: // Airtrain
    case 181: // Dead Dodo
    case 217: // Maverick
        Log::Warn("VC 载具生成被拒绝：模型 ID " + std::to_string(model) + " 不适合通过普通刷车入口生成");
        return false;
    default:
        break;
    }

    return model >= 130 && model <= 236 && CModelInfo::IsVehicleModel(model);
}

bool IsValidPedModel(unsigned int modelId) {
    const int model = static_cast<int>(modelId);
    return model >= 0 && model < CModelInfo::ms_modelInfoCount && CModelInfo::IsPedModel(model);
}

bool SetPlayerSkin(unsigned int modelId) {
    Log::Warn("VC 不支持安全玩家皮肤切换，已跳过模型 ID " + std::to_string(modelId));
    return false;
}

bool ApplyPlayerClothes(int textureId, int modelId, int bodyPart) {
    Log::Warn("VC 不支持 CJ 衣服接口，已跳过");
    return false;
}

bool SetPlayerCustomSkin(const char* name) {
    return false;
}

bool SetPlayerStat(int statId, float value) {
    Log::Warn("VC 不支持 SA stats 接口，已跳过 stat " + std::to_string(statId));
    return false;
}

CVehicle* SpawnVehicle(unsigned int modelId, const SpawnVehicleOptions& options) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return nullptr;

    const int model = static_cast<int>(modelId);
    if (!IsValidVehicleModel(modelId)) {
        Log::Warn("VC 载具生成被拒绝：无效模型 ID " + std::to_string(model));
        return nullptr;
    }

    const int hplayer = CPools::GetPedRef(player);
    const int interior = player->m_nAreaCode;
    CVector pos = player->GetPosition();
    float speed = 0.0f;

    CVehicle* previousVehicle = nullptr;
    int previousVehicleHandle = 0;

    if (options.asDriver && options.cleanupPrevious && plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer)) {
        previousVehicle = player->m_pVehicle;
        if (previousVehicle) {
            previousVehicleHandle = CPools::GetVehicleRef(previousVehicle);
            pos = previousVehicle->GetPosition();
            plugin::Command<plugin::Commands::GET_CAR_SPEED>(previousVehicleHandle, &speed);
            plugin::Command<plugin::Commands::WARP_CHAR_FROM_CAR_TO_COORD>(hplayer, pos.x, pos.y, pos.z);
        }
    } else if (!options.cleanupPrevious) {
        pos = FindSideSpawnPosition(player, pos);
    }

    if (interior == 0) {
        if (options.aircraftInAir && IsAircraftModel(model)) {
            pos.z = 400.0f;
        } else if (options.cleanupPrevious) {
            pos.z -= 5.0f;
        }
    }

    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);

    int hveh = 0;
    if (options.asDriver) {
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + (options.cleanupPrevious ? 4.0f : 1.0f), &hveh);
    } else {
        if (options.cleanupPrevious) {
            pos = player->TransformFromObjectSpace(CVector(0.0f, 10.0f, 0.0f));
        }
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + 1.0f, &hveh);
    }

    CVehicle* vehicle = CPools::GetVehicle(hveh);
    if (!vehicle) {
        CStreaming::SetModelIsDeletable(model);
        Log::Error("VC 载具生成失败：脚本命令 CREATE_CAR 未返回有效载具，模型 ID " + std::to_string(model));
        return nullptr;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    player->GetOrientation(x, y, z);
    vehicle->SetOrientation(x, y, z);
    vehicle->m_eDoorLock = DOORLOCK_UNLOCKED;
    vehicle->m_nAreaCode = interior;

    if (options.asDriver) {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(hplayer, hveh);
        SetVehicleForwardSpeed(vehicle, speed);
    }

    if (previousVehicleHandle != 0 && previousVehicleHandle != hveh) {
        plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(previousVehicleHandle);
    }

    if (!options.asDriver) {
        plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
    }
    CStreaming::SetModelIsDeletable(model);
    plugin::Command<plugin::Commands::RESTORE_CAMERA_JUMPCUT>();
    return vehicle;
}

void TeleportPlayer(CVector pos, int interiorID) {
    CPlayerPed* pPlayer = FindPlayerPed();
    if (!pPlayer) return;
    CVehicle* pVeh = pPlayer->m_pVehicle;

    CStreaming::LoadScene(&pos);
    CStreaming::LoadSceneCollision(&pos);
    CStreaming::LoadAllRequestedModels(false);

    if (pVeh && pPlayer->m_bInVehicle) {
        pVeh->m_nAreaCode = interiorID;
        pVeh->Teleport(pos);
    } else {
        pPlayer->Teleport(pos);
    }

    pPlayer->m_nAreaCode = interiorID;
    plugin::Command<plugin::Commands::SET_AREA_VISIBLE>(interiorID);
}

void TeleportMapPosition(CVector pos, bool spawnUnderwater) {
    TeleportPlayer(pos);
}

bool TeleportMarker(bool spawnUnderwater) {
    return false;
}

void ApplyVehicleAppearance(CVehicle* vehicle, const VehicleAppearanceOptions& options) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::CHANGE_CAR_COLOUR>(CPools::GetVehicleRef(vehicle), options.primaryColor, options.secondaryColor);
}

void OpenVehicleDoor(CVehicle*, int) {
    Log::Warn("VC 不支持安全车门打开接口，已跳过");
}

void PopVehicleDoor(CVehicle*, int) {
    Log::Warn("VC 不支持安全车门拆卸接口，已跳过");
}

void WarpPlayerToVehicleSeat(CVehicle* vehicle, int seatIndex) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !vehicle) return;
    if (seatIndex <= 0) {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(CPools::GetPedRef(player), CPools::GetVehicleRef(vehicle));
    } else {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR_AS_PASSENGER>(CPools::GetPedRef(player), CPools::GetVehicleRef(vehicle), seatIndex - 1);
    }
}

void ProcessAutoDrive(CVehicle*, bool enable, float) {
    if (enable) Log::Warn("VC 不支持 AutoDrive，已安全降级");
}

void SetTrafficDensity(float density) {
    static bool initialized = false;
    static float lastDensity = 0.0f;
    if (!initialized || lastDensity != density) {
        initialized = true;
        lastDensity = density;
        Log::Warn("VC 暂未接入交通密度调整，已跳过 " + std::to_string(density));
    }
}

void SetFlyingCars(bool enable) {
    plugin::patch::Set<bool>(0xA10B28, enable, false);
}

void SetVehicleNoDerail(bool) {}
void SetVehicleFlipNoBurn(bool) {}
void ProcessVehicleCheats(CVehicle*) {
    plugin::patch::Set<bool>(0xA10B28, MenuState::VehicleFlyingCars, false);
    plugin::patch::Set<bool>(0xA10B11, MenuState::VehicleBoatFly, false);
    plugin::patch::Set<bool>(0xA10B81, MenuState::VehicleDriveWater, false);
    plugin::patch::Set<bool>(0xA10ADC, MenuState::VehicleGreenLights, false);
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

CPed* SpawnPedAtMarker(const PedSpawnOptions&) {
    Log::Warn("VC 暂不支持标记点生成 Ped，已安全降级");
    return nullptr;
}

void DeletePed(CPed* ped) {
    if (!ped) return;
    plugin::Command<plugin::Commands::DELETE_CHAR>(CPools::GetPedRef(ped));
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

void DestroyAllPeds() {
    CPlayerPed* player = FindPlayerPed();
    for (CPed* ped : CPools::ms_pPedPool) {
        if (!ped || ped == static_cast<CPed*>(player) || IsMissionPed(ped)) continue;
        DeletePed(ped);
    }
}

void SetElvisEverywhere(bool enable) { }
void SetEveryoneArmed(bool enable) { plugin::patch::Set<bool>(0xA10AB3, enable, false); }
void SetPedsMayhem(bool enable) { }
void SetPedsAtkRocket(bool enable) { }
void SetPedsRiot(bool enable) { }
void SetSlutMagnet(bool enable) { plugin::patch::Set<bool>(0xA10B5F, enable, false); }
void SetGangsControl(bool enable) { }
void SetGangsEverywhere(bool enable) { }
void SetPedNoProstitutes(bool enable) { plugin::patch::Set<bool>(0xA10B99, enable, false); }
void SetPedNastyLimbs(bool) {}
void SetGangWarsActive(bool) {}
void StartGangWar(bool) {}
void EndGangWar() {}
int GetGangZoneDensity(int) { return 0; }
void SetGangZoneDensity(int, int) {}
unsigned int GetGangMemberModel(unsigned int, unsigned int) { return 0; }
void SetGangMemberModel(unsigned int, unsigned int, unsigned int) {}
void ResetGangModels() {}
void SetGangWeapons(unsigned int, int, int, int) {}

bool IsCopPed(const CPed* ped) {
    return ped && ped->m_nPedType == PED_TYPE_COP;
}
bool IsGangPed(const CPed* ped) {
    if (!ped) return false;
    int t = ped->m_nPedType;
    // VC: GANG1.. etc and specific like VERCETTI
    return t == PED_TYPE_GANG1 || t == PED_TYPE_GANG2 || t == PED_TYPE_GANG3 ||
           t == PED_TYPE_GANG4 || t == PED_TYPE_GANG5 || t == PED_TYPE_GANG6 ||
           t == PED_TYPE_GANG7 || t == PED_TYPE_GANG8 || t == PED_TYPE_GANG9 ||
           t == PED_TYPE_GANG_VERCETTI;
}

void SetPedsNoFire(bool /*enable*/) { /* hook + process suppression */ }

void ClearPedAiming(CPed* ped) {
    if (ped) ped->ClearLookFlag();
}

bool ShouldSuppressPedFire(CPed* ped) {
    if (!ped) return false;
    CPlayerPed* player = FindPlayerPed();
    if (ped == static_cast<CPed*>(player)) return false;

    if (!MenuState::PedsNoFire) return false;

    // VC mission: m_nPedStatus == 2 per summary
    const bool isMission = (ped->m_nPedStatus == 2);
    const bool isCop = IsCopPed(ped);
    const bool isGang = IsGangPed(ped);
    const bool isCiv = !isMission && !isCop && !isGang;

    if (isMission && MenuState::PedsNoFireMission) return true;
    if (isCop && MenuState::PedsNoFirePolice) return true;
    if (isGang && MenuState::PedsNoFireGangs) return true;
    if (isCiv && MenuState::PedsNoFireCivilians) return true;
    return false;
}

bool IsMissionPed(CPed* ped) {
    if (!ped) return false;
    return ped->m_nPedStatus == 2;
}

bool PlayPlayerAnimation(const char*, const char*, bool) {
    static bool logged = false;
    if (!logged) {
        logged = true;
        Log::Warn("VC 暂不支持动画页播放，已安全降级");
    }
    return false;
}

bool PlayAnimationEx(const char* group, const char* name, bool loop, bool, bool) {
    return PlayPlayerAnimation(group, name, loop);
}

void StopPlayerAnimation() {
}

void ProcessSmokingEffect(CPlayerPed*, bool) {}

void ProcessFliesEffect(CPlayerPed*, bool) {}

void ProcessVisualExtras() {}
void MaxWeaponSkills() {}
void MaxVehicleSkills() {}
void ApplyAimSkinChanger() {}
void ProcessWeaponAutoAim(bool) {}

void DisplayHud(bool enable) {
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
    } else if (filterId > 6) {
        filterId = 6;
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

namespace {
    int GetWeaponModel(eWeaponType weaponType) {
        return plugin::CallAndReturnDynGlobal<int, int>(0x4418B0, static_cast<int>(weaponType));
    }

    eWeaponType GetWeaponTypeFromModel(int model) {
        for (int i = 0; i <= 39; ++i) {
            const auto weaponType = static_cast<eWeaponType>(i);
            if (GetWeaponModel(weaponType) == model) {
                return weaponType;
            }
        }
        return WEAPONTYPE_UNARMED;
    }

    void ClearPlayerWeapon(CPlayerPed* player, eWeaponType weaponType) {
        if (!player) return;
        const CWeaponInfo* weaponInfo = CWeaponInfo::GetWeaponInfo(weaponType);
        if (!weaponInfo) return;

        const int weaponSlot = weaponInfo->m_WeaponSlot;
        if (weaponSlot == -1) return;

        CWeapon* weapon = &player->m_aWeapons[weaponSlot];
        if (weapon->m_eWeaponType != weaponType) return;

        if (player->m_nCurrentWeapon == weaponSlot) {
            CWeaponInfo* unarmedInfo = CWeaponInfo::GetWeaponInfo(WEAPONTYPE_UNARMED);
            if (unarmedInfo) {
                player->SetCurrentWeapon(unarmedInfo->m_WeaponSlot);
            }
        }
        weapon->Shutdown();
    }
}

void GiveAllWeapons(CPlayerPed* player) {
    if (!player) return;
    const unsigned int weapons[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33 };
    for (const unsigned int weapon : weapons) {
        GiveWeapon(player, weapon, 99999);
    }
}

void GiveWeapon(CPlayerPed* player, unsigned int weaponType, unsigned int ammo) {
    if (!player) return;
    const int model = GetWeaponModel(static_cast<eWeaponType>(weaponType));
    if (model <= 0) {
        Log::Warn("VC 武器发放被拒绝：武器类型未映射到有效模型，武器类型 " + std::to_string(weaponType));
        return;
    }
    const int hplayer = CPools::GetPedRef(player);
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);
    plugin::Command<plugin::Commands::GIVE_WEAPON_TO_CHAR>(hplayer, static_cast<eWeaponType>(weaponType), ammo);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
}

void GiveWeaponModel(CPlayerPed* player, unsigned int weaponModel, unsigned int ammo) {
    if (!player) return;
    const int hplayer = CPools::GetPedRef(player);
    const int model = static_cast<int>(weaponModel);
    if (model <= 0) {
        Log::Warn("VC 武器发放被拒绝：无效模型 ID " + std::to_string(model));
        return;
    }
    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);

    const eWeaponType weaponType = GetWeaponTypeFromModel(model);
    if (weaponType == WEAPONTYPE_UNARMED) {
        Log::Warn("VC 武器发放被拒绝：模型未映射到有效武器类型，模型 ID " + std::to_string(model));
        plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
        return;
    }
    plugin::Command<plugin::Commands::GIVE_WEAPON_TO_CHAR>(hplayer, weaponType, ammo);
    plugin::Command<plugin::Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(model);
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

    const int model = GetWeaponModel(weaponType);
    int pickup = 0;
    plugin::Command<plugin::Commands::CREATE_PICKUP_WITH_AMMO>(model, 3, 999, x, y, z, &pickup);
    ClearPlayerWeapon(player, weaponType);
}

void DropCurrentWeapon(CPlayerPed* player) {
    if (!player) return;
    ClearPlayerWeapon(player, player->m_aWeapons[player->m_nCurrentWeapon].m_eWeaponType);
}

void ClearWeapons(CPlayerPed* player) {
    if (player) player->ClearWeapons();
}

int RemoveTrackedPickups() {
    int removed = 0;
    for (int handle : g_trackedPickupHandles) {
        if (handle < 0) {
            continue;
        }
        CPickups::RemovePickUp(handle);
        ++removed;
    }
    g_trackedPickupHandles.clear();
    return removed;
}

void ProcessInfiniteAmmo(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    for (int i = 0; i < 10; i++) {
        player->m_aWeapons[i].m_nAmmoTotal = 9999;
    }
}

void SetFastReload(CPlayerPed* player, bool enable) {
    if (!player) return;
    plugin::Command<plugin::Commands::SET_PLAYER_FAST_RELOAD>(CPools::GetPedRef(player), enable);
}

void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange, bool rapidFire, bool dualWield, bool moveAim, bool moveFire, bool noSpread, bool customFireRate, float fireRate) {
    (void)rapidFire;
    (void)dualWield;
    (void)moveAim;
    (void)moveFire;

    auto scaleAnimLoop = [](float& start, float& end, float& fire, float rate) {
        if (rate < 0.1f) {
            rate = 0.1f;
        }
        if (rate > 20.0f) {
            rate = 20.0f;
        }
        if (std::fabs(rate - 1.0f) < 0.001f || end <= start) {
            return;
        }
        const float oldStart = start;
        const float oldEnd = end;
        const float oldFire = fire;
        end = oldStart + (oldEnd - oldStart) / rate;
        fire = oldStart + (oldFire - oldStart) / rate;
        if (fire < start) {
            fire = start;
        }
        if (fire > end) {
            fire = end;
        }
    };

    const float effectiveRate = (customFireRate && fireRate > 0.1f) ? fireRate : 1.0f;
    const bool fireRateActive = customFireRate && std::fabs(effectiveRate - 1.0f) >= 0.001f;
    const bool any = hugeDamage || longRange || noSpread || fireRateActive;
    static bool s_wasAny = false;
    static int s_lastType = -1;
    static unsigned char s_lastMask = 0;
    static int s_lastRateQ = 100;

    const unsigned char mask =
        static_cast<unsigned char>((hugeDamage ? 1 : 0) |
                                   (longRange ? 2 : 0) |
                                   (noSpread ? 4 : 0) |
                                   (fireRateActive ? 8 : 0));
    const int rateQ = fireRateActive ? static_cast<int>(effectiveRate * 100.0f + 0.5f) : 100;

    if (!any) {
        if (s_wasAny) {
            CWeaponInfo::LoadWeaponData();
            s_wasAny = false;
            s_lastType = -1;
            s_lastMask = 0;
            s_lastRateQ = 100;
        }
        return;
    }
    if (!player) {
        return;
    }

    CWeapon& weapon = player->m_aWeapons[player->m_nCurrentWeapon];
    const int type = static_cast<int>(weapon.m_eWeaponType);
    if (s_wasAny && !g_weaponTweaksNeedReapply && s_lastType == type && s_lastMask == mask && s_lastRateQ == rateQ) {
        return;
    }

    if (s_wasAny || g_weaponTweaksNeedReapply) {
        CWeaponInfo::LoadWeaponData();
    }

    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType);
    if (!info) {
        return;
    }

    if (hugeDamage) {
        info->m_nDamage = 1000;
    }
    if (longRange) {
        info->m_fRange = 1000.0f;
    }
    if (noSpread) {
        info->m_fSpread = 0.0f;
    }
    if (fireRateActive) {
        // III/VC：FiringRate 越大越慢，按倍率缩小间隔
        if (info->m_nFiringRate > 0) {
            const float scaled = static_cast<float>(info->m_nFiringRate) / effectiveRate;
            info->m_nFiringRate = scaled < 1.0f ? 1u : static_cast<unsigned int>(scaled);
        }
        scaleAnimLoop(info->m_fAnimLoopStart, info->m_fAnimLoopEnd, info->m_fAnimFrameFire, effectiveRate);
        scaleAnimLoop(info->m_fAnim2LoopStart, info->m_fAnim2LoopEnd, info->m_fAnim2FrameFire, effectiveRate);
    }

    s_wasAny = true;
    s_lastType = type;
    s_lastMask = mask;
    s_lastRateQ = rateQ;
    g_weaponTweaksNeedReapply = false;
}

void ResetWeaponStats() {
    CWeaponInfo::LoadWeaponData();
    g_weaponTweaksNeedReapply = true;
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
    PatchByteIfExpected("VC", 0x624EC0, 0x80, 0xC3, enable);
}

void SetDisableCheats(bool enable) {
    static const unsigned char originalFirst[] = { 0x88, 0xD8, 0x89, 0xF1, 0x50 };
    static const unsigned char patchedFirst[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    static const unsigned char originalSecond[] = { 0xE8, 0x34, 0x91, 0xEA, 0xFF };
    static const unsigned char patchedSecond[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

    const bool firstOk = PatchBytesIfExpected("VC 禁用作弊码", 0x602BD8, originalFirst, patchedFirst, sizeof(originalFirst), enable);
    const bool secondOk = PatchBytesIfExpected("VC 禁用作弊码", 0x602BE7, originalSecond, patchedSecond, sizeof(originalSecond), enable);
    if (!firstOk || !secondOk) {
        Log::Warn("VC 禁用作弊码补丁未完整应用，当前游戏版本可能不匹配");
    }
}

void SetForbiddenAreaWanted(bool enable) {
    Log::Warn(std::string("VC 不支持禁止通缉区域开关：") + (enable ? "开启" : "关闭"));
}

void SetFreePayNSpray(bool enable) {
    Log::Warn(std::string("VC 不支持免费喷漆店开关：") + (enable ? "开启" : "关闭"));
}

void SetFasterClock(bool enable) {
    plugin::patch::Set<bool>(0xA10B87, enable, false);
}

void ProcessSolidWater(CPlayerPed* player, bool enable) {
}

void SetNoWaterPhysics(bool enable) {
}

void SetFreezeTime(bool enable) {
    Log::Info(std::string("VC 冻结时间使用控制器锁定：") + (enable ? "开启" : "关闭"));
}

int GetDaysPassed() {
    return *reinterpret_cast<int*>(0x97F1F4);
}

void SetDaysPassed(int days) {
    *reinterpret_cast<int*>(0x97F1F4) = days;
}

float GetGravity() {
    return *reinterpret_cast<float*>(0x68F5F0);
}

void SetGravity(float gravity) {
    *reinterpret_cast<float*>(0x68F5F0) = gravity;
}

namespace {
    int lastXMenuPickupHandle = -1;
    CVector lastXMenuPickupPosition;

    unsigned char NormalizePickupType(unsigned int type) {
        if (type > PICKUP_PROPERTY_FORSALE) {
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
        if (handle >= 0) {
            g_trackedPickupHandles.push_back(handle);
        }
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
    for (auto it = g_trackedPickupHandles.begin(); it != g_trackedPickupHandles.end(); ++it) {
        if (*it == lastXMenuPickupHandle) {
            g_trackedPickupHandles.erase(it);
            break;
        }
    }
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
    for (auto it = g_trackedPickupHandles.begin(); it != g_trackedPickupHandles.end(); ++it) {
        if (*it == lastXMenuPickupHandle) {
            g_trackedPickupHandles.erase(it);
            break;
        }
    }
    lastXMenuPickupHandle = -1;
    return true;
}

} // namespace GameLogic
