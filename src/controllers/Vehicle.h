#pragma once
#include <XBase/Types.h>
#include <XBase/ValueTypes.h>

namespace Controllers::Vehicle {
    XBase::VehicleId GetCurrentVehicleId();
    void SyncRuntimeOptions();
    void ProcessHost();
    void Repair();
    void Start();
    void Stop();
    void ApplyAppearance();
    void ApplyCarcols();
    void ResetColors();
    int GetPrimaryColor();
    int GetSecondaryColor();
    void SetPrimaryColor(int color);
    void SetSecondaryColor(int color);
    int GetPaintjob();
    bool SetPaintjob(int paintjob);
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
    XBase::Types::ProofState GetProofState();
    void SetProofState(const XBase::Types::ProofState& state);
    bool GetVisible();
    void SetVisible(bool enable);
    bool TryGetAlwaysSkidMarks(bool& value);
    bool SetAlwaysSkidMarks(bool enable);
    bool TryGetDisableParticles(bool& value);
    bool SetDisableParticles(bool enable);
    bool TryGetDriverTargetable(bool& value);
    bool SetDriverTargetable(bool enable);
    bool TryGetHeatSeekingTargetable(bool& value);
    bool SetHeatSeekingTargetable(bool enable);
    bool TryGetPetrolTankWeakPoint(bool& value);
    bool SetPetrolTankWeakPoint(bool enable);
    bool TryGetSirenOrAlarm(bool& value);
    bool SetSirenOrAlarm(bool enable);
    bool TryGetTakeLessDamage(bool& value);
    bool SetTakeLessDamage(bool enable);
    void BlowUpAll();
    void ApplySpeedLock();
    void ApplyTargetSpeed();
    void RestoreDefaultTargetSpeed();
    bool Spawn(unsigned int modelId);
}