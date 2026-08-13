#include "World.h"
#include "controllers/World.h"
#include "ui/MenuState.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"
#include <XBase/Camera.h>
#include <XBase/Cheats.h>
#include <XBase/Capabilities.h>
#include <XBase/UI.h>
#include "integration/XBaseBridge.h"

namespace {
const char* T(const char* key) {
    return I18n::T(key);
}
}

namespace Pages::World {
void Draw() {
    namespace UI = XBase::UI;

    const auto withCapability = [](XBase::FeatureCapability capability, const XBase::UI::DrawFn& draw) {
        XBase::UI::Disabled(!XBaseBridge::HasCapability(capability), draw);
    };

    if (UI::CollapsingHeader(T("world.time"), true)) {
        withCapability(XBase::FeatureCapability::WorldTime, [&] {
        int hour = 0;
        int minute = 0;
        Controllers::World::GetTime(hour, minute);

        UI::PushItemWidth(150.0f);
        const bool timeChanged = UI::Slider(T("world.hour"), hour, 0, 23)
            | UI::Slider(T("world.minute"), minute, 0, 59);
        UI::PopItemWidth();
        if (timeChanged) {
            Controllers::World::SetTime(hour, minute);
        }

        if (UI::Checkbox(T("world.lockCurrentTime"), MenuState::WorldLockTime)) {
            Controllers::World::SetLockTime(MenuState::WorldLockTime);
            AppConfig::Save();
        }
        if (UI::Button(T("world.syncRealTime"))) {
            Controllers::World::SyncTimeWithSystemClock();
        }
        });
    }

    if (UI::CollapsingHeader(T("world.weather"), true)) {
        withCapability(XBase::FeatureCapability::WorldWeather, [&] {
        if (UI::Checkbox(T("world.lockCurrentWeather"), MenuState::LockWeather)) {
            Controllers::World::CaptureWeather();
        }
        Controllers::World::DrawWeatherButtons();

        int weatherId = MenuState::LockedWeatherType;
        if (UI::Input(T("world.weatherId"), weatherId)) {
            MenuState::LockedWeatherType = weatherId;
        }
        UI::SameLine();
        if (UI::Button(T("Apply"))) {
            Controllers::World::ForceWeatherNow(MenuState::LockedWeatherType);
            MenuState::LockWeather = true;
        }
        if (UI::Button(T("world.revertWeather"))) {
            Controllers::World::ReleaseWeather();
        }
        UI::SameLine();
        UI::TextDisabled("lock:");
        UI::SameLine();
        UI::Text("%d", MenuState::LockedWeatherType);
        });
    }

    if (UI::CollapsingHeader(T("world.gameRules"), true)) {
        UI::Columns(2, nullptr, false);
        const auto drawRule = [&](XBase::FeatureCapability capability, const char* label,
                                  bool& value, const XBase::UI::DrawFn& apply) {
            withCapability(capability, [&] {
                if (UI::Checkbox(label, value)) apply();
            });
            UI::NextColumn();
        };
        drawRule(XBase::FeatureCapability::WorldDisableReplay,
            T("world.disableReplay"), MenuState::DisableReplay,
            [&] { Controllers::World::SetDisableReplay(MenuState::DisableReplay); });
        drawRule(XBase::FeatureCapability::WorldDisableCheats,
            T("world.disableCheats"), MenuState::DisableCheats,
            [&] { Controllers::World::SetDisableCheats(MenuState::DisableCheats); });
        drawRule(XBase::FeatureCapability::WorldFasterClock,
            T("world.fasterClock"), MenuState::FasterClock,
            [&] { Controllers::World::SetFasterClock(MenuState::FasterClock); });
        drawRule(XBase::FeatureCapability::WorldFreezeTime,
            T("world.freezeTime"), MenuState::FreezeTime,
            [&] { Controllers::World::SetFreezeTime(MenuState::FreezeTime); });
        drawRule(XBase::FeatureCapability::WorldForbiddenAreaWanted,
            T("world.disableForbiddenAreaWanted"), MenuState::ForbiddenAreaWanted,
            [&] { Controllers::World::SetForbiddenAreaWanted(MenuState::ForbiddenAreaWanted); });
        drawRule(XBase::FeatureCapability::WorldFreePayNSpray,
            T("world.freePayNSpray"), MenuState::FreePayNSpray,
            [&] { Controllers::World::SetFreePayNSpray(MenuState::FreePayNSpray); });
#ifdef GTASA
        UI::Checkbox(T("world.solidWater"), MenuState::SolidWater);
        UI::NextColumn();
#endif
        drawRule(XBase::FeatureCapability::WorldNoWaterPhysics,
            T("world.noWaterPhysics"), MenuState::NoWaterPhysics,
            [&] { Controllers::World::SetNoWaterPhysics(MenuState::NoWaterPhysics); });
        UI::Columns(1);
        UI::Spacing();
        UI::PushItemWidth(150.0f);

        withCapability(XBase::FeatureCapability::WorldDaysPassed, [&] {
            UI::Input(T("world.daysPassed"), MenuState::DaysPassed);
            UI::SameLine();
            if (UI::Button(T("world.setDays"))) {
                if (MenuState::DaysPassed < 0) MenuState::DaysPassed = 0;
                if (MenuState::DaysPassed > 9999) MenuState::DaysPassed = 9999;
                Controllers::World::SetDaysPassed(MenuState::DaysPassed);
            }
            UI::SameLine();
            if (UI::Button(T("world.readDays"))) {
                MenuState::DaysPassed = Controllers::World::GetDaysPassed();
            }
        });

        withCapability(XBase::FeatureCapability::WorldGravity, [&] {
            float gravity = Controllers::World::GetGravity();
            if (UI::Slider(T("world.gravity"), gravity, -1.0f, 1.0f, "%.3f")) {
                Controllers::World::SetGravity(gravity);
            }
        });

        withCapability(XBase::FeatureCapability::WorldFpsLimit, [&] {
            UI::Input(T("world.fpsLimit"), MenuState::FpsLimit);
            UI::SameLine();
            if (UI::Button(T("world.setFps"))) {
                if (MenuState::FpsLimit < 1) MenuState::FpsLimit = 1;
                if (MenuState::FpsLimit > 999) MenuState::FpsLimit = 999;
                Controllers::World::SetFpsLimit(MenuState::FpsLimit);
            }
            UI::SameLine();
            if (UI::Button(T("world.readFps"))) {
                MenuState::FpsLimit = Controllers::World::GetFpsLimit();
            }
        });
        UI::PopItemWidth();
    }

    if (UI::CollapsingHeader(T("world.pickup"), true)) {
        withCapability(XBase::FeatureCapability::WorldPickups, [&] {
        UI::TextWrapped(T("world.pickupTip"));
        UI::PushItemWidth(160.0f);
        UI::Input(T("world.pickupModelId"), MenuState::PickupModelId);
        UI::Input(T("world.pickupType"), MenuState::PickupType);
        UI::Input(T("world.pickupQuantity"), MenuState::PickupQuantity);
#ifndef GTA3
        UI::Input(T("world.pickupMoneyPerDay"), MenuState::PickupMoneyPerDay);
        UI::Checkbox(T("world.pickupEmpty"), MenuState::PickupEmpty);
#endif
        UI::PopItemWidth();

        if (UI::Button(T("world.spawnPickup"))) Controllers::World::SpawnPickup();
        UI::SameLine();
        if (UI::Button(T("world.updateLastPickup"))) Controllers::World::UpdateLastPickup();
        UI::SameLine();
        if (UI::Button(T("world.removeLastPickup"))) Controllers::World::RemoveLastPickup();
        });
    }

    if (UI::CollapsingHeader(T("world.gameSpeed"), true)) {
        withCapability(XBase::FeatureCapability::WorldGameSpeed, [&] {
        float speed = Controllers::World::GetGameSpeed();
        if (UI::Slider(T("world.multiplier"), speed, 0.1f, 5.0f, "%.1fx")) {
            Controllers::World::SetGameSpeed(speed);
        }
        if (UI::Button(T("world.restoreSpeed"))) {
            Controllers::World::SetGameSpeed(1.0f);
        }
        });
    }

#ifdef GTASA
    XBase::Camera::Settings cameraSettings = XBase::Camera::GetSettings();
    XBase::Camera::Mode cameraMode = XBase::Camera::GetMode();

    if (UI::CollapsingHeader(T("world.freecam"), true)) {
        bool enabled = cameraMode == XBase::Camera::Mode::Freecam;
        if (UI::Checkbox(T("world.enableFreecam"), enabled)) {
            const XBase::Camera::Mode requested = enabled
                ? XBase::Camera::Mode::Freecam
                : XBase::Camera::Mode::Disabled;
            XBase::Camera::SetMode(requested);
            cameraMode = XBase::Camera::GetMode();
        }
        if (cameraMode == XBase::Camera::Mode::Freecam) {
            UI::TextWrapped(T("world.freecamControls"));
            const bool settingsChanged =
                UI::Slider(T("world.freecamFov"), cameraSettings.freecamFov, 10.0f, 115.0f)
                | UI::Slider(T("world.freecamSpeedMul"), cameraSettings.freecamSpeed, 1, 10);
            if (settingsChanged) XBase::Camera::SetSettings(cameraSettings);
        }
    }

    if (UI::CollapsingHeader(T("world.topDownCam"), true)) {
        bool enabled = cameraMode == XBase::Camera::Mode::TopDown;
        if (UI::Checkbox(T("world.enableTopDownCam"), enabled)) {
            const XBase::Camera::Mode requested = enabled
                ? XBase::Camera::Mode::TopDown
                : XBase::Camera::Mode::Disabled;
            XBase::Camera::SetMode(requested);
            cameraMode = XBase::Camera::GetMode();
        }
        if (cameraMode == XBase::Camera::Mode::TopDown
            && UI::Slider(T("world.topDownCamZoom"), cameraSettings.topDownZoom, 10, 100)) {
            XBase::Camera::SetSettings(cameraSettings);
        }
    }

    if (UI::CollapsingHeader(T("world.randomCheats"), true)) {
        XBase::Cheats::RandomSettings randomSettings = XBase::Cheats::GetRandomSettings();
        bool settingsChanged = UI::Checkbox(T("world.enableRandomCheats"), randomSettings.enabled);
        if (randomSettings.enabled) {
            settingsChanged |= UI::Checkbox(
                T("world.showRandomCheatsProgress"), randomSettings.showProgress);
            settingsChanged |= UI::Slider(
                T("world.randomCheatsInterval"), randomSettings.intervalSeconds, 1, 60);
            UI::Tree(T("world.randomCheatsList"), [] {
                const std::size_t count = XBase::Cheats::GetRandomCheatCount();
                for (std::size_t index = 0; index < count; ++index) {
                    const char* name = XBase::Cheats::GetRandomCheatName(index);
                    if (!name) continue;
                    const bool selected = XBase::Cheats::IsRandomCheatEnabled(index);
                    if (XBase::UI::MenuItem(name, selected)) {
                        XBase::Cheats::SetRandomCheatEnabled(index, !selected);
                    }
                }
            });
        }
        if (settingsChanged) XBase::Cheats::SetRandomSettings(randomSettings);
    }
#endif
}
} // namespace Pages::World