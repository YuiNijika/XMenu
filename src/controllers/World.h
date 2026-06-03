#pragma once

namespace Controllers::World {
    void Process();
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
    int GetDaysPassed();
    void SetDaysPassed(int days);
    float GetGravity();
    void SetGravity(float gravity);
    int SpawnPickup();
    bool UpdateLastPickup();
    bool RemoveLastPickup();

    // Freecam
    void EnableFreecam();
    void DisableFreecam();
    void DisableTopDownCam();

    void DrawRandomCheatsList();
}