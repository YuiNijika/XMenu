#include "Hotkeys.h"
#include "Player.h"
#include "Vehicle.h"
#include "Weapon.h"
#include "World.h"
#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/AppConfig.h"
#include <XBase/Hooks.h>
#include <XBase/Teleport.h>
#include "utils/I18n.h"
#include "integration/XBaseBridge.h"
#include <unordered_map>
#include <cstdio>
#include <string>

namespace {
    std::unordered_map<std::string, bool> previousActionStates;
    constexpr float ForwardTeleportBaseIntervalMs = 180.0f;

    float ForwardTeleportFrameScale() {
        float deltaMs = XBase::Hooks::GetFrameDeltaSeconds() * 1000.0f;
        if (deltaMs < 1.0f) {
            deltaMs = 1.0f;
        }
        if (deltaMs > 50.0f) {
            deltaMs = 50.0f;
        }
        return deltaMs / ForwardTeleportBaseIntervalMs;
    }

    bool HasBlockingUiInput() {
        if (XBase::Hooks::IsMenuVisible()) {
            return true;
        }

        return XBase::Hooks::IsKeyboardCaptureActive();
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

    bool Dispatch(const std::string& actionId) {
        if (!AppConfig::IsActionHotkeySupportedForRuntime(actionId)) {
            return false;
        }

        if (actionId == "teleport.marker") {
            return XBase::Teleport::Marker(MenuState::SpawnUnderwater);
        }

        if (actionId == "teleport.quickMap") {
            MenuState::QuickTeleport = true;
            MenuState::QuickTeleportMapActive = !MenuState::QuickTeleportMapActive;
            MenuState::ShowNotice(MenuState::QuickTeleportMapActive ? I18n::T("quickMap.opened") : I18n::T("quickMap.closed"), 1.5);
            return true;
        }

        if (actionId == "teleport.forward") {
            return XBase::Teleport::Forward(MenuState::TeleportForwardDistance);
        }

        if (actionId == "player.freeFly.toggle") {
            MenuState::FreeFlyEnabled = !MenuState::FreeFlyEnabled;
            MenuState::ShowNotice(I18n::T(MenuState::FreeFlyEnabled ? "player.freeFlyEnabled" : "player.freeFlyDisabled"), 1.5);
            return true;
        }

        if (actionId == "command.toggle") {
            MenuState::CommandWindowEnabled = !MenuState::CommandWindowEnabled;
            return true;
        }

        if (actionId == "overlay.toggle") {
            MenuState::OverlayEnabled = !MenuState::OverlayEnabled;
            return true;
        }

        if (actionId == "player.heal") {
            Controllers::Player::Heal();
            return true;
        }

        if (actionId == "player.armour") {
            Controllers::Player::GiveArmour();
            return true;
        }

        if (actionId == "player.clearWanted") {
            Controllers::Player::ClearWantedLevel();
            return true;
        }

        if (actionId == "player.aimSkinChanger") {
            if (!XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerAimSkinChanger)) {
                return false;
            }
            Controllers::Player::ApplyAimSkinChanger();
            return true;
        }

        if (actionId == "player.neverWanted.toggle") {
            if (!XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerNeverWanted)) {
                return false;
            }
            MenuState::NeverWanted = !MenuState::NeverWanted;
            return true;
        }

        if (actionId == "vehicle.repair") {
            Controllers::Vehicle::Repair();
            return true;
        }

        if (actionId == "vehicle.unflip") {
            Controllers::Vehicle::Unflip();
            return true;
        }

        if (actionId == "vehicle.stop") {
            Controllers::Vehicle::Stop();
            return true;
        }

        if (actionId == "weapon.giveAll") {
            return Controllers::Weapon::GiveAll();
        }

        if (actionId == "world.toggleFreezeTime") {
            if (!XBaseBridge::HasCapability(XBase::FeatureCapability::WorldFreezeTime)) {
                return false;
            }
            MenuState::FreezeTime = !MenuState::FreezeTime;
            Controllers::World::SetFreezeTime(MenuState::FreezeTime);
            return true;
        }

        return false;
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
                    XBase::Teleport::Forward(continuousDistance);
                } else if (pressed && !wasPressed) {
                    if (Dispatch(action.id)) {
                        ShowActionNotice(action);
                    }
                }
                previousActionStates[action.id] = pressed;
                continue;
            }

            if (pressed && !wasPressed) {
                const bool succeeded = Dispatch(action.id);
                if (succeeded && action.id != "teleport.quickMap") {
                    ShowActionNotice(action);
                }
            }
            previousActionStates[action.id] = pressed;
        }
    }
}