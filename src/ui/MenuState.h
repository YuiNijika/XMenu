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
    inline int WeaponCyclerId = 1;
    inline bool WeaponCyclerEnabled = false;
    inline bool VehicleNoDamage = false;
    inline bool VehicleAutoUnflip = false;
    inline bool VehicleHeavy = false;
    inline bool VehicleWatertight = false;
    inline bool VehicleSpeedLock = false;
    inline float VehicleSpeed = 60.0f;
    inline bool VehicleSpawnAsDriver = true;
    inline bool VehicleSpawnAircraftInAir = true;
    inline bool VehicleCleanupAfterSpawn = true;
    inline int VehicleSpawnModel = 0;
    inline int VehicleColorPrimary = 0;
    inline int VehicleColorSecondary = 0;
    inline int VehiclePaintjob = -1;
    inline int VehicleModId = 1000;
    inline bool VehicleAutoDrive = false;
    inline float VehicleAutoDriveSpeed = 35.0f;
    inline bool VehicleFlyingCars = false;
    inline int VehicleDoorIndex = 0;
    inline int VehicleSeatIndex = 0;
    inline float VehicleTrafficClearRadius = 80.0f;
    inline bool TeleportInsertCoord = false;
    inline bool TeleportMarker = false;
    inline bool QuickTeleport = false;
    inline bool QuickTeleportMapActive = false;
    inline bool SpawnUnderwater = false;
    inline float TeleportForwardDistance = 5.0f;
    inline bool TeleportForwardHold = false;
    inline float TeleportMapWidth = 6000.0f;
    inline float TeleportMapHeight = 6000.0f;
    inline int PedSpawnModel = 7;
    inline int PedSpawnType = 4;
    inline int PedGangType = 0;
    inline int PedWeaponModel = 0;
    inline float PedHealth = 100.0f;
    inline float PedArmour = 0.0f;
    inline bool PedSpawnAsGang = false;
    inline bool PedFreeze = false;
    inline bool PedHostile = false;
    inline bool SmokingEffect = false;
    inline bool FliesEffect = false;
    inline char SceneAnimGroup[64] = "PED";
    inline char SceneAnimName[64] = "WALK_player";
    inline bool SceneAnimLoop = false;
    inline char SceneParticleName[64] = "";
    inline char SceneCutsceneName[64] = "";
    inline int SceneMissionIndex = 0;
    inline bool VisualHud = true;
    inline bool VisualRadar = true;
    inline bool VisualFilter = false;
    inline int VisualFilterId = 0;
    inline float VisualTimecycStrength = 1.0f;
    inline int PlayerSkinModel = 0;
    inline int PlayerClothesTexture = 0;
    inline int PlayerClothesModel = 0;
    inline int PlayerClothesBodyPart = 0;
    inline int PlayerStatId = 0;
    inline float PlayerStatValue = 0.0f;
    inline bool OverlayEnabled = false;
    inline bool OverlayShowPosition = true;
    inline bool OverlayShowVehicle = false;
    inline bool OverlayShowFps = true;
    inline bool OverlayShowPlayer = true;
    inline bool OverlayShowTime = true;
    inline bool OverlayShowWorld = false;
    inline bool OverlayShowDetails = false;
    inline bool OverlayShowFeatures = false;
    inline bool WorldLockTime = false;
    inline bool FreeFlyEnabled = false;
    inline float FreeFlySpeed = 1.0f;
    inline bool CommandWindowEnabled = false;
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
    inline int PickupModelId = 1240;
    inline int PickupType = 3;
    inline int PickupQuantity = 1;
    inline int PickupMoneyPerDay = 0;
    inline bool PickupEmpty = false;
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