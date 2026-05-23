#pragma once

namespace MenuState {
    inline bool GodMode = false;
    inline bool InfiniteSprint = false;
    inline bool AutoHeal = false;
    inline bool HardMode = false;
    inline bool RespawnAtDeathPosition = false;
    inline bool KeepStuff = false;
    inline bool FreezeWantedLevel = false;
    inline bool BulletProof = false;
    inline bool CollisionProof = false;
    inline bool ExplosionProof = false;
    inline bool FireProof = false;
    inline bool MeleeProof = false;
    inline int WantedLevel = 0;
    inline float PlayerHealth = 100.0f;
    inline float PlayerArmour = 0.0f;
    inline int PlayerMoney = 0;
    inline bool InfiniteAmmo = false;
    inline bool FastReload = false;
    inline bool HugeWeaponDamage = false;
    inline bool LongWeaponRange = false;
    inline bool RapidFire = false;
    inline bool DualWield = false;
    inline bool MoveAim = false;
    inline bool MoveFire = false;
    inline bool NoSpread = false;
    inline int WeaponAmmo = 9999;
    inline bool VehicleNoDamage = false;
    inline bool VehicleAutoUnflip = false;
    inline bool VehicleHeavy = false;
    inline bool VehicleWatertight = false;
    inline bool VehicleSpeedLock = false;
    inline float VehicleSpeed = 60.0f;
    inline bool VehicleSpawnAsDriver = true;
    inline bool VehicleSpawnAircraftInAir = true;
    inline int VehicleSpawnModel = 0;
    inline bool TeleportInsertCoord = false;
    inline bool TeleportMarker = false;
    inline bool QuickTeleport = false;
    inline bool SpawnUnderwater = false;
    inline float TeleportMapWidth = 6000.0f;
    inline float TeleportMapHeight = 6000.0f;
    inline bool LockWeather = false;
}