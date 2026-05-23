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
}