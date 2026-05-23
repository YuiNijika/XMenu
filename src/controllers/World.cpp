#include "World.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "CWeather.h"

namespace {
    short lockedWeatherType = 0;
    bool wasLockingWeather = false;
}

namespace Controllers::World {
    void CaptureWeather() {
        lockedWeatherType = CWeather::OldWeatherType;
    }

    void Process() {
        if (MenuState::LockWeather) {
            CWeather::ForceWeatherNow(lockedWeatherType);
            wasLockingWeather = true;
            return;
        }

        if (wasLockingWeather) {
            CWeather::ReleaseWeather();
            wasLockingWeather = false;
        }

        CaptureWeather();
    }

    void SetTime(int hour, int minute) {
        GameLogic::SetTime(hour, minute);
    }

    void GetTime(int& hour, int& minute) {
        GameLogic::GetTime(hour, minute);
    }

    void SyncTimeWithSystemClock() {
        GameLogic::SyncTimeWithSystemClock();
    }

    void SetGameSpeed(float speed) {
        GameLogic::SetGameSpeed(speed);
    }

    float GetGameSpeed() {
        return GameLogic::GetGameSpeed();
    }
}