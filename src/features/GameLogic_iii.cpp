#include "GameLogic.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CStreaming.h"
#include "CVehicle.h"
#include "CAutomobile.h"
#include "CWeaponInfo.h"
#include "CPools.h"
#include "extensions/ScriptCommands.h"
#include "Patch.h"
#include "CTimer.h"
#include "CClock.h"
#include <ctime>

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
    state.nonPlayer = player->bOnlyDamagedByPlayer;
    return state;
}

void SetPlayerProofState(CPlayerPed* player, const ProofState& state) {
    if (!player) return;
    player->bBulletProof = state.bullet;
    player->bCollisionProof = state.collision;
    player->bExplosionProof = state.explosion;
    player->bFireProof = state.fire;
    player->bMeleeProof = state.melee;
    player->bOnlyDamagedByPlayer = state.nonPlayer;
}

void ApplyGodMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    player->bBulletProof = enable;
    player->bCollisionProof = enable;
    player->bExplosionProof = enable;
    player->bFireProof = enable;
    player->bMeleeProof = enable;
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
    return player ? player->m_pWanted->m_nWantedLevel : 0;
}

void ProcessHardMode(CPlayerPed* player, bool enable) {
    if (!enable || !player) return;
    if (player->m_fHealth > 50.0f) player->m_fHealth = 50.0f;
    player->m_fArmour = 0.0f;
}

ProofState GetVehicleProofState(CVehicle* vehicle) {
    ProofState state;
    if (!vehicle) return state;
    state.bullet = vehicle->bBulletProof;
    state.collision = vehicle->bCollisionProof;
    state.explosion = vehicle->bExplosionProof;
    state.fire = vehicle->bFireProof;
    state.melee = vehicle->bMeleeProof;
    state.nonPlayer = vehicle->bOnlyDamagedByPlayer;
    return state;
}

void SetVehicleProofState(CVehicle* vehicle, const ProofState& state) {
    if (!vehicle) return;
    vehicle->bBulletProof = state.bullet;
    vehicle->bCollisionProof = state.collision;
    vehicle->bExplosionProof = state.explosion;
    vehicle->bFireProof = state.fire;
    vehicle->bMeleeProof = state.melee;
    vehicle->bOnlyDamagedByPlayer = state.nonPlayer;
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

void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed) {
    if (!enable || !vehicle) return;
    const float current = vehicle->m_vecMoveSpeed.Magnitude();
    if (current <= 0.001f) return;

    const float target = speed / 50.0f;
    vehicle->m_vecMoveSpeed *= target / current;
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

void TeleportPlayer(CVector pos) {
    CPlayerPed* pPlayer = FindPlayerPed();
    if (!pPlayer) return;
    CVehicle* pVeh = pPlayer->m_pVehicle;

    CStreaming::LoadScene(pos);
    CStreaming::LoadAllRequestedModels(false);

    if (pVeh && pPlayer->m_bInVehicle) {
        pVeh->Teleport(pos);
    } else {
        pPlayer->Teleport(pos);
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
    player->GiveWeapon(WEAPONTYPE_BASEBALLBAT, 1);
    player->GiveWeapon(WEAPONTYPE_COLT45, 9999);
    player->GiveWeapon(WEAPONTYPE_UZI, 9999);
    player->GiveWeapon(WEAPONTYPE_SHOTGUN, 9999);
    player->GiveWeapon(WEAPONTYPE_AK47, 9999);
    player->GiveWeapon(WEAPONTYPE_M16, 9999);
    player->GiveWeapon(WEAPONTYPE_SNIPERRIFLE, 9999);
    player->GiveWeapon(WEAPONTYPE_ROCKETLAUNCHER, 9999);
    player->GiveWeapon(WEAPONTYPE_GRENADE, 9999);
    player->SetCurrentWeapon(WEAPONTYPE_M16);
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

void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange) {
    if (!player || (!hugeDamage && !longRange)) return;

    CWeapon& weapon = player->m_aWeapons[player->m_nSelectedWepSlot];
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weapon.m_eWeaponType);
    if (!info) return;

    if (hugeDamage) {
        info->m_nDamage = 1000;
    }
    if (longRange) {
        info->m_fRange = 1000.0f;
    }
}

void ResetWeaponStats() {
    CWeaponInfo::LoadWeaponData();
}

void SetTime(int hour, int minute) {
    CClock::SetGameClock(hour, minute);
}

void GetTime(int& hour, int& minute) {
    hour = CClock::ms_nGameClockHours;
    minute = CClock::ms_nGameClockMinutes;
}

void SyncTimeWithSystemClock() {
    const std::time_t nowTime = std::time(nullptr);
    const std::tm* now = std::localtime(&nowTime);
    if (!now) return;

    CClock::SetGameClock(now->tm_hour, now->tm_min);
}

void SetGameSpeed(float speed) {
    CTimer::ms_fTimeScale = speed;
}

float GetGameSpeed() {
    return CTimer::ms_fTimeScale;
}

} // namespace GameLogic
