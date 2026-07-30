#include "World.h"
#include "controllers/World.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }
}

namespace Pages::World {
    void Process() {
        Controllers::World::Process();
    }

    void Draw() {
        if (ImGui::CollapsingHeader(T("world.time"), ImGuiTreeNodeFlags_DefaultOpen)) {
            int hour = 0;
            int minute = 0;
            Controllers::World::GetTime(hour, minute);

            ImGui::PushItemWidth(150);
            bool timeChanged = false;
            if (ImGui::SliderInt(T("world.hour"), &hour, 0, 23)) {
                timeChanged = true;
            }
            if (ImGui::SliderInt(T("world.minute"), &minute, 0, 59)) {
                timeChanged = true;
            }
            ImGui::PopItemWidth();

            if (timeChanged) {
                Controllers::World::SetTime(hour, minute);
            }

            if (ImGui::Checkbox(T("world.lockCurrentTime"), &MenuState::WorldLockTime)) {
                Controllers::World::SetLockTime(MenuState::WorldLockTime);
                AppConfig::Save();
            }

            if (UI::Button(T("world.syncRealTime"))) {
                Controllers::World::SyncTimeWithSystemClock();
            }
        }

        if (ImGui::CollapsingHeader(T("world.weather"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox(T("world.lockCurrentWeather"), &MenuState::LockWeather)) {
                Controllers::World::CaptureWeather();
            }
            Controllers::World::DrawWeatherButtons();

            // Full selector support: id + apply + revert
            int wid = MenuState::LockedWeatherType;
            if (ImGui::InputInt(T("world.weatherId"), &wid)) {
                MenuState::LockedWeatherType = wid;
            }
            ImGui::SameLine();
            if (UI::Button(T("Apply"), 2)) {
                Controllers::World::ForceWeatherNow(MenuState::LockedWeatherType);
                MenuState::LockWeather = true;
            }
            if (UI::Button(T("world.revertWeather"), 2)) {
                Controllers::World::ReleaseWeather();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("lock: %d", MenuState::LockedWeatherType);
        }

        if (ImGui::CollapsingHeader(T("world.gameRules"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Columns(2, nullptr, false);
            if (ImGui::Checkbox(T("world.disableReplay"), &MenuState::DisableReplay)) {
                Controllers::World::SetDisableReplay(MenuState::DisableReplay);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox(T("world.disableCheats"), &MenuState::DisableCheats)) {
                Controllers::World::SetDisableCheats(MenuState::DisableCheats);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox(T("world.fasterClock"), &MenuState::FasterClock)) {
                Controllers::World::SetFasterClock(MenuState::FasterClock);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox(T("world.freezeTime"), &MenuState::FreezeTime)) {
                Controllers::World::SetFreezeTime(MenuState::FreezeTime);
            }
#ifdef GTASA
            ImGui::NextColumn();
            if (ImGui::Checkbox(T("world.disableForbiddenAreaWanted"), &MenuState::ForbiddenAreaWanted)) {
                Controllers::World::SetForbiddenAreaWanted(MenuState::ForbiddenAreaWanted);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox(T("world.freePayNSpray"), &MenuState::FreePayNSpray)) {
                Controllers::World::SetFreePayNSpray(MenuState::FreePayNSpray);
            }
            ImGui::NextColumn();
            ImGui::Checkbox(T("world.solidWater"), &MenuState::SolidWater);
            if (ImGui::Checkbox(T("world.noWaterPhysics"), &MenuState::NoWaterPhysics)) {
                Controllers::World::SetNoWaterPhysics(MenuState::NoWaterPhysics);
            }
#endif
            ImGui::Columns(1);
            ImGui::Spacing();
            ImGui::PushItemWidth(150);

            ImGui::InputInt(T("world.daysPassed"), &MenuState::DaysPassed);
            ImGui::SameLine();
            if (UI::Button(T("world.setDays"), 4)) {
                if (MenuState::DaysPassed < 0) MenuState::DaysPassed = 0;
                if (MenuState::DaysPassed > 9999) MenuState::DaysPassed = 9999;
                Controllers::World::SetDaysPassed(MenuState::DaysPassed);
            }
            ImGui::SameLine();
            if (UI::Button(T("world.readDays"), 4)) {
                MenuState::DaysPassed = Controllers::World::GetDaysPassed();
            }

            float gravity = Controllers::World::GetGravity();
            if (ImGui::SliderFloat(T("world.gravity"), &gravity, -1.0f, 1.0f, "%.3f")) {
                Controllers::World::SetGravity(gravity);
            }

            ImGui::InputInt(T("world.fpsLimit"), &MenuState::FpsLimit);
            ImGui::SameLine();
            if (UI::Button(T("world.setFps"), 4)) {
                if (MenuState::FpsLimit < 1) MenuState::FpsLimit = 1;
                if (MenuState::FpsLimit > 999) MenuState::FpsLimit = 999;
                Controllers::World::SetFpsLimit(MenuState::FpsLimit);
            }
            ImGui::SameLine();
            if (UI::Button(T("world.readFps"), 4)) {
                MenuState::FpsLimit = Controllers::World::GetFpsLimit();
            }
            ImGui::PopItemWidth();
        }

        if (ImGui::CollapsingHeader(T("world.pickup"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextWrapped("%s", T("world.pickupTip"));
            ImGui::PushItemWidth(160);
            ImGui::InputInt(T("world.pickupModelId"), &MenuState::PickupModelId);
            ImGui::InputInt(T("world.pickupType"), &MenuState::PickupType);
            ImGui::InputInt(T("world.pickupQuantity"), &MenuState::PickupQuantity);
#ifndef GTA3
            ImGui::InputInt(T("world.pickupMoneyPerDay"), &MenuState::PickupMoneyPerDay);
            ImGui::Checkbox(T("world.pickupEmpty"), &MenuState::PickupEmpty);
#endif
            ImGui::PopItemWidth();

            if (UI::Button(T("world.spawnPickup"))) {
                Controllers::World::SpawnPickup();
            }
            ImGui::SameLine();
            if (UI::Button(T("world.updateLastPickup"))) {
                Controllers::World::UpdateLastPickup();
            }
            ImGui::SameLine();
            if (UI::Button(T("world.removeLastPickup"))) {
                Controllers::World::RemoveLastPickup();
            }
        }

        if (ImGui::CollapsingHeader(T("world.gameSpeed"), ImGuiTreeNodeFlags_DefaultOpen)) {
            float speed = Controllers::World::GetGameSpeed();
            if (ImGui::SliderFloat(T("world.multiplier"), &speed, 0.1f, 5.0f, "%.1fx")) {
                Controllers::World::SetGameSpeed(speed);
            }
            if (UI::Button(T("world.restoreSpeed"))) {
                Controllers::World::SetGameSpeed(1.0f);
            }
        }

#ifdef GTASA
        if (ImGui::CollapsingHeader(T("world.freecam"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox(T("world.enableFreecam"), &MenuState::FreecamEnabled)) {
                if (MenuState::FreecamEnabled) {
                    Controllers::World::EnableFreecam();
                } else {
                    Controllers::World::DisableFreecam();
                }
            }

            if (MenuState::FreecamEnabled) {
                ImGui::TextWrapped("%s", T("world.freecamControls"));
                ImGui::SliderFloat(T("world.freecamFov"), &MenuState::FreecamFov, 10.0f, 115.0f);
                ImGui::SliderInt(T("world.freecamSpeedMul"), &MenuState::FreecamSpeedMul, 1, 10);
            }
        }

        if (ImGui::CollapsingHeader(T("world.topDownCam"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox(T("world.enableTopDownCam"), &MenuState::TopDownCamEnabled)) {
                if (!MenuState::TopDownCamEnabled) {
                    Controllers::World::DisableTopDownCam();
                }
            }
            if (MenuState::TopDownCamEnabled) {
                ImGui::SliderInt(T("world.topDownCamZoom"), &MenuState::TopDownCamZoom, 10, 100);
            }
        }

        if (ImGui::CollapsingHeader(T("world.randomCheats"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox(T("world.enableRandomCheats"), &MenuState::RandomCheatsEnabled);
            if (MenuState::RandomCheatsEnabled) {
                ImGui::Checkbox(T("world.showRandomCheatsProgress"), &MenuState::RandomCheatsProgressBar);
                ImGui::SliderInt(T("world.randomCheatsInterval"), &MenuState::RandomCheatsInterval, 1, 60);
                
                if (ImGui::TreeNode(T("world.randomCheatsList"))) {
                    Controllers::World::DrawRandomCheatsList();
                    ImGui::TreePop();
                }
            }
        }
#endif
    }
}