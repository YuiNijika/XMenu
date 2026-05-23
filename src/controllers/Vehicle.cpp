#include "Vehicle.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "plugin.h"
#include "CPlayerPed.h"
#include "CVehicle.h"

namespace {
    CVehicle* savedVehicle = nullptr;
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
            return;
        }

        GameLogic::SetVehicleSpeedLock(vehicle, MenuState::VehicleSpeedLock, MenuState::VehicleSpeed);
    }

    void Repair() {
        GameLogic::RepairVehicle(GetCurrentVehicle());
    }

    void Stop() {
        GameLogic::StopVehicle(GetCurrentVehicle());
    }

    void SetEngine(bool enable) {
        GameLogic::SetVehicleEngine(GetCurrentVehicle(), enable);
    }
}