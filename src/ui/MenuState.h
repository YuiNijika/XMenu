#pragma once
#include <chrono>
#include <cstdio>

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
    inline int WeaponSpawnId = 0;
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
    inline bool DisableReplay = false;
    inline bool DisableCheats = false;
    inline bool ForbiddenAreaWanted = false;
    inline bool FreePayNSpray = false;
    inline bool FasterClock = false;
    inline bool FreezeTime = false;
    inline int DaysPassed = 0;
    inline float Gravity = 0.008f;
    inline int FpsLimit = 30;
    inline char NoticeText[256] = "";
    inline double NoticeExpireTime = 0.0;

    inline void ShowNotice(const char* text, double durationSeconds = 2.0) {
        std::snprintf(NoticeText, sizeof(NoticeText), "%s", text ? text : "");
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const double nowSeconds = std::chrono::duration<double>(now).count();
        NoticeExpireTime = nowSeconds + (durationSeconds > 0.0 ? durationSeconds : 0.0);
    }

    inline void ClearNotice() {
        NoticeText[0] = '\0';
        NoticeExpireTime = 0.0;
    }

    inline bool HasNotice() {
        if (NoticeText[0] == '\0') {
            return false;
        }

        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const double nowSeconds = std::chrono::duration<double>(now).count();
        if (nowSeconds >= NoticeExpireTime) {
            ClearNotice();
            return false;
        }
        return true;
    }
}