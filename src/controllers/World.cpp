#include "World.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "CWeather.h"
#include <string>

namespace {
    short lockedWeatherType = 0;
    bool wasLockingWeather = false;
    int frozenHour = 0;
    int frozenMinute = 0;
}

namespace Controllers::World {
    void CaptureWeather() {
        lockedWeatherType = CWeather::OldWeatherType;
    }

    void Process() {
        if (MenuState::LockWeather) {
            CWeather::ForceWeatherNow(lockedWeatherType);
            wasLockingWeather = true;
        } else {
            if (wasLockingWeather) {
                CWeather::ReleaseWeather();
                wasLockingWeather = false;
            }
            CaptureWeather();
        }

        if (MenuState::FreezeTime) {
            GameLogic::SetTime(frozenHour, frozenMinute);
        }
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

    void SetDisableReplay(bool enable) {
        GameLogic::SetDisableReplay(enable);
        Log::Info(std::string("禁用回放：") + (enable ? "开启" : "关闭"));
    }

    void SetDisableCheats(bool enable) {
        GameLogic::SetDisableCheats(enable);
        Log::Info(std::string("禁用作弊码：") + (enable ? "开启" : "关闭"));
    }

    void SetForbiddenAreaWanted(bool enable) {
        GameLogic::SetForbiddenAreaWanted(enable);
        Log::Info(std::string("禁止通缉区域：") + (enable ? "开启" : "关闭"));
    }

    void SetFreePayNSpray(bool enable) {
        GameLogic::SetFreePayNSpray(enable);
        Log::Info(std::string("免费喷漆店：") + (enable ? "开启" : "关闭"));
    }

    void SetFasterClock(bool enable) {
        GameLogic::SetFasterClock(enable);
        Log::Info(std::string("加快时钟：") + (enable ? "开启" : "关闭"));
    }

    void SetFreezeTime(bool enable) {
        if (enable) {
            GameLogic::GetTime(frozenHour, frozenMinute);
        }
        GameLogic::SetFreezeTime(enable);
        Log::Info(std::string("冻结时间：") + (enable ? "开启" : "关闭"));
    }

    int GetDaysPassed() {
        return GameLogic::GetDaysPassed();
    }

    void SetDaysPassed(int days) {
        GameLogic::SetDaysPassed(days);
    }

    float GetGravity() {
        return GameLogic::GetGravity();
    }

    void SetGravity(float gravity) {
        GameLogic::SetGravity(gravity);
    }
}