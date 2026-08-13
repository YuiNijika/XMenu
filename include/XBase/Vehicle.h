#pragma once

#include "Types.h"
#include "ValueTypes.h"

namespace XBase::Vehicle {

struct RuntimeOptions {
    bool noDamage = false;
    bool autoUnflip = false;
    bool heavy = false;
    bool watertight = false;
    bool speedLock = false;
    float speed = 60.0f;
};

struct Colors {
    int primary = 0;
    int secondary = 0;
    int tertiary = 0;
    int quaternary = 0;
};

enum class SpawnFailureReason {
    None,
    SpawnInProgress,
    RateLimited,
    InvalidModel,
    BackendRejected,
};

enum class VehicleEventType {
    SpawnRejected,
    Spawned,
    PreviousVehicleCleaned,
    PreviousVehicleCleanupSkipped,
};

struct SpawnOptions {
    bool asDriver = true;
    bool aircraftInAir = true;
    bool cleanupPrevious = true;
};

struct SpawnPolicy {
    unsigned int windowMs = 3000;
    unsigned int maxSpawns = 2;
};

struct SpawnResult {
    bool success = false;
    VehicleId vehicle;
    SpawnFailureReason failure = SpawnFailureReason::None;
};

struct VehicleEvent {
    VehicleEventType type = VehicleEventType::SpawnRejected;
    SpawnFailureReason reason = SpawnFailureReason::None;
    unsigned int modelId = 0;
};

struct VehicleSnapshot {
    VehicleId id;
    unsigned int modelId = 0;
    float health = 0.0f;
    Colors colors;
    bool lights = false;
    bool locked = false;
    bool visible = false;
    Types::ProofState proofs;
};

VehicleId GetCurrentId();
VehicleSnapshot GetSnapshot();
void SetRuntimeOptions(const RuntimeOptions& options);
bool SetTrafficDensity(float density);
bool SetAutoDriveToWaypoint(bool enable);
void SetSpawnPolicy(const SpawnPolicy& policy);
SpawnPolicy GetSpawnPolicy();
SpawnResult SpawnEx(unsigned int modelId, const SpawnOptions& options);
bool PollEvent(VehicleEvent& event);
void NotifyGameInit();
void Process();
void Shutdown();

void Repair();
void Start();
void Stop();
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
Types::ProofState GetProofState();
void SetProofState(const Types::ProofState& state);
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

void AddUpgrade(unsigned int modelId);
void RemoveUpgrade(unsigned int modelId);
void RemoveAllUpgrades();
int GetUpgrade(int slot);

Colors GetColors();
void SetColors(const Colors& colors);
int  GetPrimaryColor();
int  GetSecondaryColor();
void SetPrimaryColor(int color);
void SetSecondaryColor(int color);
int  GetPaintjob();
bool SetPaintjob(int paintjob);

void OpenDoor(int doorIndex);
void WarpToSeat(int seatIndex);
void PopDoor(int doorIndex);
void ApplySpeedLock(float speed);
void ApplyTargetSpeed(float speed);
void RestoreTargetSpeed();

void BlowUpAll();
bool Spawn(unsigned int modelId, const SpawnOptions& options);
bool Spawn(unsigned int modelId);

} // namespace XBase::Vehicle
