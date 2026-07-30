#include "World.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "utils/I18n.h"
#include "CWeather.h"
#include <string>

#ifdef GTASA
#include "freecam_sa.h"
#include "topdowncam_sa.h"
#include "randomcheats_sa.h"
#include "controllers/Player.h"
#endif

namespace {
    bool wasLockingWeather = false;
    int frozenHour = 0;
    int frozenMinute = 0;
    bool hasLockedTime = false;

    int ClampInt(int value, int minValue, int maxValue) {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    struct WeatherSnapshot {
        short oldType = 0;
        short newType = 0;
        short forcedType = 0;
    };

    WeatherSnapshot CaptureWeatherState() {
        WeatherSnapshot state;
        state.oldType = CWeather::OldWeatherType;
        state.newType = CWeather::NewWeatherType;
        state.forcedType = CWeather::ForcedWeatherType;
        return state;
    }

    void RestoreWeatherState(const WeatherSnapshot& state) {
        CWeather::OldWeatherType = state.oldType;
        CWeather::NewWeatherType = state.newType;
        CWeather::ForcedWeatherType = state.forcedType;
    }

    void SetTimePreservingWeather(int hour, int minute) {
        const WeatherSnapshot weather = CaptureWeatherState();
        GameLogic::SetTime(hour, minute);
        RestoreWeatherState(weather);
    }

    GameLogic::PickupOptions GetPickupOptions() {
        MenuState::PickupModelId = ClampInt(MenuState::PickupModelId, 1, 20000);
        MenuState::PickupType = ClampInt(MenuState::PickupType, 0, 255);
        MenuState::PickupQuantity = ClampInt(MenuState::PickupQuantity, 0, 999999);
        MenuState::PickupMoneyPerDay = ClampInt(MenuState::PickupMoneyPerDay, 0, 999999);

        GameLogic::PickupOptions options;
        options.modelId = static_cast<unsigned int>(MenuState::PickupModelId);
        options.type = static_cast<unsigned char>(MenuState::PickupType);
        options.quantity = static_cast<unsigned int>(MenuState::PickupQuantity);
        options.moneyPerDay = static_cast<unsigned int>(MenuState::PickupMoneyPerDay);
        options.empty = MenuState::PickupEmpty;
        return options;
    }
}

namespace Controllers::World {
    void SetWeather(int id, bool lock) {
        ForceWeatherNow(id);
        MenuState::LockWeather = lock;
    }

    void ReleaseWeather() {
        if (wasLockingWeather) {
            CWeather::ReleaseWeather();
            wasLockingWeather = false;
        }
        MenuState::LockWeather = false;
        CaptureWeather();
    }

    bool IsWeatherLocked() {
        return MenuState::LockWeather;
    }

    void CaptureWeather() {
        MenuState::LockedWeatherType = static_cast<int>(CWeather::OldWeatherType);
    }

    void ForceWeatherNow(int id) {
        CWeather::ForceWeatherNow(static_cast<short>(id));
        MenuState::LockedWeatherType = id;
    }

    void Process() {
        if (MenuState::LockWeather) {
            CWeather::ForceWeatherNow(static_cast<short>(MenuState::LockedWeatherType));
            wasLockingWeather = true;
        } else {
            if (wasLockingWeather) {
                CWeather::ReleaseWeather();
                wasLockingWeather = false;
            }
            CaptureWeather();
        }

        if (MenuState::FreezeTime || MenuState::WorldLockTime) {
            if (!hasLockedTime) {
                GameLogic::GetTime(frozenHour, frozenMinute);
                hasLockedTime = true;
            }
            SetTimePreservingWeather(frozenHour, frozenMinute);
        } else {
            hasLockedTime = false;
        }

#ifdef GTASA
        if (MenuState::FreecamEnabled) {
            Freecam.Process();
        }
        if (MenuState::TopDownCamEnabled) {
            TopDownCam.Process();
        }
        if (MenuState::RandomCheatsEnabled) {
            RandomCheats.Process();
        }

        GameLogic::ProcessSolidWater(Controllers::Player::GetPlayer(), MenuState::SolidWater);
#endif
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

    int GetFpsLimit() {
        return GameLogic::GetFpsLimit();
    }

    void SetFpsLimit(int limit) {
        GameLogic::SetFpsLimit(limit);
        Log::Info("FPS 限制已设置为 " + std::to_string(limit));
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
            hasLockedTime = true;
        } else if (!MenuState::WorldLockTime) {
            hasLockedTime = false;
        }
        GameLogic::SetFreezeTime(enable);
        Log::Info(std::string("冻结时间：") + (enable ? "开启" : "关闭"));
    }

    void SetLockTime(bool enable) {
        if (enable) {
            GameLogic::GetTime(frozenHour, frozenMinute);
            hasLockedTime = true;
        } else if (!MenuState::FreezeTime) {
            hasLockedTime = false;
        }
        Log::Info(std::string("锁定当前时间：") + (enable ? "开启" : "关闭"));
    }

    void SetNoWaterPhysics(bool enable) {
        GameLogic::SetNoWaterPhysics(enable);
    }

    void DestroyAllPeds() {
        GameLogic::DestroyAllPeds();
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

    int SpawnPickup(const GameLogic::PickupOptions& options) {
        return GameLogic::SpawnPickupNearPlayer(options);
    }

    int SpawnPickup() {
        const int handle = GameLogic::SpawnPickupNearPlayer(GetPickupOptions());
        if (handle >= 0) {
            MenuState::ShowNotice(I18n::T("world.pickupSpawned"), 2.0);
            Log::Info("pickup 已生成，句柄 " + std::to_string(handle));
        } else {
            MenuState::ShowNotice(I18n::T("world.pickupFailed"), 2.0);
            Log::Warn("pickup 生成失败");
        }
        return handle;
    }

    bool RemoveTrackedPickups() {
        return GameLogic::RemoveTrackedPickups() > 0;
    }

    bool UpdateLastPickup() {
        const bool ok = GameLogic::UpdateLastPickup(GetPickupOptions());
        MenuState::ShowNotice(I18n::T(ok ? "world.pickupUpdated" : "world.noPickupToUpdate"), 2.0);
        Log::Info(std::string("修改最近 pickup：") + (ok ? "成功" : "无可修改对象"));
        return ok;
    }

    bool RemoveLastPickup() {
        const bool ok = GameLogic::RemoveLastPickup();
        MenuState::ShowNotice(I18n::T(ok ? "world.pickupRemoved" : "world.noPickupToRemove"), 2.0);
        Log::Info(std::string("删除最近 pickup：") + (ok ? "成功" : "无可删除对象"));
        return ok;
    }

    void EnableFreecam() {
#ifdef GTASA
        Freecam.Enable();
        Log::Info("Freecam enabled");
#endif
    }

    void DisableFreecam() {
#ifdef GTASA
        Freecam.Disable();
        Log::Info("Freecam disabled");
#endif
    }

    void DisableTopDownCam() {
#ifdef GTASA
        TopDownCam.Disable();
        Log::Info("TopDownCam disabled");
#endif
    }

    void DrawRandomCheatsList() {
#ifdef GTASA
        RandomCheats.DrawList();
#endif
    }
}