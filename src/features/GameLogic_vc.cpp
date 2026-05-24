#include "GameLogic.h"
#include "utils/Log.h"
#include "resources/ResourceData.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CStreaming.h"
#include "CVehicle.h"
#include "CAutomobile.h"
#include "CModelInfo.h"
#include "CWeaponInfo.h"
#include "CPools.h"
#include "extensions/ScriptCommands.h"
#include "Patch.h"
#include "CTimer.h"
#include "CClock.h"
#include "rw/skeleton.h"
#include <cstdint>
#include <windows.h>
#include <string>
#include <ctime>
#include <string>

namespace {
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
    if (!vehicle) return;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    vehicle->GetOrientation(x, y, z);
    vehicle->SetOrientation(x, y + 135.0f, z);
    StopVehicle(vehicle);
}

void SetVehicleForwardSpeed(CVehicle* vehicle, float speed) {
    if (!vehicle) return;
    plugin::Command<plugin::Commands::SET_CAR_FORWARD_SPEED>(CPools::GetVehicleRef(vehicle), speed);
}

void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed) {
    if (!enable || !vehicle) return;
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
    if (model == 217) {
        Log::Warn("VC 载具生成被拒绝：Maverick(217) 会导致游戏崩溃，请使用 VCN Maverick(218)");
        return false;
    }

    return model >= 130 && model <= 236 && CModelInfo::IsVehicleModel(model);
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

    if (options.asDriver && plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer)) {
        CVehicle* currentVehicle = player->m_pVehicle;
        if (currentVehicle) {
            const int hveh = CPools::GetVehicleRef(currentVehicle);
            pos = currentVehicle->GetPosition();
            plugin::Command<plugin::Commands::GET_CAR_SPEED>(hveh, &speed);
            plugin::Command<plugin::Commands::WARP_CHAR_FROM_CAR_TO_COORD>(hplayer, pos.x, pos.y, pos.z);
            plugin::Command<plugin::Commands::DELETE_CAR>(hveh);
        }
    }

    if (interior == 0) {
        if (options.aircraftInAir && IsAircraftModel(model)) {
            pos.z = 400.0f;
        } else {
            pos.z -= 5.0f;
        }
    }

    CStreaming::RequestModel(model, PRIORITY_REQUEST);
    CStreaming::LoadAllRequestedModels(false);

    int hveh = 0;
    if (options.asDriver) {
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + 4.0f, &hveh);
    } else {
        pos = player->TransformFromObjectSpace(CVector(0.0f, 10.0f, 0.0f));
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + 4.0f, &hveh);
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

    plugin::Command<plugin::Commands::MARK_CAR_AS_NO_LONGER_NEEDED>(hveh);
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
        for (int i = 0; i < 37; ++i) {
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
    const unsigned int models[] = { 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 287, 288, 279, 278, 280, 281, 282, 274, 285, 286, 283, 284, 290, 289, 258, 275, 276, 277, 291, 292, 293 };
    for (const unsigned int model : models) {
        GiveWeaponModel(player, model, 99999);
    }
}

void GiveWeapon(CPlayerPed* player, unsigned int weaponType, unsigned int ammo) {
    if (!player) return;
    GiveWeaponModel(player, static_cast<unsigned int>(GetWeaponModel(static_cast<eWeaponType>(weaponType))), ammo);
}

void GiveWeaponModel(CPlayerPed* player, unsigned int weaponModel, unsigned int ammo) {
    if (!player) return;
    const int hplayer = CPools::GetPedRef(player);
    const int model = static_cast<int>(weaponModel);
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

void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange, bool rapidFire, bool dualWield, bool moveAim, bool moveFire, bool noSpread) {
    if (!player || (!hugeDamage && !longRange && !noSpread)) return;

    CWeapon& weapon = player->m_aWeapons[player->m_nCurrentWeapon];
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType);
    if (!info) return;

    if (hugeDamage) {
        info->m_nDamage = 1000;
    }
    if (longRange) {
        info->m_fRange = 1000.0f;
    }
    if (noSpread) {
        info->m_fSpread = 0.0f;
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
    if (enable) {
        plugin::patch::Nop(0x602BD8, 5, false);
        plugin::patch::Nop(0x602BE7, 5, false);
        return;
    }

    plugin::patch::SetRaw(0x602BD8, (void*)"\x88\xD8\x89\xF1\x50", 5, false);
    plugin::patch::SetRaw(0x602BE7, (void*)"\xE8\x34\x91\xEA\xFF", 5, false);
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

} // namespace GameLogic
