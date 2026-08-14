#include "Vehicle.h"
#include <XBase/Types.h>
#include <XBase/Vehicle.h>
#include <XBase/VehicleEffects.h>
#include <XBase/Capabilities.h>
#include <XBase/Cheats.h>
#include <XBase/Host.h>
#include "integration/XBaseBridge.h"
#include "utils/Log.h"
#include "resources/ResourceData.h"
#include "ui/MenuState.h"
#include <string>

namespace {
    XBase::Vehicle::SpawnOptions GetCurrentSpawnOptions() {
        XBase::Vehicle::SpawnOptions options;
        options.asDriver = MenuState::VehicleSpawnAsDriver;
        options.aircraftInAir = MenuState::VehicleSpawnAircraftInAir;
        options.cleanupPrevious = MenuState::VehicleCleanupAfterSpawn;
        return options;
    }

    void ReportVehicleEvents() {
        XBase::Vehicle::VehicleEvent event;
        while (XBase::Vehicle::PollEvent(event)) {
            if (event.type == XBase::Vehicle::VehicleEventType::SpawnRejected) {
                Log::Warn("XBase Vehicle spawn rejected: model " + std::to_string(event.modelId));
                if (event.reason == XBase::Vehicle::SpawnFailureReason::RateLimited) {
                    XBase::Host::ShowMessage("XBase: Vehicle spawn is too frequent");
                    MenuState::ShowNotice("载具生成过于频繁，请稍后再试", 2.0);
                }
            } else if (event.type == XBase::Vehicle::VehicleEventType::PreviousVehicleCleaned) {
                Log::Info("XBase Vehicle previous vehicle cleaned");
            } else if (event.type == XBase::Vehicle::VehicleEventType::PreviousVehicleCleanupSkipped) {
                Log::Warn("XBase Vehicle previous vehicle cleanup skipped");
            }
        }
    }

    bool ExecuteSpawnNow(unsigned int modelId, const XBase::Vehicle::SpawnOptions& options) {
        XBase::Vehicle::SpawnPolicy policy;
        XBase::Vehicle::SetSpawnPolicy(policy);

        const XBase::Vehicle::SpawnResult result =
            XBase::Vehicle::SpawnEx(modelId, options);
        ReportVehicleEvents();
        if (!result.success) {
            Log::Warn("XBase Vehicle spawn failed: model " + std::to_string(modelId));
        }
        return result.success;
    }

}

namespace Controllers::Vehicle {
    XBase::VehicleId GetCurrentVehicleId() {
        return XBase::Vehicle::GetCurrentId();
    }

    void SyncRuntimeOptions() {
        XBase::Vehicle::RuntimeOptions runtimeOptions;
        runtimeOptions.noDamage = MenuState::VehicleNoDamage;
        runtimeOptions.autoUnflip = MenuState::VehicleAutoUnflip;
        runtimeOptions.heavy = MenuState::VehicleHeavy;
        runtimeOptions.watertight = MenuState::VehicleWatertight;
        runtimeOptions.speedLock = MenuState::VehicleSpeedLock;
        runtimeOptions.speed = MenuState::VehicleSpeed;
        XBase::Vehicle::SetRuntimeOptions(runtimeOptions);
    }

    void ProcessHost() {
        ReportVehicleEvents();

        const bool hasVehicle = static_cast<bool>(GetCurrentVehicleId());
        SyncRuntimeOptions();

        const bool canTrafficDensity =
            XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleTrafficDensity);
        if (canTrafficDensity) {
            XBase::Vehicle::SetTrafficDensity(MenuState::VehicleTrafficClearRadius / 100.0f);
        } else {
            MenuState::VehicleTrafficClearRadius = 100.0f;
        }

        const bool canAutoDrive =
            XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleAutoDrive);
        if (!canAutoDrive) {
            MenuState::VehicleAutoDrive = false;
        }

        const bool canCheats =
            XBaseBridge::HasCapability(XBase::Capability::Cheats);
        if (canCheats) {
            XBase::Cheats::FlyingCars(MenuState::VehicleFlyingCars);
            XBase::Cheats::PerfectHandling(MenuState::VehiclePerfectHandling);
            XBase::Cheats::GreenLights(MenuState::VehicleGreenLights);
            XBase::Cheats::AllCarsHaveNitro(MenuState::VehicleInfNitro);
            XBase::Cheats::BoatFly(MenuState::VehicleBoatFly);
            XBase::Cheats::BikeFly(MenuState::VehicleBikeFly);
            XBase::Cheats::StayOnBike(MenuState::VehicleStayOnBike);
            XBase::Cheats::DriveWater(MenuState::VehicleDriveWater);
            XBase::Cheats::TankMode(MenuState::VehicleTankMode);
            XBase::Cheats::AimDrive(MenuState::VehicleAimDrive);
            XBase::Cheats::NoDerail(MenuState::VehicleNoDerail);
            XBase::Cheats::FlipNoBurn(MenuState::VehicleFlipNoBurn);
        } else {
            MenuState::VehicleFlyingCars = false;
            MenuState::VehiclePerfectHandling = false;
            MenuState::VehicleGreenLights = false;
            MenuState::VehicleInfNitro = false;
            MenuState::VehicleBoatFly = false;
            MenuState::VehicleBikeFly = false;
            MenuState::VehicleStayOnBike = false;
            MenuState::VehicleDriveWater = false;
            MenuState::VehicleTankMode = false;
            MenuState::VehicleAimDrive = false;
            MenuState::VehicleNoDerail = false;
            MenuState::VehicleFlipNoBurn = false;
        }

        if (!hasVehicle) {
            return;
        }

        /*
         * 产品配置在宿主同步；车辆持续状态、自动驾驶与 SA 特效由 XBase 领域持有。
         */
#ifdef GTASA
        XBase::VehicleEffects::NeonSettings neonSettings;
        neonSettings.enabled = MenuState::VehicleNeon;
        neonSettings.red = MenuState::VehicleNeonColorR;
        neonSettings.green = MenuState::VehicleNeonColorG;
        neonSettings.blue = MenuState::VehicleNeonColorB;
        XBase::VehicleEffects::ApplyCurrentNeon(neonSettings);

        if (canAutoDrive) {
            XBase::Vehicle::SetAutoDriveToWaypoint(MenuState::VehicleAutoDrive);
        }
#else
        (void)canAutoDrive;
#endif
    }

    void Repair() {
        XBase::Vehicle::Repair();
    }

    void Start() {
        XBase::Vehicle::Start();
    }

    void Stop() {
        XBase::Vehicle::Stop();
    }

    void ApplyAppearance() {
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleColors)) {
            return;
        }

        XBase::Vehicle::Colors colors;
        colors.primary = MenuState::VehicleColorPrimary;
        colors.secondary = MenuState::VehicleColorSecondary;
        colors.tertiary = MenuState::VehicleColorTertiary;
        colors.quaternary = MenuState::VehicleColorQuaternary;
        XBase::Vehicle::SetColors(colors);
        if (XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePaintjob) &&
            MenuState::VehiclePaintjob >= 0) {
            XBase::Vehicle::SetPaintjob(MenuState::VehiclePaintjob);
        }
        if (XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleUpgrades) &&
            MenuState::VehicleModId > 0) {
            XBase::Vehicle::AddUpgrade(static_cast<unsigned int>(MenuState::VehicleModId));
        }
    }

    void ApplyCarcols() {
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleColors)) return;
        XBase::Vehicle::Colors colors;
        colors.primary = MenuState::VehicleColorPrimary;
        colors.secondary = MenuState::VehicleColorSecondary;
        colors.tertiary = MenuState::VehicleColorTertiary;
        colors.quaternary = MenuState::VehicleColorQuaternary;
        XBase::Vehicle::SetColors(colors);
    }

    void ResetColors() {
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleColors)) return;
        XBase::Vehicle::SetColors(XBase::Vehicle::Colors{});
    }

    int GetPrimaryColor() {
        return XBase::Vehicle::GetPrimaryColor();
    }

    int GetSecondaryColor() {
        return XBase::Vehicle::GetSecondaryColor();
    }

    void SetPrimaryColor(int color) {
        XBase::Vehicle::SetPrimaryColor(color);
    }

    void SetSecondaryColor(int color) {
        XBase::Vehicle::SetSecondaryColor(color);
    }

    int GetPaintjob() {
        return MenuState::VehiclePaintjob;
    }

    bool SetPaintjob(int paintjob) {
        if (paintjob < -1 || paintjob > 2) return false;
        MenuState::VehiclePaintjob = paintjob;
        ApplyAppearance();
        return static_cast<bool>(GetCurrentVehicleId());
    }

    void OpenDoor() {
        XBase::Vehicle::OpenDoor(MenuState::VehicleDoorIndex);
    }

    void PopDoor() {
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePopDoors)) return;
        XBase::Vehicle::PopDoor(MenuState::VehicleDoorIndex);
    }

    void WarpToSeat() {
        XBase::Vehicle::WarpToSeat(MenuState::VehicleSeatIndex);
    }

    void SetTrafficDensity(float density) {
        XBase::Vehicle::SetTrafficDensity(density);
    }

    void SetFlyingCars(bool enable) {
        XBase::Cheats::FlyingCars(enable);
    }

    void SetEngine(bool enable) {
        XBase::Vehicle::SetEngine(enable);
    }

    void Unflip() {
        XBase::Vehicle::Unflip();
    }

    void SetHeavy(bool enable) {
        XBase::Vehicle::SetHeavy(enable);
    }

    void SetWatertight(bool enable) {
        XBase::Vehicle::SetWatertight(enable);
    }

    float GetHealth() {
        return XBase::Vehicle::GetHealth();
    }

    void SetHealth(float health) {
        XBase::Vehicle::SetHealth(health);
    }

    bool GetLights() {
        return XBase::Vehicle::GetLights();
    }

    void SetLights(bool enable) {
        XBase::Vehicle::SetLights(enable);
    }

    bool GetLocked() {
        return XBase::Vehicle::GetLocked();
    }

    void SetLocked(bool enable) {
        XBase::Vehicle::SetLocked(enable);
    }

    XBase::Types::ProofState GetProofState() {
        return XBase::Vehicle::GetProofState();
    }

    void SetProofState(const XBase::Types::ProofState& state) {
        XBase::Vehicle::SetProofState(state);
    }

    bool GetVisible() {
        return XBase::Vehicle::GetVisible();
    }

    void SetVisible(bool enable) {
        XBase::Vehicle::SetVisible(enable);
    }

    bool TryGetAlwaysSkidMarks(bool& value) {
        return XBase::Vehicle::TryGetAlwaysSkidMarks(value);
    }

    bool SetAlwaysSkidMarks(bool enable) {
        return XBase::Vehicle::SetAlwaysSkidMarks(enable);
    }

    bool TryGetDisableParticles(bool& value) {
        return XBase::Vehicle::TryGetDisableParticles(value);
    }

    bool SetDisableParticles(bool enable) {
        return XBase::Vehicle::SetDisableParticles(enable);
    }

    bool TryGetDriverTargetable(bool& value) {
        return XBase::Vehicle::TryGetDriverTargetable(value);
    }

    bool SetDriverTargetable(bool enable) {
        return XBase::Vehicle::SetDriverTargetable(enable);
    }

    bool TryGetHeatSeekingTargetable(bool& value) {
        return XBase::Vehicle::TryGetHeatSeekingTargetable(value);
    }

    bool SetHeatSeekingTargetable(bool enable) {
        return XBase::Vehicle::SetHeatSeekingTargetable(enable);
    }

    bool TryGetPetrolTankWeakPoint(bool& value) {
        return XBase::Vehicle::TryGetPetrolTankWeakPoint(value);
    }

    bool SetPetrolTankWeakPoint(bool enable) {
        return XBase::Vehicle::SetPetrolTankWeakPoint(enable);
    }

    bool TryGetSirenOrAlarm(bool& value) {
        return XBase::Vehicle::TryGetSirenOrAlarm(value);
    }

    bool SetSirenOrAlarm(bool enable) {
        return XBase::Vehicle::SetSirenOrAlarm(enable);
    }

    bool TryGetTakeLessDamage(bool& value) {
        return XBase::Vehicle::TryGetTakeLessDamage(value);
    }

    bool SetTakeLessDamage(bool enable) {
        return XBase::Vehicle::SetTakeLessDamage(enable);
    }

    void BlowUpAll() {
        XBase::Vehicle::BlowUpAll();
    }

    void ApplySpeedLock() {
        XBase::Vehicle::ApplySpeedLock(
            MenuState::VehicleSpeedLock ? MenuState::VehicleSpeed : 0.0f);
    }

    void ApplyTargetSpeed() {
        XBase::Vehicle::ApplyTargetSpeed(MenuState::VehicleSpeed);
    }

    void RestoreDefaultTargetSpeed() {
        MenuState::VehicleSpeed = 60.0f;
        XBase::Vehicle::RestoreTargetSpeed();
        ApplySpeedLock();
    }

    bool Spawn(unsigned int modelId) {
        const XBase::Vehicle::SpawnOptions options = GetCurrentSpawnOptions();
        return ExecuteSpawnNow(modelId, options);
    }
}