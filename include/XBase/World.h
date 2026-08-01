#pragma once

#include "Types.h"

namespace XBase::World {

void NotifyGameInit();
void Shutdown();
void Process();
int GetWeather();
void SetWeather(int id, bool lock);
void ReleaseWeather();
bool IsWeatherLocked();
void SetTime(int hour, int minute);
void GetTime(int& hour, int& minute);
void SyncTimeWithSystemClock();
void SetGameSpeed(float speed);
float GetGameSpeed();
int GetFpsLimit();
void SetFpsLimit(int limit);
float GetGravity();
void SetGravity(float gravity);

float GetRain();
void SetRain(float rain);
float GetFoggyness();
void SetFoggyness(float fog);
float GetCloudCoverage();
void SetCloudCoverage(float clouds);
float GetWind();
void SetWind(float wind);
float GetSandstorm();
void SetSandstorm(float sandstorm);
float GetExtraSunnyness();
void SetExtraSunnyness(float sun);
float GetWetRoads();
void SetWetRoads(float wet);

void DestroyAllVehicles();
void DestroyAllPeds();

void SetFreezeTime(bool enable);
bool IsTimeFrozen();
void SetLockedTime(bool enable, int hour, int minute);

void SetFasterClock(bool enable);
bool IsFasterClock();
void SetDisableReplay(bool enable);
bool IsReplayDisabled();
void SetDisableCheats(bool enable);
bool AreCheatsDisabled();
void SetForbiddenAreaWanted(bool enable);
bool IsForbiddenAreaWanted();
void SetFreePayNSpray(bool enable);
bool IsFreePayNSpray();
void SetNoWaterPhysics(bool enable);
bool IsNoWaterPhysics();

int GetDaysPassed();
void SetDaysPassed(int days);

int SpawnPickup(const Types::PickupOptions& options);
bool UpdateLastPickup(const Types::PickupOptions& options);
bool RemoveLastPickup();
bool RemoveTrackedPickups();

} // namespace XBase::World
