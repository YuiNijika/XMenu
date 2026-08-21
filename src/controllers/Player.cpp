#include "Player.h"
#include "ui/MenuState.h"
#include <XBase/Hooks.h>
#include <XBase/Input.h>
#include "utils/AppConfig.h"
#include <XBase/Capabilities.h>
#include <XBase/Player.h>
#ifdef GTASA
#include <XBase/Weapon.h>
#endif

namespace {
    int ClampWantedLevel(int level) {
        if (level < 0) {
            return 0;
        }
        if (level > 6) {
            return 6;
        }
        return level;
    }

    float ClampHealth(float value) {
        if (value <= 0.0f) {
            return 0.0f;
        }
        return value < 2.0f ? 2.0f : value;
    }

    bool IsKeyDown(XBase::Input::Key key) {
        return XBase::Input::IsDown(key);
    }

    bool IsAutoFlightHoldActive() {
        const AppConfig::Hotkey* hotkey = AppConfig::GetActionHotkey("player.autoFlight.hold");
        return hotkey && AppConfig::IsHotkeyPressed(*hotkey);
    }

    void ProcessFreeFly(bool active) {
        const bool menuVisible = XBase::Hooks::IsMenuVisible();
        if (!active || menuVisible) return;

        const float speed = MenuState::FreeFlySpeed > 0.05f ? MenuState::FreeFlySpeed : 0.05f;
        float forward = 0.0f;
        float right = 0.0f;
        float up = 0.0f;

        if (IsKeyDown(XBase::Input::Key::W)) {
            forward += speed;
        }
        if (IsKeyDown(XBase::Input::Key::S)) {
            forward -= speed;
        }
        if (IsKeyDown(XBase::Input::Key::D)) {
            right += speed;
        }
        if (IsKeyDown(XBase::Input::Key::A)) {
            right -= speed;
        }
        if (IsKeyDown(XBase::Input::Key::Space)) {
            up += speed;
        }
        if (IsKeyDown(XBase::Input::Key::C)) {
            up -= speed;
        }

        if (forward != 0.0f || right != 0.0f || up != 0.0f) {
            XBase::Player::MoveRelative(forward, right, up);
        }
    }
}

namespace Controllers::Player {
    bool GetPlayerAvailable() {
        return XBase::Player::IsAvailable();
    }

    void Process() {
        const bool menuVisible = XBase::Hooks::IsMenuVisible();
        const bool freeFlyActive = MenuState::FreeFlyEnabled || (!menuVisible && IsAutoFlightHoldActive());

        XBase::Player::RuntimeOptions options;
        options.godMode = MenuState::GodMode;
#ifdef GTASA
        options.invisible = MenuState::InvisiblePlayer;
#endif
        options.hardMode = MenuState::HardMode;
        options.autoHeal = MenuState::AutoHeal;
        options.respawnAtDeathPosition = MenuState::RespawnAtDeathPosition;
        options.freezeWantedLevel = MenuState::FreezeWantedLevel;
        options.wantedLevel = ClampWantedLevel(MenuState::WantedLevel);
        options.freeFlyProtection = freeFlyActive;
        XBase::Player::SetRuntimeOptions(options);

        XBase::Player::SetInfiniteSprint(MenuState::InfiniteSprint);
        if (XBase::HasCapability(XBase::FeatureCapability::PlayerKeepStuff)) {
            XBase::Player::SetKeepStuff(MenuState::KeepStuff);
        } else {
            MenuState::KeepStuff = false;
        }
        const auto syncToggle = [](XBase::FeatureCapability capability, bool& state, auto&& setter) {
            if (!XBase::HasCapability(capability)) {
                state = false;
                return;
            }
            setter(state);
        };
        syncToggle(XBase::FeatureCapability::PlayerNeverWanted, MenuState::NeverWanted,
            [](bool enabled) { XBase::Player::SetNeverWanted(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerSuperJump, MenuState::MegaJump,
            [](bool enabled) { XBase::Player::SuperJump(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerSuperPunch, MenuState::MegaPunch,
            [](bool enabled) { XBase::Player::SuperPunch(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerCycleJump, MenuState::CycleJump,
            [](bool enabled) { XBase::Player::SetCycleJump(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerUnderwaterBreathing, MenuState::InfiniteOxygen,
            [](bool enabled) { XBase::Player::UnderwaterBreathing(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerNeverHungry, MenuState::NeverHungry,
            [](bool enabled) { XBase::Player::SetNeverHungry(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerFastSprint, MenuState::FastSprint,
            [](bool enabled) { XBase::Player::SetFastSprint(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerSprintEverywhere, MenuState::SprintEverywhere,
            [](bool enabled) { XBase::Player::SetSprintEverywhere(enabled); });
        syncToggle(XBase::FeatureCapability::PlayerDrunkEffect, MenuState::DrunkEffect,
            [](bool enabled) { XBase::Player::SetDrunkEffect(enabled); });
        if (!XBase::HasCapability(XBase::FeatureCapability::PlayerAimSkinChanger)) {
            MenuState::AimSkinChanger = false;
        }
        ProcessFreeFly(freeFlyActive);
    }

    void Heal() {
        XBase::Player::Heal();
    }

    void GiveArmour() {
        XBase::Player::GiveArmour();
    }

    void GiveMoney() {
        XBase::Player::GiveMoney(250000);
    }

    void Kill() {
        XBase::Player::Kill();
    }

    int GetWantedLevel() {
        return XBase::Player::GetWantedLevel();
    }

    void SetWantedLevel(int level) {
        XBase::Player::SetWantedLevel(ClampWantedLevel(level));
    }

    void ClearWantedLevel() {
        SetWantedLevel(0);
    }

    int GetMoney() {
        return XBase::Player::GetMoney();
    }

    void SetMoney(int amount) {
        XBase::Player::SetMoney(amount);
    }

    float GetHealth() {
        return XBase::Player::GetHealth();
    }

    void SetHealth(float value) {
        XBase::Player::SetHealth(ClampHealth(value));
    }

    float GetArmour() {
        return XBase::Player::GetArmour();
    }

    void SetArmour(float value) {
        XBase::Player::SetArmour(value);
    }

    GameTypes::ProofState GetProofState() {
        const XBase::Types::ProofState state = XBase::Player::GetProofState();
        GameTypes::ProofState result;
        result.bullet = state.bullet;
        result.collision = state.collision;
        result.explosion = state.explosion;
        result.fire = state.fire;
        result.melee = state.melee;
        result.nonPlayer = state.nonPlayer;
        return result;
    }

    void SetProofState(const GameTypes::ProofState& state) {
        if (MenuState::GodMode) return;
        XBase::Types::ProofState result;
        result.bullet = state.bullet;
        result.collision = state.collision;
        result.explosion = state.explosion;
        result.fire = state.fire;
        result.melee = state.melee;
        result.nonPlayer = state.nonPlayer;
        XBase::Player::SetProofState(result);
    }

    void CopyCoordinates() {
        XBase::Player::CopyCoordinates();
    }

    void SetInfiniteSprint(bool enable) {
        XBase::Player::SetInfiniteSprint(enable);
    }

    bool RequestSaveGame() {
        return XBase::Player::RequestSaveGame();
    }

    void MoveForward(float distance) {
        XBase::Player::MoveForward(distance);
    }

    void MoveUp(float distance) {
        XBase::Player::MoveUp(distance);
    }

    void MoveDown(float distance) {
        XBase::Player::MoveDown(distance);
    }

    bool SetKeepStuff(bool enable) {
        return XBase::Player::SetKeepStuff(enable);
    }

    bool GetFreeHealthcare() {
        return XBase::Player::GetFreeHealthcare();
    }

    void SetFreeHealthcare(bool enable) {
        XBase::Player::SetFreeHealthcare(enable);
    }

    bool GetFreeJail() {
        return XBase::Player::GetFreeJail();
    }

    void SetFreeJail(bool enable) {
        XBase::Player::SetFreeJail(enable);
    }

    bool SetSkin(unsigned int modelId) {
        return XBase::Player::SetSkin(modelId);
    }

    bool SetCustomSkin(const char* name) {
        return XBase::Player::SetCustomSkin(name);
    }

    bool ApplyClothes(int textureId, int modelId, int bodyPart) {
        return XBase::Player::ApplyClothes(textureId, modelId, bodyPart);
    }

    bool SetStat(int statId, float value) {
        return XBase::Player::SetStat(statId, value);
    }

    void MaxWeaponSkills() {
#ifdef GTASA
        XBase::Weapon::MaxWeaponSkills();
#endif
    }

    bool MaxVehicleSkills() {
        return XBase::Player::MaxVehicleSkills();
    }

    bool ApplyAimSkinChanger() {
        return XBase::Player::ApplyAimSkinChanger();
    }
}