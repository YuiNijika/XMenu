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
#include "ui/MenuState.h"
#include "rw/skeleton.h"
#include "CCheat.h"
#include "CCamera.h"
#include "CPad.h"
#include "CGangWars.h"
#include "CGangs.h"
#include "CPopulation.h"
#include "CTheZones.h"
#include "utils/StdExtras.h"
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

// 首次启用前备份原字节，关闭时只还原备份，避免硬编码/错版本写坏代码
struct CodePatch {
    std::uintptr_t address = 0;
    std::size_t size = 0;
    bool backedUp = false;
    unsigned char backup[32]{};

    bool EnsureBackup() {
        if (backedUp || size == 0 || size > sizeof(backup) || address == 0) {
            return backedUp;
        }
        auto* target = reinterpret_cast<unsigned char*>(address);
        MEMORY_BASIC_INFORMATION memoryInfo{};
        if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT ||
            (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
            reinterpret_cast<std::uintptr_t>(target) + size >
                reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize) {
            return false;
        }
        std::memcpy(backup, target, size);
        backedUp = true;
        return true;
    }

    bool ApplyBytes(const void* bytes) {
        if (!bytes || !EnsureBackup()) {
            return false;
        }
        plugin::patch::SetRaw(address, const_cast<void*>(bytes), static_cast<int>(size));
        return true;
    }

    bool ApplyNop() {
        if (!EnsureBackup()) {
            return false;
        }
        plugin::patch::Nop(address, static_cast<int>(size));
        return true;
    }

    bool Restore() {
        if (!backedUp) {
            return false;
        }
        plugin::patch::SetRaw(address, backup, static_cast<int>(size));
        return true;
    }
};

CodePatch MakePatch(std::uintptr_t address, std::size_t size) {
    CodePatch patch;
    patch.address = address;
    patch.size = size;
    return patch;
}

// 动画 IFP：禁止同帧 PLAY + REMOVE，延迟卸载
char g_pendingAnimGroup[32] = {};
DWORD g_pendingAnimRemoveAt = 0;
std::string g_activeAnimGroup;

void RequestAnimGroupKeepAlive(const char* group) {
    if (!group || group[0] == '\0' || std::strcmp(group, "PED") == 0) {
        return;
    }
    if (!g_activeAnimGroup.empty() && g_activeAnimGroup != group) {
        plugin::Command<plugin::Commands::REMOVE_ANIMATION>(g_activeAnimGroup.c_str());
    }
    plugin::Command<plugin::Commands::REQUEST_ANIMATION>(group);
    plugin::Command<plugin::Commands::LOAD_ALL_MODELS_NOW>();
    g_activeAnimGroup = group;
    g_pendingAnimGroup[0] = '\0';
    g_pendingAnimRemoveAt = 0;
}

void ScheduleAnimGroupUnload(const char* group, DWORD delayMs = 8000) {
    if (!group || group[0] == '\0' || std::strcmp(group, "PED") == 0) {
        return;
    }
    if (g_activeAnimGroup == group) {
        std::snprintf(g_pendingAnimGroup, sizeof(g_pendingAnimGroup), "%s", group);
        g_pendingAnimRemoveAt = GetTickCount() + delayMs;
    }
}

void ProcessDeferredAnimUnload() {
    if (g_pendingAnimGroup[0] == '\0' || g_pendingAnimRemoveAt == 0) {
        return;
    }
    if (GetTickCount() < g_pendingAnimRemoveAt) {
        return;
    }
    if (g_activeAnimGroup == g_pendingAnimGroup) {
        plugin::Command<plugin::Commands::REMOVE_ANIMATION>(g_pendingAnimGroup);
        g_activeAnimGroup.clear();
    }
    g_pendingAnimGroup[0] = '\0';
    g_pendingAnimRemoveAt = 0;
}

bool IsPedPointerInPool(CPed* ped) {
    if (!ped || !CPools::ms_pPedPool) {
        return false;
    }
    for (CPed* poolPed : CPools::ms_pPedPool) {
        if (poolPed == ped) {
            return true;
        }
    }
    return false;
}

void ScaleAnimNodeIfValid(RpHAnimHierarchy* animHier, RwMatrix* matrices, int boneId, const RwV3d& scale) {
    if (!animHier || !matrices) {
        return;
    }
    const int index = RpHAnimIDGetIndex(animHier, boneId);
    if (index < 0 || index >= animHier->numNodes) {
        return;
    }
    RwMatrixScale(&matrices[index], &scale, rwCOMBINEPRECONCAT);
}

std::vector<int> g_trackedPickupHandles;
bool g_weaponTweaksNeedReapply = true;

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
    return true;
}

void Init() {
    plugin::Events::pedRenderEvent += [](CPed* ped) {
        if (!ped || !ped->m_pRwClump) return;
        auto animHier = GetAnimHierarchyFromSkinClump(ped->m_pRwClump);
        if (!animHier) return;
        auto matrices = RpHAnimHierarchyGetMatrixArray(animHier);
        if (!matrices) return;

        if (MenuState::ThinBodyMode) {
            const RwV3d scale = {0.7f, 0.7f, 0.7f};
            for (int i = 1; i <= 54; ++i) {
                ScaleAnimNodeIfValid(animHier, matrices, i, scale);
            }
        }

        if (MenuState::BigHeadMode) {
            const RwV3d scale = {3.0f, 3.0f, 3.0f};
            for (int i = 5; i <= 8; ++i) {
                ScaleAnimNodeIfValid(animHier, matrices, i, scale);
            }
        }
    };
}

void Process() {
    ProcessDeferredAnimUnload();
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
    // 全局无敌位必须对称：关闭时也要清掉，不能因 player 空指针提前 return
    plugin::patch::Set<bool>(0x96916D, enable, false);
    if (!player || !IsPedPointerInPool(player)) {
        return;
    }
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
    static unsigned int lastTimer = 0;
    static unsigned int lastDmgTimer = 0;
    static float prevHealth = 0.0f;
    static float prevArmour = 0.0f;

    if (!player) return;

    unsigned int timer = CTimer::m_snTimeInMilliseconds;
    
    float currentHealth = player->m_fHealth;
    float currentArmour = player->m_fArmour;

    if (currentHealth < prevHealth || currentArmour < prevArmour) {
        lastDmgTimer = timer;
    }
    
    prevHealth = currentHealth;
    prevArmour = currentArmour;

    if (!enable) return;

    if (timer - lastDmgTimer > 5000 && timer - lastTimer > 1000) {
        if (player->m_fHealth < player->m_fMaxHealth) {
            player->m_fHealth += 2.0f;
            if (player->m_fHealth > player->m_fMaxHealth) player->m_fHealth = player->m_fMaxHealth;
            prevHealth = player->m_fHealth;
        } else if (player->m_fArmour < 100.0f && player->m_fArmour > 0.0f) {
            player->m_fArmour += 2.0f;
            if (player->m_fArmour > 100.0f) player->m_fArmour = 100.0f;
            prevArmour = player->m_fArmour;
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
    static bool wasHardMode = false;
    static float savedMaxHealth = 0.0f;
    static float savedStamina = 0.0f;
    static float savedHealth = 0.0f;
    static float savedArmour = 0.0f;

    if (!player) return;

    if (enable && !wasHardMode) {
        wasHardMode = true;
        plugin::Command<plugin::Commands::GET_FLOAT_STAT>(24, &savedMaxHealth); // STAT_MAX_HEALTH
        plugin::Command<plugin::Commands::GET_FLOAT_STAT>(22, &savedStamina);   // STAT_STAMINA
        savedHealth = player->m_fHealth;
        savedArmour = player->m_fArmour;
        
        player->m_fHealth = 50.0f;
        player->m_fArmour = 0.0f;
        plugin::Command<plugin::Commands::SET_FLOAT_STAT>(24, 350.0f);
        plugin::Command<plugin::Commands::SET_FLOAT_STAT>(22, 0.0f);
    } else if (!enable && wasHardMode) {
        wasHardMode = false;
        plugin::Command<plugin::Commands::SET_FLOAT_STAT>(24, savedMaxHealth);
        plugin::Command<plugin::Commands::SET_FLOAT_STAT>(22, savedStamina);
        player->m_fHealth = savedHealth;
        player->m_fArmour = savedArmour;
    } else if (enable && wasHardMode) {
        if (player->m_fHealth > 50.0f) player->m_fHealth = 50.0f;
        player->m_fArmour = 0.0f;
        plugin::Command<plugin::Commands::SET_FLOAT_STAT>(24, 350.0f);
        plugin::Command<plugin::Commands::SET_FLOAT_STAT>(22, 0.0f);
    }
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

bool SetPlayerCustomSkin(const char* name) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !name || name[0] == '\0') return false;

    CStreaming::RequestSpecialChar(1, name, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(true);
    
    player->SetModelIndex(291);
    CStreaming::SetSpecialCharIsDeletable(291);
    return true;
}

bool SetPlayerStat(int statId, float value) {
    plugin::Command<plugin::Commands::SET_FLOAT_STAT>(statId, value);
    return true;
}

void MaxWeaponSkills() {
    CCheat::WeaponSkillsCheat();
}

void MaxVehicleSkills() {
    CCheat::VehicleSkillsCheat();
}

void ApplyAimSkinChanger() {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !player->m_pPlayerTargettedPed) {
        return;
    }
    const int model = player->m_pPlayerTargettedPed->m_nModelIndex;
    if (model > 0) {
        SetPlayerSkin(static_cast<unsigned int>(model));
    }
}

void ProcessPlayerCheats(CPlayerPed* player) {
    static bool lastNeverWanted = false;
    if (MenuState::NeverWanted) {
        plugin::patch::Set<bool>(0x969171, true, false);
        if (!lastNeverWanted) {
            CCheat::NotWantedCheat();
        }
        if (player) {
            SetWantedLevel(player, 0);
        }
    } else if (lastNeverWanted) {
        plugin::patch::Set<bool>(0x969171, false, false);
    }
    lastNeverWanted = MenuState::NeverWanted;

    if (player) {
        // plugin-sdk SA: ped flags are anonymous bitfields on CPed, not m_nPedFlags
        player->bDontRender = MenuState::InvisiblePlayer ? 1 : 0;
    }

    plugin::patch::Set<bool>(0x96916C, MenuState::MegaJump, false);
    plugin::patch::Set<bool>(0x969173, MenuState::MegaPunch, false);
    plugin::patch::Set<bool>(0x969161, MenuState::CycleJump, false);
    plugin::patch::Set<bool>(0x96916E, MenuState::InfiniteOxygen, false);
    plugin::patch::Set<bool>(0x969174, MenuState::NeverHungry, false);

    static float s_originalSprintSpeed = 0.0f;
    static bool s_hasOriginalSprintSpeed = false;
    if (!s_hasOriginalSprintSpeed) {
        s_originalSprintSpeed = plugin::patch::GetFloat(0x8D2458);
        s_hasOriginalSprintSpeed = true;
    }
    plugin::patch::SetFloat(0x8D2458, MenuState::FastSprint ? 0.1f : s_originalSprintSpeed);

    static CodePatch s_sprintEverywherePatch = MakePatch(0x688610, 2);
    static bool lastSprintEverywhere = false;
    if (lastSprintEverywhere != MenuState::SprintEverywhere) {
        lastSprintEverywhere = MenuState::SprintEverywhere;
        if (MenuState::SprintEverywhere) {
            static const unsigned char kNopSprint[2] = {0x90, 0x90};
            s_sprintEverywherePatch.ApplyBytes(kNopSprint);
        } else {
            s_sprintEverywherePatch.Restore();
        }
    }

    static bool lastDrunk = false;
    if (MenuState::DrunkEffect) {
        plugin::Command<plugin::Commands::SET_PLAYER_DRUNKENNESS>(0, 100);
        lastDrunk = true;
    } else if (lastDrunk) {
        plugin::Command<plugin::Commands::SET_PLAYER_DRUNKENNESS>(0, 0);
        lastDrunk = false;
    }
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
    if (doorIndex < 0) doorIndex = 0;
    if (doorIndex > 5) doorIndex = 5;
    plugin::Command<plugin::Commands::OPEN_CAR_DOOR>(CPools::GetVehicleRef(vehicle), doorIndex);
}

void PopVehicleDoor(CVehicle* vehicle, int doorIndex) {
    if (!vehicle) return;
    if (doorIndex < 0) doorIndex = 0;
    if (doorIndex > 5) doorIndex = 5;
    plugin::Command<plugin::Commands::POP_CAR_DOOR>(CPools::GetVehicleRef(vehicle), doorIndex);
}

void WarpPlayerToVehicleSeat(CVehicle* vehicle, int seatIndex) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !vehicle) return;
    if (seatIndex < 0) seatIndex = 0;
    if (seatIndex > 8) seatIndex = 8;
    const int hplayer = CPools::GetPedRef(player);
    const int hveh = CPools::GetVehicleRef(vehicle);
    if (seatIndex <= 0) {
        plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(hplayer, hveh);
        return;
    }
    plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR_AS_PASSENGER>(hplayer, hveh, seatIndex - 1);
}

void ProcessAutoDrive(CVehicle* vehicle, bool enable, float speed) {
    // SA 实际自动驾驶在 Controllers::Vehicle 中节流下发；此处保留最小降级入口
    CPlayerPed* player = FindPlayerPed();
    if (!enable || !player || !vehicle) {
        return;
    }
    (void)speed;
}

void SetTrafficDensity(float density) {
    plugin::Command<plugin::Commands::SET_CAR_DENSITY_MULTIPLIER>(density);
    plugin::Command<plugin::Commands::SET_PED_DENSITY_MULTIPLIER>(density);
}

void SetFlyingCars(bool enable) {
    plugin::patch::Set<bool>(0x969160, enable, false);
}

void SetVehicleNoDerail(bool enable) {
    // 三处站点分别备份；关闭时整段还原，禁止只写死 7 字节
    static CodePatch s_noDerailA = MakePatch(0x6F8C2A, 4);
    static CodePatch s_noDerailB = MakePatch(0x6F8C2E, 1);
    static CodePatch s_noDerailC = MakePatch(0x6F8C41, 2);
    if (enable) {
        s_noDerailA.EnsureBackup();
        s_noDerailB.EnsureBackup();
        s_noDerailC.EnsureBackup();
        plugin::patch::Set<uint32_t>(0x6F8C2A, 0x00441F0F, true);
        plugin::patch::Set<uint8_t>(0x6F8C2E, 0x00, true);
        plugin::patch::Set<uint16_t>(0x6F8C41, 0xE990, true);
    } else {
        s_noDerailA.Restore();
        s_noDerailB.Restore();
        s_noDerailC.Restore();
    }
}

void SetVehicleFlipNoBurn(bool enable) {
    static CodePatch s_flipA = MakePatch(0x6A776B, 6);
    static CodePatch s_flipB = MakePatch(0x570E7F, 6);
    static const unsigned char kDisableBurn[6] = {0xD8, 0xDD, 0x00, 0x00, 0x00, 0x00};
    if (enable) {
        s_flipA.ApplyBytes(kDisableBurn);
        s_flipB.ApplyBytes(kDisableBurn);
    } else {
        s_flipA.Restore();
        s_flipB.Restore();
    }
}

void ProcessVehicleCheats(CVehicle* vehicle) {
    plugin::patch::Set<bool>(0x969160, MenuState::VehicleFlyingCars, false);
    plugin::patch::Set<bool>(0x969153, MenuState::VehicleBoatFly, false);
    plugin::patch::Set<bool>(0x969152, MenuState::VehicleDriveWater, false);
    plugin::patch::Set<bool>(0x96914C, MenuState::VehiclePerfectHandling, false);
    plugin::patch::Set<bool>(0x969164, MenuState::VehicleTankMode, false);
    plugin::patch::Set<bool>(0x96914E, MenuState::VehicleGreenLights, false);
    plugin::patch::Set<bool>(0x969179, MenuState::VehicleAimDrive, false);
    // InfNitro 必须对称写 false，不能只开不关
    plugin::patch::Set<bool>(0x969165, MenuState::VehicleInfNitro, false);

    CPlayerPed* player = FindPlayerPed();
    static unsigned char s_savedKnockOff = 0;
    static bool s_hasSavedKnockOff = false;
    static CPlayerPed* s_knockOffPlayer = nullptr;
    if (player && IsPedPointerInPool(player)) {
        if (MenuState::VehicleStayOnBike) {
            if (!s_hasSavedKnockOff || s_knockOffPlayer != player) {
                s_savedKnockOff = player->CantBeKnockedOffBike;
                s_knockOffPlayer = player;
                s_hasSavedKnockOff = true;
            }
            player->CantBeKnockedOffBike = 1; // never knocked off
        } else if (s_hasSavedKnockOff && s_knockOffPlayer == player) {
            player->CantBeKnockedOffBike = s_savedKnockOff;
            s_hasSavedKnockOff = false;
            s_knockOffPlayer = nullptr;
        }
    } else {
        s_hasSavedKnockOff = false;
        s_knockOffPlayer = nullptr;
    }

    if (MenuState::VehicleBikeFly && vehicle && player && vehicle->IsDriver(player)) {
        if (vehicle->m_nVehicleSubClass == VEHICLE_BIKE || vehicle->m_nVehicleSubClass == VEHICLE_BMX) {
            const float speed = sqrt(
                vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x +
                vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y +
                vehicle->m_vecMoveSpeed.z * vehicle->m_vecMoveSpeed.z);
            if (speed > 0.0f && CTimer::ms_fTimeStep > 0.0f) {
                vehicle->FlyingControl(3, -9999.9902f, -9999.9902f, -9999.9902f, -9999.9902f);
            }
        }
    }

    static bool lastNoDerail = false;
    if (lastNoDerail != MenuState::VehicleNoDerail) {
        lastNoDerail = MenuState::VehicleNoDerail;
        SetVehicleNoDerail(MenuState::VehicleNoDerail);
    }
    static bool lastFlipNoBurn = false;
    if (lastFlipNoBurn != MenuState::VehicleFlipNoBurn) {
        lastFlipNoBurn = MenuState::VehicleFlipNoBurn;
        SetVehicleFlipNoBurn(MenuState::VehicleFlipNoBurn);
    }
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
    const unsigned int index = static_cast<unsigned int>(LOWORD(FrontEndMenuManager.m_nTargetBlipIndex));
    if (index >= MAX_RADAR_TRACES) return nullptr;
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

void SetElvisEverywhere(bool enable) { plugin::patch::Set<bool>(0x969157, enable, false); }
void SetEveryoneArmed(bool enable) { plugin::patch::Set<bool>(0x969140, enable, false); }
void SetPedsMayhem(bool enable) { plugin::patch::Set<bool>(0x96913E, enable, false); }
void SetPedsAtkRocket(bool enable) { plugin::patch::Set<bool>(0x969158, enable, false); }
void SetPedsRiot(bool enable) { plugin::patch::Set<bool>(0x969175, enable, false); }
void SetSlutMagnet(bool enable) { plugin::patch::Set<bool>(0x96915D, enable, false); }
void SetGangsControl(bool enable) { plugin::patch::Set<bool>(0x96915B, enable, false); }
void SetGangsEverywhere(bool enable) { plugin::patch::Set<bool>(0x96915A, enable, false); }
void SetPedNoProstitutes(bool) {}
void SetPedNastyLimbs(bool) {}
void SetGangWarsActive(bool enable) {
    CGangWars::bGangWarsActive = enable;
    MenuState::GangWarsActive = enable;
}
void StartGangWar(bool offensive) {
    if (offensive) {
        CGangWars::StartOffensiveGangWar();
    } else {
        CGangWars::StartDefensiveGangWar();
    }
    CGangWars::bGangWarsActive = true;
    MenuState::GangWarsActive = true;
}
void EndGangWar() {
    CGangWars::EndGangWar(true);
}
int GetGangZoneDensity(int gangId) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || gangId < 0 || gangId > 9) return 0;
    CVector pos = player->GetPosition();
    CZoneInfo* info = CTheZones::GetZoneInfo(&pos, nullptr);
    return info ? info->m_nGangDensity[gangId] : 0;
}
void SetGangZoneDensity(int gangId, int density) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || gangId < 0 || gangId > 9) return;
    CVector pos = player->GetPosition();
    CZoneInfo* info = CTheZones::GetZoneInfo(&pos, nullptr);
    if (!info) return;
    if (density < 0) density = 0;
    if (density > 127) density = 127;
    info->m_nGangDensity[gangId] = static_cast<int8_t>(density);
    plugin::Command<plugin::Commands::CLEAR_SPECIFIC_ZONES_TO_TRIGGER_GANG_WAR>();
    CGangWars::bGangWarsActive = true;
    MenuState::GangWarsActive = true;
}
unsigned int GetGangMemberModel(unsigned int gangId, unsigned int memberId) {
    if (gangId > 9 || memberId > 2) return 0;
    return static_cast<unsigned int>(CPopulation::m_PedGroups[42 + static_cast<int>(gangId)][memberId]);
}
void SetGangMemberModel(unsigned int gangId, unsigned int memberId, unsigned int model) {
    if (gangId > 9 || memberId > 2) return;
    CPopulation::m_PedGroups[42 + static_cast<int>(gangId)][memberId] = static_cast<short>(model);
}
void ResetGangModels() {
    CPopulation::LoadPedGroups();
}
void SetGangWeapons(unsigned int gangId, int weapon1, int weapon2, int weapon3) {
    if (gangId > 9) return;
    CGangs::SetGangWeapons(gangId, weapon1, weapon2, weapon3);
}

bool PlayPlayerAnimation(const char* group, const char* name, bool loop) {
    return PlayAnimationEx(group, name, loop, false, false);
}

bool PlayAnimationEx(const char* group, const char* name, bool loop, bool secondary, bool onTargetPed) {
    CPlayerPed* player = FindPlayerPed();
    if (!player || !group || !name || group[0] == '\0' || name[0] == '\0') return false;

    CPed* target = player;
    if (onTargetPed && player->m_pPlayerTargettedPed) {
        target = player->m_pPlayerTargettedPed;
        if (!IsPedPointerInPool(target)) {
            return false;
        }
    }
    if (!target || !IsPedPointerInPool(target)) return false;

    const int hped = CPools::GetPedRef(target);
    if (std::strcmp(group, "PED") != 0) {
        RequestAnimGroupKeepAlive(group);
    }
    plugin::Command<plugin::Commands::CLEAR_CHAR_TASKS>(hped);
    if (secondary) {
        plugin::Command<plugin::Commands::TASK_PLAY_ANIM_SECONDARY>(hped, name, group, 4.0f, loop, false, false, false, -1);
    } else {
        plugin::Command<plugin::Commands::TASK_PLAY_ANIM>(hped, name, group, 4.0f, loop, false, false, false, -1);
    }
    if (std::strcmp(group, "PED") != 0) {
        // 禁止同帧 REMOVE；延迟卸载，避免动画任务读到已卸载 IFP
        ScheduleAnimGroupUnload(group, loop ? 60000 : 8000);
    }
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

void ProcessSmokingEffect(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    static DWORD lastSmokeTick = 0;
    const DWORD now = GetTickCount();
    if (now - lastSmokeTick < 200) return;
    lastSmokeTick = now;
    const CVector pos = player->GetPosition();
    int fx = 0;
    plugin::Command<plugin::Commands::CREATE_FX_SYSTEM>("cigarette_smoke", pos.x, pos.y, pos.z + 0.7f, 0, &fx);
    plugin::Command<plugin::Commands::PLAY_AND_KILL_FX_SYSTEM>(fx);
}

void ProcessFliesEffect(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    static DWORD lastFliesTick = 0;
    const DWORD now = GetTickCount();
    if (now - lastFliesTick < 500) return;
    lastFliesTick = now;
    const CVector pos = player->GetPosition();
    const float rx = static_cast<float>((rand() % 200 - 100)) / 100.0f;
    const float ry = static_cast<float>((rand() % 200 - 100)) / 100.0f;
    const float rz = static_cast<float>((rand() % 100)) / 100.0f + 1.0f;
    int fx = 0;
    plugin::Command<plugin::Commands::CREATE_FX_SYSTEM>("insects", pos.x + rx, pos.y + ry, pos.z + rz, 0, &fx);
    plugin::Command<plugin::Commands::PLAY_AND_KILL_FX_SYSTEM>(fx);
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

void StartMission(int missionId) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;

    if (CTheScripts::OnAMissionFlag && player->m_nAreaCode == 0) {
        player->SetWantedLevel(0);
        plugin::Command<plugin::Commands::LOAD_AND_LAUNCH_MISSION_INTERNAL>(missionId);
        Log::Info("任务已强制加载: " + std::to_string(missionId));
    } else {
        Log::Warn("任务加载失败：不在任务中或在室内");
    }
}

void FailMission() {
    if (!CCutsceneMgr::ms_running) {
        plugin::Command<plugin::Commands::FAIL_CURRENT_MISSION>();
        Log::Info("已强制失败当前任务");
    }
}

void SetFightingStyle(int styleIndex) {
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    if (styleIndex < 0) styleIndex = 0;
    if (styleIndex > 4) styleIndex = 4;
    plugin::Command<plugin::Commands::GIVE_MELEE_ATTACK_TO_CHAR>(CPools::GetPedRef(player), styleIndex + 4, 6);
}

void SetWalkingStyle(int styleIndex) {
    static const char* walkStyles[] = {
        "default", "man", "shuffle", "oldman", "gang1", "gang2", "oldfatman",
        "fatman", "jogger", "drunkman", "blindman", "swat", "woman", "shopping", "busywoman",
        "sexywoman", "pro", "oldwoman", "fatwoman", "jogwoman", "oldfatwoman", "skate"
    };
    const int count = static_cast<int>(sizeof(walkStyles) / sizeof(walkStyles[0]));
    if (styleIndex < 0 || styleIndex >= count) return;

    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    const char* style = walkStyles[styleIndex];
    if (std::strcmp(style, "default") == 0) {
        plugin::patch::Set<DWORD>(0x609A4E, 0x4D48689);
        plugin::patch::Set<WORD>(0x609A52, 0);
    } else {
        plugin::patch::Nop(0x609A4E, 6);
        plugin::Command<plugin::Commands::REQUEST_ANIMATION>(style);
        plugin::Command<plugin::Commands::LOAD_ALL_MODELS_NOW>();
        plugin::Command<plugin::Commands::SET_ANIM_GROUP_FOR_CHAR>(CPools::GetPedRef(player), style);
        plugin::Command<plugin::Commands::REMOVE_ANIMATION>(style);
    }
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

void ProcessVisualExtras() {
    static bool lastFullscreen = false;
    static bool lastNoRadarRot = false;
    static bool lastUnfog = false;

    static CodePatch s_fullscreenPatches[] = {
        MakePatch(0x575BF6, 5), MakePatch(0x575C40, 5), MakePatch(0x575C84, 5), MakePatch(0x575CCE, 5),
        MakePatch(0x575D1F, 5), MakePatch(0x575D6F, 5), MakePatch(0x575DC2, 5), MakePatch(0x575E12, 5),
        MakePatch(0x5754EC, 6), MakePatch(0x575537, 6), MakePatch(0x575311, 6), MakePatch(0x575361, 6),
    };
    static CodePatch s_radarRotPatches[] = {
        MakePatch(0x5837FB, 6), MakePatch(0x583805, 6), MakePatch(0x58380D, 6),
        MakePatch(0x5837D6, 6), MakePatch(0x5837D0, 6), MakePatch(0x5837C6, 8),
    };
    static CodePatch s_squareRadarPatch = MakePatch(0x58585C, 4);

    if (lastFullscreen != MenuState::VisualFullscreenMap) {
        lastFullscreen = MenuState::VisualFullscreenMap;
        if (MenuState::VisualFullscreenMap) {
            for (auto& site : s_fullscreenPatches) {
                site.ApplyNop();
            }
        } else {
            for (auto& site : s_fullscreenPatches) {
                site.Restore();
            }
        }
    }

    if (lastNoRadarRot != MenuState::VisualNoRadarRot) {
        lastNoRadarRot = MenuState::VisualNoRadarRot;
        if (MenuState::VisualNoRadarRot) {
            plugin::patch::SetFloat(0xBA8310, 0.0f);
            plugin::patch::SetFloat(0xBA830C, 0.0f);
            plugin::patch::SetFloat(0xBA8308, 1.0f);
            for (auto& site : s_radarRotPatches) {
                site.ApplyNop();
            }
        } else {
            for (auto& site : s_radarRotPatches) {
                site.Restore();
            }
        }
    }

    if (lastUnfog != MenuState::VisualUnfogMap) {
        lastUnfog = MenuState::VisualUnfogMap;
        plugin::patch::SetUChar(0xBA372C, MenuState::VisualUnfogMap ? 0x50 : 0x00);
    }

    plugin::patch::Set<bool>(0xC402B8, MenuState::VisualNightVision, false);
    plugin::patch::Set<bool>(0xC402B9, MenuState::VisualInfrared, false);

    CHud::bScriptDontDisplayAreaName = MenuState::VisualHideAreaNames;
    CHud::bScriptDontDisplayVehicleName = MenuState::VisualHideVehicleNames;
    plugin::Command<plugin::Commands::DISPLAY_ZONE_NAMES>(!MenuState::VisualHideAreaNames);
    plugin::Command<plugin::Commands::DISPLAY_CAR_NAMES>(!MenuState::VisualHideVehicleNames);

    // Square radar: 简化软实现（缩放雷达盘），非完整 LimitRadarPoint 替换
    static bool lastSquare = false;
    if (lastSquare != MenuState::VisualSquareRadar) {
        lastSquare = MenuState::VisualSquareRadar;
        if (MenuState::VisualSquareRadar) {
            static float var = 0.000001f;
            s_squareRadarPatch.EnsureBackup();
            plugin::patch::Set(0x58585C, &var);
        } else {
            s_squareRadarPatch.Restore();
        }
    }
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
    if (!player) return;
    const int model = static_cast<int>(weaponModel);
    if (model <= 0) return;

    // Find weapon type from model ID (SA lacks GetWeaponTypeFromModel)
    eWeaponType weaponType = WEAPONTYPE_UNARMED;
    for (int t = 0; t <= 46; ++t) {
        int checkModel = -1;
        plugin::Command<plugin::Commands::GET_WEAPONTYPE_MODEL>(t, &checkModel);
        if (checkModel == model) {
            weaponType = static_cast<eWeaponType>(t);
            break;
        }
    }
    if (weaponType == WEAPONTYPE_UNARMED) return;

    GiveWeapon(player, static_cast<unsigned int>(weaponType), ammo);
    plugin::Command<plugin::Commands::SET_CURRENT_PLAYER_WEAPON>(0, weaponType);
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
    // 只删菜单创建/追踪的 pickup，禁止遍历全图 aPickUps 清任务道具
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
    for (int i = 0; i < 13; i++) {
        player->m_aWeapons[i].m_nAmmoTotal = 9999;
    }
}

void SetFastReload(CPlayerPed* player, bool enable) {
    if (!player) return;
    plugin::Command<plugin::Commands::SET_PLAYER_FAST_RELOAD>(CPools::GetPedRef(player), enable);
}

void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange, bool rapidFire, bool dualWield, bool moveAim, bool moveFire, bool noSpread, bool customFireRate, float fireRate) {
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
        const float newLen = (oldEnd - oldStart) / rate;
        end = oldStart + newLen;
        fire = oldStart + (oldFire - oldStart) / rate;
        if (fire < start) {
            fire = start;
        }
        if (fire > end) {
            fire = end;
        }
    };

    auto asFloat = [](unsigned int& value) -> float& {
        return *reinterpret_cast<float*>(&value);
    };

    const float effectiveRate = (customFireRate && fireRate > 0.1f) ? fireRate : 1.0f;
    const bool fireRateActive = customFireRate && std::fabs(effectiveRate - 1.0f) >= 0.001f;
    const bool any = hugeDamage || longRange || rapidFire || dualWield || moveAim || moveFire || noSpread || fireRateActive;
    static bool s_wasAny = false;
    static int s_lastType = -1;
    static unsigned char s_lastSkill = 255;
    static unsigned char s_lastMask = 0;
    static int s_lastRateQ = 100; // fireRate * 100

    const unsigned char mask =
        static_cast<unsigned char>((hugeDamage ? 1 : 0) |
                                   (longRange ? 2 : 0) |
                                   (rapidFire ? 4 : 0) |
                                   (dualWield ? 8 : 0) |
                                   (moveAim ? 16 : 0) |
                                   (moveFire ? 32 : 0) |
                                   (noSpread ? 64 : 0) |
                                   (fireRateActive ? 128 : 0));
    const int rateQ = fireRateActive ? static_cast<int>(effectiveRate * 100.0f + 0.5f) : 100;

    if (!any) {
        if (s_wasAny) {
            CWeaponInfo::LoadWeaponData();
            s_wasAny = false;
            s_lastType = -1;
            s_lastSkill = 255;
            s_lastMask = 0;
            s_lastRateQ = 100;
        }
        return;
    }
    if (!player) {
        return;
    }

    CWeapon& weapon = player->m_aWeapons[player->m_nSelectedWepSlot];
    const unsigned char skill = player->GetWeaponSkill(weapon.m_eWeaponType);
    const int type = static_cast<int>(weapon.m_eWeaponType);

    // 仅在槽位/类型/开关组合变化或外部重置后写入，避免每帧污染全局表
    if (s_wasAny && !g_weaponTweaksNeedReapply && s_lastType == type && s_lastSkill == skill && s_lastMask == mask && s_lastRateQ == rateQ) {
        return;
    }

    if (s_wasAny || g_weaponTweaksNeedReapply) {
        CWeaponInfo::LoadWeaponData();
    }

    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType, skill);
    if (!info) {
        return;
    }

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
    if (fireRateActive) {
        // SDK 把部分 anim 时间标成 uint，实际内存是 float
        scaleAnimLoop(info->m_fAnimLoopStart, info->m_fAnimLoopEnd, asFloat(info->m_nAnimLoopFire), effectiveRate);
        scaleAnimLoop(asFloat(info->m_nAnimLoop2Start), asFloat(info->m_nAnimLoop2End), asFloat(info->m_nAnimLoop2Fire), effectiveRate);
    }

    s_wasAny = true;
    s_lastType = type;
    s_lastSkill = skill;
    s_lastMask = mask;
    s_lastRateQ = rateQ;
    g_weaponTweaksNeedReapply = false;
}

void ResetWeaponStats() {
    CWeaponInfo::LoadWeaponData();
    g_weaponTweaksNeedReapply = true;
}

void ProcessWeaponAutoAim(bool enable) {
    static bool s_savedMouse3rd = true;
    static bool s_hasSaved = false;
    static bool s_wasEnabled = false;

    if (!enable) {
        if (s_wasEnabled && s_hasSaved) {
            CCamera::m_bUseMouse3rdPerson = s_savedMouse3rd;
        }
        s_wasEnabled = false;
        s_hasSaved = false;
        return;
    }

    if (!s_wasEnabled) {
        s_savedMouse3rd = CCamera::m_bUseMouse3rdPerson;
        s_hasSaved = true;
        s_wasEnabled = true;
    }

    if (CPad::NewMouseControllerState.x == 0 && CPad::NewMouseControllerState.y == 0) {
        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
            CCamera::m_bUseMouse3rdPerson = false;
        }
    } else {
        CCamera::m_bUseMouse3rdPerson = true;
    }
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

void ProcessSolidWater(CPlayerPed* player, bool enable) {
    static int solidWaterObj = 0;
    auto destroySolidWater = []() {
        if (!solidWaterObj) {
            return;
        }
        if (plugin::Command<plugin::Commands::DOES_OBJECT_EXIST>(solidWaterObj)) {
            plugin::Command<plugin::Commands::DELETE_OBJECT>(solidWaterObj);
        }
        solidWaterObj = 0;
    };

    if (!enable || !player) {
        destroySolidWater();
        return;
    }

    CVector pos = player->GetPosition();
    float waterHeight = 0.0f;
    plugin::Command<plugin::Commands::GET_WATER_HEIGHT_AT_COORDS>(pos.x, pos.y, false, &waterHeight);

    int hplayer = CPools::GetPedRef(player);
    if (!plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_BOAT>(hplayer) && waterHeight != -1000.0f && pos.z > waterHeight) {
        if (solidWaterObj == 0) {
            CStreaming::RequestModel(3095, PRIORITY_REQUEST);
            CStreaming::LoadAllRequestedModels(false);
            plugin::Command<plugin::Commands::CREATE_OBJECT>(3095, pos.x, pos.y, waterHeight, &solidWaterObj);
            if (!solidWaterObj || !plugin::Command<plugin::Commands::DOES_OBJECT_EXIST>(solidWaterObj)) {
                solidWaterObj = 0;
                return;
            }
            plugin::Command<plugin::Commands::SET_OBJECT_VISIBLE>(solidWaterObj, false);
            if (pos.z < waterHeight + 1.0f) {
                player->SetPosn(pos.x, pos.y, waterHeight + 1.0f);
            }
        } else if (!plugin::Command<plugin::Commands::DOES_OBJECT_EXIST>(solidWaterObj)) {
            solidWaterObj = 0;
        } else {
            plugin::Command<plugin::Commands::SET_OBJECT_COORDINATES>(solidWaterObj, pos.x, pos.y, waterHeight);
        }
    } else {
        destroySolidWater();
    }
}

void SetNoWaterPhysics(bool enable) {
    plugin::patch::Set<uint8_t>(0x6C2759, enable ? 1 : 0, true);
}

void SetFreezeTime(bool enable) {
    // 实际时钟冻结由 Controllers::World 用 WorldLockTime||FreezeTime 重断言；此处不写误导日志
    (void)enable;
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
