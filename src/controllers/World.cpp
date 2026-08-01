#include "World.h"
#include <XBase/World.h>
#include <XBase/Cheats.h>
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "utils/I18n.h"
#include <string>

#ifdef GTASA
#include "controllers/Player.h"
#endif

namespace {
    int ClampInt(int value, int minValue, int maxValue) {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    void SetTimePreservingWeather(int hour, int minute) {
        XBase::World::SetTime(hour, minute);
    }

    XBase::Types::PickupOptions GetPickupOptions() {
        MenuState::PickupModelId = ClampInt(MenuState::PickupModelId, 1, 20000);
        MenuState::PickupType = ClampInt(MenuState::PickupType, 0, 255);
        MenuState::PickupQuantity = ClampInt(MenuState::PickupQuantity, 0, 999999);
        MenuState::PickupMoneyPerDay = ClampInt(MenuState::PickupMoneyPerDay, 0, 999999);

        XBase::Types::PickupOptions options;
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
        XBase::World::SetWeather(id, lock);
        MenuState::LockWeather = lock;
    }

    void ReleaseWeather() {
        XBase::World::ReleaseWeather();
        MenuState::LockWeather = false;
        CaptureWeather();
    }

    bool IsWeatherLocked() {
        return XBase::World::IsWeatherLocked();
    }

    void CaptureWeather() {
        MenuState::LockedWeatherType = XBase::World::GetWeather();
    }

    void ForceWeatherNow(int id) {
        XBase::World::SetWeather(id, MenuState::LockWeather);
        MenuState::LockedWeatherType = id;
    }

    void SetTime(int hour, int minute) {
        XBase::World::SetTime(hour, minute);
    }

    void GetTime(int& hour, int& minute) {
        XBase::World::GetTime(hour, minute);
    }

    void SyncTimeWithSystemClock() {
        XBase::World::SyncTimeWithSystemClock();
    }

    void SetGameSpeed(float speed) {
        XBase::World::SetGameSpeed(speed);
    }

    float GetGameSpeed() {
        return XBase::World::GetGameSpeed();
    }

    int GetFpsLimit() {
        return XBase::World::GetFpsLimit();
    }

    void SetFpsLimit(int limit) {
        XBase::World::SetFpsLimit(limit);
        Log::Info("FPS 限制已设置为 " + std::to_string(limit));
    }

    void SetDisableReplay(bool enable) {
        XBase::World::SetDisableReplay(enable);
        Log::Info(std::string("禁用回放：") + (enable ? "开启" : "关闭"));
    }

    void SetDisableCheats(bool enable) {
        XBase::World::SetDisableCheats(enable);
        Log::Info(std::string("禁用作弊码：") + (enable ? "开启" : "关闭"));
    }

    void SetForbiddenAreaWanted(bool enable) {
        XBase::World::SetForbiddenAreaWanted(enable);
        Log::Info(std::string("禁止通缉区域：") + (enable ? "开启" : "关闭"));
    }

    void SetFreePayNSpray(bool enable) {
        XBase::World::SetFreePayNSpray(enable);
        Log::Info(std::string("免费喷漆店：") + (enable ? "开启" : "关闭"));
    }

    void SetFasterClock(bool enable) {
        XBase::World::SetFasterClock(enable);
        Log::Info(std::string("加快时钟：") + (enable ? "开启" : "关闭"));
    }

    void SetFreezeTime(bool enable) {
        XBase::World::SetFreezeTime(enable);
        Log::Info(std::string("冻结时间：") + (enable ? "开启" : "关闭"));
    }

    void SetLockTime(bool enable) {
        int hour = 0;
        int minute = 0;
        if (enable) {
            XBase::World::GetTime(hour, minute);
        }
        XBase::World::SetLockedTime(enable, hour, minute);
        Log::Info(std::string("锁定当前时间：") + (enable ? "开启" : "关闭"));
    }

    void SetNoWaterPhysics(bool enable) {
        XBase::World::SetNoWaterPhysics(enable);
    }

    void DestroyAllPeds() {
        XBase::World::DestroyAllPeds();
    }

    int GetDaysPassed() {
        return XBase::World::GetDaysPassed();
    }

    void SetDaysPassed(int days) {
        XBase::World::SetDaysPassed(days);
    }

    float GetGravity() {
        return XBase::World::GetGravity();
    }

    void SetGravity(float gravity) {
        XBase::World::SetGravity(gravity);
    }

    int SpawnPickup(const XBase::Types::PickupOptions& options) {
        return XBase::World::SpawnPickup(options);
    }

    int SpawnPickup() {
        const int handle = XBase::World::SpawnPickup(GetPickupOptions());
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
        return XBase::World::RemoveTrackedPickups();
    }

    bool UpdateLastPickup() {
        const bool ok = XBase::World::UpdateLastPickup(GetPickupOptions());
        MenuState::ShowNotice(I18n::T(ok ? "world.pickupUpdated" : "world.noPickupToUpdate"), 2.0);
        Log::Info(std::string("修改最近 pickup：") + (ok ? "成功" : "无可修改对象"));
        return ok;
    }

    bool RemoveLastPickup() {
        const bool ok = XBase::World::RemoveLastPickup();
        MenuState::ShowNotice(I18n::T(ok ? "world.pickupRemoved" : "world.noPickupToRemove"), 2.0);
        Log::Info(std::string("删除最近 pickup：") + (ok ? "成功" : "无可删除对象"));
        return ok;
    }

}