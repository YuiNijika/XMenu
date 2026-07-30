#include "Player.h"
#include "ui/MenuState.h"
#include "utils/D3DHook.h"
#include "utils/AppConfig.h"
#include <XBase/Capabilities.h>
#include <XBase/Player.h>
#ifdef GTASA
#include <XBase/Weapon.h>
#endif
#include <windows.h>

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

    bool IsKeyDown(int key) {
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    bool IsAutoFlightHoldActive() {
        const AppConfig::Hotkey* hotkey = AppConfig::GetActionHotkey("player.autoFlight.hold");
        return hotkey && AppConfig::IsHotkeyPressed(*hotkey);
    }

    void ProcessFreeFly(bool active) {
        const bool menuVisible = D3DHook::IsMenuVisible();
        if (!active || menuVisible) return;

        const float speed = MenuState::FreeFlySpeed > 0.05f ? MenuState::FreeFlySpeed : 0.05f;
        float forward = 0.0f;
        float right = 0.0f;
        float up = 0.0f;

        if (IsKeyDown('W')) {
            forward += speed;
        }
        if (IsKeyDown('S')) {
            forward -= speed;
        }
        if (IsKeyDown('D')) {
            right += speed;
        }
        if (IsKeyDown('A')) {
            right -= speed;
        }
        if (IsKeyDown(VK_SPACE)) {
            up += speed;
        }
        if (IsKeyDown('C')) {
            up -= speed;
        }

        if (forward != 0.0f || right != 0.0f || up != 0.0f) {
            XBase::Player::MoveRelative(forward, right, up);
        }
    }
}

namespace Controllers::Player {
    CPlayerPed* GetPlayer() {
        return static_cast<CPlayerPed*>(XBase::Player::GetHandle());
    }

    void Process() {
        const bool menuVisible = D3DHook::IsMenuVisible();
        const bool freeFlyActive = MenuState::FreeFlyEnabled || (!menuVisible && IsAutoFlightHoldActive());

        XBase::Player::RuntimeOptions options;
        options.godMode = MenuState::GodMode;
        options.invisible = MenuState::InvisiblePlayer;
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
        }
#ifdef GTASA
        XBase::Player::SetNeverWanted(MenuState::NeverWanted);
        XBase::Player::SuperJump(MenuState::MegaJump);
        XBase::Player::SuperPunch(MenuState::MegaPunch);
        XBase::Player::SetCycleJump(MenuState::CycleJump);
        XBase::Player::UnderwaterBreathing(MenuState::InfiniteOxygen);
        XBase::Player::SetNeverHungry(MenuState::NeverHungry);
        XBase::Player::SetFastSprint(MenuState::FastSprint);
        XBase::Player::SetSprintEverywhere(MenuState::SprintEverywhere);
        XBase::Player::SetDrunkEffect(MenuState::DrunkEffect);
#endif
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

    void SetKeepStuff(bool enable) {
        XBase::Player::SetKeepStuff(enable);
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

    void MaxVehicleSkills() {
        XBase::Player::MaxVehicleSkills();
    }

    void ApplyAimSkinChanger() {
        XBase::Player::ApplyAimSkinChanger();
    }
}