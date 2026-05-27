#include "Hotkeys.h"
#include "Player.h"
#include "Vehicle.h"
#include "Weapon.h"
#include "World.h"
#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/AppConfig.h"
#include "utils/D3DHook.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include <windows.h>
#include <unordered_map>
#include <cstdio>
#include <string>

namespace {
    std::unordered_map<std::string, bool> previousActionStates;
    constexpr float ForwardTeleportBaseIntervalMs = 180.0f;

    float ForwardTeleportFrameScale() {
        if (!ImGui::GetCurrentContext()) {
            return 16.6667f / ForwardTeleportBaseIntervalMs;
        }

        float deltaMs = ImGui::GetIO().DeltaTime * 1000.0f;
        if (deltaMs < 1.0f) {
            deltaMs = 1.0f;
        }
        if (deltaMs > 50.0f) {
            deltaMs = 50.0f;
        }
        return deltaMs / ForwardTeleportBaseIntervalMs;
    }

    bool HasBlockingUiInput() {
        if (D3DHook::IsMenuVisible()) {
            return true;
        }

        if (!ImGui::GetCurrentContext()) {
            return false;
        }

        const ImGuiIO& io = ImGui::GetIO();
        return io.WantTextInput || io.WantCaptureKeyboard;
    }

    void UpdatePreviousStatesOnly() {
        const std::vector<AppConfig::ActionHotkey>& actions = AppConfig::GetActionHotkeys();
        for (const AppConfig::ActionHotkey& action : actions) {
            previousActionStates[action.id] = AppConfig::IsHotkeyPressed(action.hotkey);
        }
    }

    void ShowActionNotice(const AppConfig::ActionHotkey& action) {
        char message[256] = {};
        std::snprintf(message, sizeof(message), I18n::T("hotkey.triggered"), I18n::T(action.name.c_str()));
        MenuState::ShowNotice(message, 1.5);
    }

    void Dispatch(const std::string& actionId) {
        if (!AppConfig::IsActionHotkeySupportedForRuntime(actionId)) {
            return;
        }

        if (actionId == "teleport.marker") {
            Controllers::Teleport::Marker(MenuState::SpawnUnderwater);
            return;
        }

        if (actionId == "teleport.quickMap") {
            MenuState::QuickTeleport = true;
            MenuState::QuickTeleportMapActive = !MenuState::QuickTeleportMapActive;
            MenuState::ShowNotice(MenuState::QuickTeleportMapActive ? I18n::T("quickMap.opened") : I18n::T("quickMap.closed"), 1.5);
            return;
        }

        if (actionId == "teleport.forward") {
            Controllers::Teleport::Forward(MenuState::TeleportForwardDistance);
            return;
        }

        if (actionId == "player.freeFly.toggle") {
            MenuState::FreeFlyEnabled = !MenuState::FreeFlyEnabled;
            MenuState::ShowNotice(I18n::T(MenuState::FreeFlyEnabled ? "player.freeFlyEnabled" : "player.freeFlyDisabled"), 1.5);
            return;
        }

        if (actionId == "command.toggle") {
            MenuState::CommandWindowEnabled = !MenuState::CommandWindowEnabled;
            return;
        }

        if (actionId == "overlay.toggle") {
            MenuState::OverlayEnabled = !MenuState::OverlayEnabled;
            return;
        }

        if (actionId == "player.heal") {
            Controllers::Player::Heal();
            return;
        }

        if (actionId == "player.armour") {
            Controllers::Player::GiveArmour();
            return;
        }

        if (actionId == "player.clearWanted") {
            Controllers::Player::ClearWantedLevel();
            return;
        }

        if (actionId == "vehicle.repair") {
            Controllers::Vehicle::Repair();
            return;
        }

        if (actionId == "vehicle.unflip") {
            Controllers::Vehicle::Unflip();
            return;
        }

        if (actionId == "vehicle.stop") {
            Controllers::Vehicle::Stop();
            return;
        }

        if (actionId == "weapon.giveAll") {
            Controllers::Weapon::GiveAll();
            return;
        }

        if (actionId == "world.toggleFreezeTime") {
            MenuState::FreezeTime = !MenuState::FreezeTime;
            Controllers::World::SetFreezeTime(MenuState::FreezeTime);
            return;
        }
    }
}

namespace Controllers::Hotkeys {
    void Process() {
        if (AppConfig::IsMenuHotkeyPressed() || HasBlockingUiInput()) {
            UpdatePreviousStatesOnly();
            return;
        }

        const std::vector<AppConfig::ActionHotkey>& actions = AppConfig::GetActionHotkeys();
        for (const AppConfig::ActionHotkey& action : actions) {
            const bool pressed = AppConfig::IsHotkeyPressed(action.hotkey);
            const bool wasPressed = previousActionStates[action.id];
            if (action.id == "player.autoFlight.hold") {
                previousActionStates[action.id] = pressed;
                continue;
            }
            if (action.id == "teleport.forward") {
                if (pressed && MenuState::TeleportForwardHold) {
                    const float continuousDistance = MenuState::TeleportForwardDistance * ForwardTeleportFrameScale();
                    Controllers::Teleport::Forward(continuousDistance);
                } else if (pressed && !wasPressed) {
                    Dispatch(action.id);
                    ShowActionNotice(action);
                }
                previousActionStates[action.id] = pressed;
                continue;
            }

            if (pressed) {
                if (!wasPressed) {
                    Dispatch(action.id);
                }
                if (!wasPressed && action.id != "teleport.quickMap") {
                    ShowActionNotice(action);
                }
            }
            previousActionStates[action.id] = pressed;
        }
    }
}