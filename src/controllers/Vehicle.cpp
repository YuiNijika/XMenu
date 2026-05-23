#include "Vehicle.h"
#include "features/GameLogic.h"
#include "utils/Log.h"
#include "ui/MenuState.h"
#include "plugin.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#include <string>

namespace {
    CVehicle* savedVehicle = nullptr;
    CVehicle* effectVehicle = nullptr;
    GameLogic::ProofState savedVehicleProofs;
    bool hasSavedVehicleProofs = false;

    void RestoreVehicleProofs() {
        if (!hasSavedVehicleProofs || !savedVehicle) {
            return;
        }

        GameLogic::SetVehicleProofState(savedVehicle, savedVehicleProofs);
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
            GameLogic::SetVehicleHeavy(effectVehicle, false);
            GameLogic::SetVehicleWatertight(effectVehicle, false);
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
        return player->m_pVehicle;
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

    bool Spawn(unsigned int modelId) {
        GameLogic::SpawnVehicleOptions options;
        options.asDriver = MenuState::VehicleSpawnAsDriver;
        options.aircraftInAir = MenuState::VehicleSpawnAircraftInAir;

        if (!GameLogic::IsValidVehicleModel(modelId)) {
            Log::Warn("Vehicle spawn rejected by controller: " + std::to_string(modelId));
            return false;
        }

        return GameLogic::SpawnVehicle(modelId, options) != nullptr;
    }
}