#include "GameLogic.h"
#include "utils/Log.h"
#include "CMenuManager.h"
#include "CRadar.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CStreaming.h"
#include "CVehicle.h"
#include "CAutomobile.h"
#include "CBike.h"
#include "CPools.h"
#include "CModelInfo.h"
#include "CWeaponInfo.h"
#include "CPools.h"
#include "extensions/ScriptCommands.h"
#include "Patch.h"
#include "CTimer.h"
#include "CClock.h"
#include "rw/skeleton.h"
#include <ctime>
#include <string>

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
    if (!vehicle) return;
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
    plugin::Command<plugin::Commands::SET_CAR_FORWARD_SPEED>(CPools::GetVehicleRef(vehicle), speed);
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

bool IsAircraftModel(int model) {
    return CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)
        || model == 417 || model == 425 || model == 447 || model == 460 || model == 476
        || model == 487 || model == 488 || model == 511 || model == 512 || model == 513
        || model == 519 || model == 520 || model == 548 || model == 553 || model == 563
        || model == 577 || model == 592 || model == 593;
}

bool IsValidVehicleModel(unsigned int modelId) {
    const int model = static_cast<int>(modelId);
    return model >= 400 && model <= 611
        && (CModelInfo::IsCarModel(model) || CModelInfo::IsBoatModel(model) || IsAircraftModel(model))
        && !CModelInfo::IsTrainModel(model)
        && !CModelInfo::IsTrailerModel(model);
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

    if (player->m_nAreaCode == 0) {
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
        player->TransformFromObjectSpace(pos, CVector(0.0f, 10.0f, 0.0f));
        plugin::Command<plugin::Commands::CREATE_CAR>(model, pos.x, pos.y, pos.z + 3.0f, &hveh);
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
    plugin::patch::Set<unsigned char>(0x460500, enable ? 0xC3 : 0x80, false);
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

} // namespace GameLogic
