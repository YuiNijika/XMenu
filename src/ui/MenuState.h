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
    inline bool NeverWanted = false;
    inline bool InvisiblePlayer = false;
    inline bool MegaJump = false;
    inline bool MegaPunch = false;
    inline bool CycleJump = false;
    inline bool InfiniteOxygen = false;
    inline bool NeverHungry = false;
    inline bool FastSprint = false;
    inline bool DrunkEffect = false;
    inline bool SprintEverywhere = false;
    inline bool AimSkinChanger = false;
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
    inline bool WeaponAutoAim = false;
    inline bool HugeWeaponDamage = false;
    inline bool LongWeaponRange = false;
    inline bool RapidFire = false;
    inline bool DualWield = false;
    inline bool MoveAim = false;
    inline bool MoveFire = false;
    inline bool NoSpread = false;
    inline bool WeaponFireRateEnabled = false;     // 自定义射速倍率
    inline float WeaponFireRate = 2.0f;            // 1.0=原速，越大越快
    inline bool WeaponPedEsp = false;              // 绘制行人包围盒线框
    inline bool WeaponPedColEsp = false;           // 绘制行人碰撞体局部线框
    inline bool WeaponPedSkeleton = false;         // 绘制行人骨骼
    inline bool WeaponVehicleEsp = false;          // 绘制车辆包围盒线框
    inline bool WeaponVehicleColEsp = false;       // 绘制车辆碰撞体局部线框
    inline bool WeaponBulletTrack = false;         // 子弹追踪
    inline bool WeaponBulletThroughWalls = false;  // 子弹穿墙
    inline float WeaponBulletLockRange = 100.0f;   // 追踪锁定最大距离 米
    inline int WeaponBulletMaxTargets = 4;         // 同时锁定目标数（1=单目标，>1 可分配到多个目标）
    inline int WeaponAmmo = 9999;
    inline int WeaponSpawnId = 0;
    inline int WeaponCyclerId = 1;
    inline bool WeaponCyclerEnabled = false;
    inline int WeaponCyclerInputId = 0;
    inline bool WeaponSafeMode = true;
    inline int GuiThemeIndex = 0;
    inline int GuiInteractionMode = 0;
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
    inline int VehicleColorTertiary = 0;
    inline int VehicleColorQuaternary = 0;
    inline int VehiclePaintjob = -1;
    inline int VehicleModId = 1000;
    inline bool VehicleNeon = false;
    inline int VehicleNeonColorR = 255;
    inline int VehicleNeonColorG = 0;
    inline int VehicleNeonColorB = 0;
    inline bool VehicleAutoDrive = false;
    inline float VehicleAutoDriveSpeed = 35.0f;
    inline bool VehicleFlyingCars = false;
    inline bool VehicleBoatFly = false;
    inline bool VehicleBikeFly = false;
    inline bool VehicleStayOnBike = false;
    inline bool VehicleDriveWater = false;
    inline bool VehiclePerfectHandling = false;
    inline bool VehicleTankMode = false;
    inline bool VehicleGreenLights = false;
    inline bool VehicleAimDrive = false;
    inline bool VehicleNoDerail = false;
    inline bool VehicleFlipNoBurn = false;
    inline bool VehicleInfNitro = false;
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
    inline bool ElvisEverywhere = false;
    inline bool EveryoneArmed = false;
    inline bool PedsMayhem = false;
    inline bool PedsAtkRocket = false;
    inline bool PedsRiot = false;
    inline bool SlutMagnet = false;
    inline bool GangsControl = false;
    inline bool GangsEverywhere = false;
    inline char SceneAnimGroup[64] = "PED";
    inline char SceneAnimName[64] = "WALK_player";
    inline bool SceneAnimLoop = false;
    inline bool SceneAnimSecondary = false;
    inline bool SceneAnimOnPed = false;
    inline int SceneFightStyle = 0;
    inline int SceneWalkStyle = 0;
    inline char SceneParticleName[64] = "";
    inline char SceneCutsceneName[64] = "";
    inline char SceneCutsceneInterior[64] = "0";
    inline int SceneMissionIndex = 0;
    inline bool VisualHud = true;
    inline bool VisualRadar = true;
    inline bool VisualFilter = false;
    inline int VisualFilterId = 0;
    inline float VisualTimecycStrength = 1.0f;
    inline bool VisualSquareRadar = false;
    inline bool VisualNoRadarRot = false;
    inline bool VisualFullscreenMap = false;
    inline bool VisualUnfogMap = false;
    inline bool VisualHideAreaNames = false;
    inline bool VisualHideVehicleNames = false;
    inline bool VisualNightVision = false;
    inline bool VisualInfrared = false;
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
    inline bool SolidWater = false;
    inline bool NoWaterPhysics = false;
    inline bool BigHeadMode = false;
    inline bool ThinBodyMode = false;
    inline bool GangWarsActive = false;
    inline bool PedNoProstitutes = false;
    inline bool PedNastyLimbs = false;
    inline int GangSelected = 0;
    inline int GangMemberIndex = 0;
    inline int GangWeaponSlot = 0;
    inline int GangWeaponType = 0;
    inline bool FreeFlyEnabled = false;
    inline float FreeFlySpeed = 1.0f;
    inline bool FreecamEnabled = false;
    inline float FreecamFov = 70.0f;
    inline int FreecamSpeedMul = 1;
    inline bool TopDownCamEnabled = false;
    inline int TopDownCamZoom = 40;
    inline bool RandomCheatsEnabled = false;
    inline int RandomCheatsInterval = 5;
    inline bool RandomCheatsProgressBar = false;
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

    inline bool UseNativeMenu = false;
    inline bool ListMenuMouseInput = true; // 列表模式默认允许鼠标（GTA5 PC 交互）

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