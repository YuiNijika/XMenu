#pragma once
#include "features/GameLogic.h"

class CVehicle;

namespace Controllers::Vehicle {
    CVehicle* GetCurrentVehicle();
    void Process();
    void Repair();
    void Start();
    void Stop();
    void ApplyAppearance();
    void OpenDoor();
    void PopDoor();
    void WarpToSeat();
    void SetTrafficDensity(float density);
    void SetFlyingCars(bool enable);
    void SetEngine(bool enable);
    void Unflip();
    void SetHeavy(bool enable);
    void SetWatertight(bool enable);
    float GetHealth();
    void SetHealth(float health);
    bool GetLights();
    void SetLights(bool enable);
    bool GetLocked();
    void SetLocked(bool enable);
    GameLogic::ProofState GetProofState();
    void SetProofState(const GameLogic::ProofState& state);
    bool GetVisible();
    void SetVisible(bool enable);
    bool GetAlwaysSkidMarks();
    void SetAlwaysSkidMarks(bool enable);
    bool GetDisableParticles();
    void SetDisableParticles(bool enable);
    bool GetDriverTargetable();
    void SetDriverTargetable(bool enable);
    bool GetHeatSeekingTargetable();
    void SetHeatSeekingTargetable(bool enable);
    bool GetPetrolTankWeakPoint();
    void SetPetrolTankWeakPoint(bool enable);
    bool GetSirenOrAlarm();
    void SetSirenOrAlarm(bool enable);
    bool GetTakeLessDamage();
    void SetTakeLessDamage(bool enable);
    void BlowUpAll();
    void ApplySpeedLock();
    bool Spawn(unsigned int modelId);
}