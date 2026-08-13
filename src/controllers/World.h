#pragma once

#include <XBase/Types.h>

namespace Controllers::World {
    void ProcessHost();
    void SetWeather(int id, bool lock);
    void ReleaseWeather();
    bool IsWeatherLocked();
    void CaptureWeather();
    void SetTime(int hour, int minute);
    void GetTime(int& hour, int& minute);
    void SyncTimeWithSystemClock();
    void DrawWeatherButtons();
    void SetGameSpeed(float speed);
    float GetGameSpeed();
    int GetFpsLimit();
    void SetFpsLimit(int limit);
    void SetDisableReplay(bool enable);
    void SetDisableCheats(bool enable);
    void SetForbiddenAreaWanted(bool enable);
    void SetFreePayNSpray(bool enable);
    void SetFasterClock(bool enable);
    void SetFreezeTime(bool enable);
    void SetLockTime(bool enable);
    void SetNoWaterPhysics(bool enable);
    void DestroyAllPeds();
    void ForceWeatherNow(int id);
    int GetDaysPassed();
    void SetDaysPassed(int days);
    float GetGravity();
    void SetGravity(float gravity);
    int SpawnPickup(const XBase::Types::PickupOptions& options);
    int SpawnPickup();
    bool RemoveTrackedPickups();
    bool UpdateLastPickup();
    bool RemoveLastPickup();

}