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
#include <windows.h>
#include <string>

namespace {
    CVehicle* savedVehicle = nullptr;
    CVehicle* effectVehicle = nullptr;
    GameLogic::ProofState savedVehicleProofs;
    bool hasSavedVehicleProofs = false;
    bool spawnInProgress = false;
    DWORD lastSpawnTick = 0;
    constexpr DWORD SpawnCooldownMs = 650;

    bool CanStartSpawn(unsigned int modelId) {
        const DWORD now = GetTickCount();
        if (spawnInProgress) {
            Log::Warn("载具生成被跳过：已有生成流程正在执行，模型 ID " + std::to_string(modelId));
            return false;
        }

        if (lastSpawnTick != 0 && now - lastSpawnTick < SpawnCooldownMs) {
            Log::Warn("载具生成被节流：请求过于频繁，模型 ID " + std::to_string(modelId));
            return false;
        }

        spawnInProgress = true;
        lastSpawnTick = now;
        return true;
    }

    void FinishSpawn() {
        spawnInProgress = false;
    }

    struct SpawnGuard {
        ~SpawnGuard() {
            FinishSpawn();
        }
    };

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
        CVehicle* vehicle = GetCurrentVehicle();
        ProcessNoDamage(vehicle);

        if (!vehicle) {
            ProcessVehicleEffects(nullptr);
            return;
        }

        if (MenuState::VehicleAutoUnflip && vehicle->IsUpsideDown()) {
            GameLogic::UnflipVehicle(vehicle);
        }

        GameLogic::SetVehicleSpeedLock(vehicle, MenuState::VehicleSpeedLock, MenuState::VehicleSpeed);
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

    bool Spawn(unsigned int modelId) {
        if (!CanStartSpawn(modelId)) {
            return false;
        }
        SpawnGuard spawnGuard;

        GameLogic::SpawnVehicleOptions options;
        options.asDriver = MenuState::VehicleSpawnAsDriver;
        options.aircraftInAir = MenuState::VehicleSpawnAircraftInAir;

        if (!GameLogic::IsValidVehicleModel(modelId)) {
            Log::Error("载具生成失败：底层校验拒绝模型 ID " + std::to_string(modelId));
            return false;
        }

        return GameLogic::SpawnVehicle(modelId, options) != nullptr;
    }
}