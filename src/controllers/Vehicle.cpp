#include "Vehicle.h"
#include "features/GameLogic.h"
#include "utils/Log.h"
#include "resources/ResourceData.h"
#include "ui/MenuState.h"
#include "plugin.h"
#include "extensions/ScriptCommands.h"
#include "CPools.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#include "CMessages.h"
#include <windows.h>
#include <string>

namespace {
    CVehicle* savedVehicle = nullptr;
    CVehicle* effectVehicle = nullptr;
    GameLogic::ProofState savedVehicleProofs;
    bool hasSavedVehicleProofs = false;
    bool spawnInProgress = false;
    CVehicle* trackedSpawnedVehicle = nullptr;
    CVehicle* pendingCleanupVehicle = nullptr;
    DWORD pendingCleanupReadyTick = 0;
    DWORD spawnWindowStartTick = 0;
    unsigned int spawnWindowCount = 0;
    DWORD lastLimitMessageTick = 0;
    constexpr DWORD SpawnRateWindowMs = 3000;
    constexpr unsigned int MaxSpawnsPerWindow = 2;
    constexpr DWORD LimitMessageCooldownMs = 1000;

    GameLogic::SpawnVehicleOptions GetCurrentSpawnOptions() {
        GameLogic::SpawnVehicleOptions options;
        options.asDriver = MenuState::VehicleSpawnAsDriver;
        options.aircraftInAir = MenuState::VehicleSpawnAircraftInAir;
        options.cleanupPrevious = MenuState::VehicleCleanupAfterSpawn;
        return options;
    }

    void ShowSpawnLimitMessage() {
        const DWORD now = GetTickCount();
        if (lastLimitMessageTick != 0 && now - lastLimitMessageTick < LimitMessageCooldownMs) {
            return;
        }

        lastLimitMessageTick = now;
        CMessages::AddMessageJumpQ("XMenu: Vehicle spawn is too frequent", 1200, 0);
        MenuState::ShowNotice("载具生成过于频繁，请稍后再试", 2.0);
    }

    bool CanConsumeSpawnQuota(unsigned int modelId) {
        const DWORD now = GetTickCount();
        if (spawnWindowStartTick == 0 || now - spawnWindowStartTick >= SpawnRateWindowMs) {
            spawnWindowStartTick = now;
            spawnWindowCount = 0;
        }

        if (spawnWindowCount >= MaxSpawnsPerWindow) {
            Log::Warn("载具生成被限流：" + std::to_string(SpawnRateWindowMs) + "ms 内最多允许 "
                + std::to_string(MaxSpawnsPerWindow) + " 次，模型 ID " + std::to_string(modelId));
            ShowSpawnLimitMessage();
            return false;
        }

        ++spawnWindowCount;
        return true;
    }

    bool ExecuteSpawnNow(unsigned int modelId, const GameLogic::SpawnVehicleOptions& options);

    bool IsVehicleInPool(CVehicle* vehicle) {
        if (!vehicle) {
            return false;
        }

        for (CVehicle* poolVehicle : CPools::ms_pVehiclePool) {
            if (poolVehicle == vehicle) {
                return true;
            }
        }
        return false;
    }

    bool IsPlayerUsingVehicle(CVehicle* vehicle) {
        if (!vehicle || !IsVehicleInPool(vehicle)) {
            return false;
        }

        CPlayerPed* player = FindPlayerPed();
        if (!player) {
            return false;
        }

        const int hplayer = CPools::GetPedRef(player);
        return plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer) && player->m_pVehicle == vehicle;
    }

    void TrackSpawnedVehicle(CVehicle* vehicle, bool cleanupPrevious) {
        if (!vehicle || !IsVehicleInPool(vehicle)) {
            return;
        }

        if (cleanupPrevious && trackedSpawnedVehicle && trackedSpawnedVehicle != vehicle && IsVehicleInPool(trackedSpawnedVehicle)) {
            if (IsPlayerUsingVehicle(trackedSpawnedVehicle)) {
                Log::Warn("跳过立即清理旧 XMenu 载具：玩家正在使用");
            } else {
                GameLogic::DeleteVehicle(trackedSpawnedVehicle);
                Log::Info("旧 XMenu 载具已立即清理");
            }
        }

        trackedSpawnedVehicle = vehicle;
        Log::Info("已追踪 XMenu 生成载具");
    }

    void ProcessSpawnedVehicleCleanup() {
        if (!MenuState::VehicleCleanupAfterSpawn) {
            pendingCleanupVehicle = nullptr;
            pendingCleanupReadyTick = 0;
            return;
        }

        if (!pendingCleanupVehicle) {
            return;
        }

        if (!IsVehicleInPool(pendingCleanupVehicle)) {
            pendingCleanupVehicle = nullptr;
            pendingCleanupReadyTick = 0;
            return;
        }

        const DWORD now = GetTickCount();
        if (pendingCleanupReadyTick != 0 && now < pendingCleanupReadyTick) {
            return;
        }

        if (IsPlayerUsingVehicle(pendingCleanupVehicle)) {
            Log::Warn("跳过清理旧 XMenu 载具：玩家正在使用");
            pendingCleanupVehicle = nullptr;
            pendingCleanupReadyTick = 0;
            return;
        }

        GameLogic::DeleteVehicle(pendingCleanupVehicle);
        Log::Info("旧 XMenu 载具已清理");
        pendingCleanupVehicle = nullptr;
        pendingCleanupReadyTick = 0;
    }

    bool ExecuteSpawnNow(unsigned int modelId, const GameLogic::SpawnVehicleOptions& options) {
        if (spawnInProgress) {
            Log::Warn("载具生成被拒绝：已有生成流程正在执行，模型 ID " + std::to_string(modelId));
            ShowSpawnLimitMessage();
            return false;
        }

        if (!CanConsumeSpawnQuota(modelId)) {
            return false;
        }

        spawnInProgress = true;
        Log::Info("载具生成开始：模型 ID " + std::to_string(modelId));

        if (!GameLogic::IsValidVehicleModel(modelId)) {
            spawnInProgress = false;
            Log::Error("载具生成失败：底层校验拒绝模型 ID " + std::to_string(modelId));
            return false;
        }

        CVehicle* spawnedVehicle = GameLogic::SpawnVehicle(modelId, options);
        const bool ok = spawnedVehicle != nullptr;
        if (ok) {
            TrackSpawnedVehicle(spawnedVehicle, options.cleanupPrevious);
        }
        spawnInProgress = false;
        return ok;
    }

    void RestoreVehicleProofs() {
        if (!hasSavedVehicleProofs || !savedVehicle) {
            return;
        }

        if (IsVehicleInPool(savedVehicle)) {
            GameLogic::SetVehicleProofState(savedVehicle, savedVehicleProofs);
        }
        savedVehicle = nullptr;
        hasSavedVehicleProofs = false;
    }

    void ProcessNoDamage(CVehicle* vehicle) {
        if (!MenuState::VehicleNoDamage) {
            RestoreVehicleProofs();
            return;
        }

        if (!vehicle) {
            return;
        }

        // 玩家换车很频繁，切到新车前先把上一辆车的防护状态还回去。
        if (!hasSavedVehicleProofs || savedVehicle != vehicle) {
            RestoreVehicleProofs();
            savedVehicleProofs = GameLogic::GetVehicleProofState(vehicle);
            savedVehicle = vehicle;
            hasSavedVehicleProofs = true;
        }

        GameLogic::SetVehicleInvincible(vehicle, true);
    }

    void RestoreVehicleEffectsIfNeeded() {
        if (!effectVehicle) {
            return;
        }

        if (!IsVehicleInPool(effectVehicle)) {
            effectVehicle = nullptr;
            return;
        }

        if (!MenuState::VehicleHeavy) {
            GameLogic::SetVehicleHeavy(effectVehicle, false);
        }
        if (!MenuState::VehicleWatertight) {
            GameLogic::SetVehicleWatertight(effectVehicle, false);
        }
        if (!MenuState::VehicleHeavy && !MenuState::VehicleWatertight) {
            effectVehicle = nullptr;
        }
    }

    void ProcessVehicleEffects(CVehicle* vehicle) {
        if (!vehicle) {
            RestoreVehicleEffectsIfNeeded();
            return;
        }

        if (effectVehicle && effectVehicle != vehicle) {
            if (IsVehicleInPool(effectVehicle)) {
                GameLogic::SetVehicleHeavy(effectVehicle, false);
                GameLogic::SetVehicleWatertight(effectVehicle, false);
            }
            effectVehicle = nullptr;
        }

        effectVehicle = vehicle;
        GameLogic::SetVehicleHeavy(vehicle, MenuState::VehicleHeavy);
        GameLogic::SetVehicleWatertight(vehicle, MenuState::VehicleWatertight);
        RestoreVehicleEffectsIfNeeded();
    }
}

namespace Controllers::Vehicle {
    CVehicle* GetCurrentVehicle() {
        CPlayerPed* player = FindPlayerPed();
        if (!player) {
            return nullptr;
        }

        const int hplayer = CPools::GetPedRef(player);
        if (!plugin::Command<plugin::Commands::IS_CHAR_IN_ANY_CAR>(hplayer)) {
            return nullptr;
        }

        CVehicle* vehicle = player->m_pVehicle;
        return IsVehicleInPool(vehicle) ? vehicle : nullptr;
    }

    void Process() {
        ProcessSpawnedVehicleCleanup();

        CVehicle* vehicle = GetCurrentVehicle();
        ProcessNoDamage(vehicle);

        GameLogic::SetTrafficDensity(MenuState::VehicleTrafficClearRadius / 100.0f);
        GameLogic::SetFlyingCars(MenuState::VehicleFlyingCars);

        if (!vehicle) {
            ProcessVehicleEffects(nullptr);
            return;
        }

        if (MenuState::VehicleAutoUnflip && vehicle->IsUpsideDown()) {
            GameLogic::UnflipVehicle(vehicle);
        }

        GameLogic::SetVehicleSpeedLock(vehicle, MenuState::VehicleSpeedLock, MenuState::VehicleSpeed);
        GameLogic::ProcessAutoDrive(vehicle, MenuState::VehicleAutoDrive, MenuState::VehicleAutoDriveSpeed);
        ProcessVehicleEffects(vehicle);
    }

    void Repair() {
        GameLogic::RepairVehicle(GetCurrentVehicle());
    }

    void Start() {
        GameLogic::SetVehicleForwardSpeed(GetCurrentVehicle(), 40.0f);
    }

    void Stop() {
        GameLogic::StopVehicle(GetCurrentVehicle());
    }

    void ApplyAppearance() {
        GameLogic::VehicleAppearanceOptions options;
        options.primaryColor = MenuState::VehicleColorPrimary;
        options.secondaryColor = MenuState::VehicleColorSecondary;
        options.paintjob = MenuState::VehiclePaintjob;
        options.modId = MenuState::VehicleModId;
        GameLogic::ApplyVehicleAppearance(GetCurrentVehicle(), options);
    }

    void OpenDoor() {
        GameLogic::OpenVehicleDoor(GetCurrentVehicle(), MenuState::VehicleDoorIndex);
    }

    void PopDoor() {
        GameLogic::PopVehicleDoor(GetCurrentVehicle(), MenuState::VehicleDoorIndex);
    }

    void WarpToSeat() {
        GameLogic::WarpPlayerToVehicleSeat(GetCurrentVehicle(), MenuState::VehicleSeatIndex);
    }

    void SetTrafficDensity(float density) {
        GameLogic::SetTrafficDensity(density);
    }

    void SetFlyingCars(bool enable) {
        GameLogic::SetFlyingCars(enable);
    }

    void SetEngine(bool enable) {
        GameLogic::SetVehicleEngine(GetCurrentVehicle(), enable);
    }

    void Unflip() {
        GameLogic::UnflipVehicle(GetCurrentVehicle());
    }

    void SetHeavy(bool enable) {
        GameLogic::SetVehicleHeavy(GetCurrentVehicle(), enable);
    }

    void SetWatertight(bool enable) {
        GameLogic::SetVehicleWatertight(GetCurrentVehicle(), enable);
    }

    float GetHealth() {
        return GameLogic::GetVehicleHealth(GetCurrentVehicle());
    }

    void SetHealth(float health) {
        GameLogic::SetVehicleHealth(GetCurrentVehicle(), health);
    }

    bool GetLights() {
        return GameLogic::GetVehicleLights(GetCurrentVehicle());
    }

    void SetLights(bool enable) {
        GameLogic::SetVehicleLights(GetCurrentVehicle(), enable);
    }

    bool GetLocked() {
        return GameLogic::GetVehicleLocked(GetCurrentVehicle());
    }

    void SetLocked(bool enable) {
        GameLogic::SetVehicleLocked(GetCurrentVehicle(), enable);
    }

    GameLogic::ProofState GetProofState() {
        return GameLogic::GetVehicleProofState(GetCurrentVehicle());
    }

    void SetProofState(const GameLogic::ProofState& state) {
        GameLogic::SetVehicleProofState(GetCurrentVehicle(), state);
    }

    bool GetVisible() {
        return GameLogic::GetVehicleVisible(GetCurrentVehicle());
    }

    void SetVisible(bool enable) {
        GameLogic::SetVehicleVisible(GetCurrentVehicle(), enable);
    }

    bool GetAlwaysSkidMarks() {
        return GameLogic::GetVehicleAlwaysSkidMarks(GetCurrentVehicle());
    }

    void SetAlwaysSkidMarks(bool enable) {
        GameLogic::SetVehicleAlwaysSkidMarks(GetCurrentVehicle(), enable);
    }

    bool GetDisableParticles() {
        return GameLogic::GetVehicleDisableParticles(GetCurrentVehicle());
    }

    void SetDisableParticles(bool enable) {
        GameLogic::SetVehicleDisableParticles(GetCurrentVehicle(), enable);
    }

    bool GetDriverTargetable() {
        return GameLogic::GetVehicleDriverTargetable(GetCurrentVehicle());
    }

    void SetDriverTargetable(bool enable) {
        GameLogic::SetVehicleDriverTargetable(GetCurrentVehicle(), enable);
    }

    bool GetHeatSeekingTargetable() {
        return GameLogic::GetVehicleHeatSeekingTargetable(GetCurrentVehicle());
    }

    void SetHeatSeekingTargetable(bool enable) {
        GameLogic::SetVehicleHeatSeekingTargetable(GetCurrentVehicle(), enable);
    }

    bool GetPetrolTankWeakPoint() {
        return GameLogic::GetVehiclePetrolTankWeakPoint(GetCurrentVehicle());
    }

    void SetPetrolTankWeakPoint(bool enable) {
        GameLogic::SetVehiclePetrolTankWeakPoint(GetCurrentVehicle(), enable);
    }

    bool GetSirenOrAlarm() {
        return GameLogic::GetVehicleSirenOrAlarm(GetCurrentVehicle());
    }

    void SetSirenOrAlarm(bool enable) {
        GameLogic::SetVehicleSirenOrAlarm(GetCurrentVehicle(), enable);
    }

    bool GetTakeLessDamage() {
        return GameLogic::GetVehicleTakeLessDamage(GetCurrentVehicle());
    }

    void SetTakeLessDamage(bool enable) {
        GameLogic::SetVehicleTakeLessDamage(GetCurrentVehicle(), enable);
    }

    void BlowUpAll() {
        GameLogic::BlowUpAllVehicles();
    }

    void ApplySpeedLock() {
        GameLogic::SetVehicleSpeedLock(GetCurrentVehicle(), MenuState::VehicleSpeedLock, MenuState::VehicleSpeed);
    }

    void ApplyTargetSpeed() {
        GameLogic::SetVehicleForwardSpeed(GetCurrentVehicle(), MenuState::VehicleSpeed);
    }

    void RestoreDefaultTargetSpeed() {
        MenuState::VehicleSpeed = 60.0f;
        ApplyTargetSpeed();
        ApplySpeedLock();
    }

    bool Spawn(unsigned int modelId) {
        const GameLogic::SpawnVehicleOptions options = GetCurrentSpawnOptions();
        return ExecuteSpawnNow(modelId, options);
    }
}