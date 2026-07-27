#include "Player.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/D3DHook.h"
#include "utils/AppConfig.h"
#include "plugin.h"
#include "CPlayerPed.h"
#include "CPools.h"
#include <cstdio>
#include <cstring>
#include <windows.h>

namespace {
    CPlayerPed* godModePlayer = nullptr;
    GameTypes::ProofState savedPlayerProofs;
    float savedGodModeHealth = 0.0f;
    bool hasSavedPlayerProofs = false;
    CPlayerPed* freeFlyPlayer = nullptr;
    GameTypes::ProofState savedFreeFlyProofs;
    float savedFreeFlyHealth = 0.0f;
    bool hasSavedFreeFlyProofs = false;

    bool IsPedInPool(CPed* ped) {
        if (!ped || !CPools::ms_pPedPool) {
            return false;
        }
        for (CPed* poolPed : CPools::ms_pPedPool) {
            if (poolPed == ped) {
                return true;
            }
        }
        return false;
    }

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

    GameLogic::ProofState ToLogicProof(const GameTypes::ProofState& state) {
        GameLogic::ProofState result;
        result.bullet = state.bullet;
        result.collision = state.collision;
        result.explosion = state.explosion;
        result.fire = state.fire;
        result.melee = state.melee;
        result.nonPlayer = state.nonPlayer;
        return result;
    }

    GameTypes::ProofState ToUiProof(const GameLogic::ProofState& state) {
        GameTypes::ProofState result;
        result.bullet = state.bullet;
        result.collision = state.collision;
        result.explosion = state.explosion;
        result.fire = state.fire;
        result.melee = state.melee;
        result.nonPlayer = state.nonPlayer;
        return result;
    }

    void RestoreGodModeState() {
        if (!hasSavedPlayerProofs) {
            GameLogic::ApplyGodMode(nullptr, false);
            return;
        }

        if (godModePlayer && IsPedInPool(godModePlayer)) {
            GameLogic::SetPlayerProofState(godModePlayer, ToLogicProof(savedPlayerProofs));
        }
        GameLogic::ApplyGodMode(nullptr, false);
        savedGodModeHealth = 0.0f;
        godModePlayer = nullptr;
        hasSavedPlayerProofs = false;
    }

    void RestoreFreeFlyProtection() {
        if (!hasSavedFreeFlyProofs) {
            return;
        }

        if (freeFlyPlayer && IsPedInPool(freeFlyPlayer)) {
            GameLogic::SetPlayerProofState(freeFlyPlayer, ToLogicProof(savedFreeFlyProofs));
        }
        savedFreeFlyHealth = 0.0f;
        freeFlyPlayer = nullptr;
        hasSavedFreeFlyProofs = false;
    }

    void ProcessFreeFlyProtection(CPlayerPed* player, bool active) {
        if (!active || MenuState::GodMode) {
            RestoreFreeFlyProtection();
            return;
        }

        if (!player || !IsPedInPool(player)) {
            RestoreFreeFlyProtection();
            return;
        }

        if (!hasSavedFreeFlyProofs || freeFlyPlayer != player) {
            RestoreFreeFlyProtection();
            savedFreeFlyProofs = ToUiProof(GameLogic::GetPlayerProofState(player));
            savedFreeFlyHealth = ClampHealth(GameLogic::GetHealth(player));
            freeFlyPlayer = player;
            hasSavedFreeFlyProofs = true;
        }

        const float currentHealth = GameLogic::GetHealth(player);
        if (currentHealth > savedFreeFlyHealth) {
            savedFreeFlyHealth = ClampHealth(currentHealth);
        }

        GameLogic::ProofState protectedState = ToLogicProof(savedFreeFlyProofs);
        protectedState.collision = true;
        protectedState.nonPlayer = true;
        GameLogic::SetPlayerProofState(player, protectedState);
        if (GameLogic::GetHealth(player) < savedFreeFlyHealth) {
            GameLogic::SetHealth(player, savedFreeFlyHealth);
        }
    }

    void ProcessGodMode(CPlayerPed* player) {
        if (!MenuState::GodMode) {
            RestoreGodModeState();
            return;
        }

        if (!player || !IsPedInPool(player)) {
            // 玩家指针无效时仍清全局无敌位，并丢弃旧缓存
            GameLogic::ApplyGodMode(nullptr, false);
            if (hasSavedPlayerProofs && (!godModePlayer || !IsPedInPool(godModePlayer))) {
                savedGodModeHealth = 0.0f;
                godModePlayer = nullptr;
                hasSavedPlayerProofs = false;
            }
            return;
        }

        if (!hasSavedPlayerProofs || godModePlayer != player) {
            RestoreGodModeState();
            savedPlayerProofs = ToUiProof(GameLogic::GetPlayerProofState(player));
            savedGodModeHealth = ClampHealth(GameLogic::GetHealth(player));
            godModePlayer = player;
            hasSavedPlayerProofs = true;
        }

        const float currentHealth = GameLogic::GetHealth(player);
        if (currentHealth > savedGodModeHealth) {
            savedGodModeHealth = ClampHealth(currentHealth);
        }

        GameLogic::ApplyGodMode(player, true);
        if (GameLogic::GetHealth(player) < savedGodModeHealth) {
            GameLogic::SetHealth(player, savedGodModeHealth);
        }
    }

    bool IsKeyDown(int key) {
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    bool IsAutoFlightHoldActive() {
        const AppConfig::Hotkey* hotkey = AppConfig::GetActionHotkey("player.autoFlight.hold");
        return hotkey && AppConfig::IsHotkeyPressed(*hotkey);
    }

    void ProcessFreeFly(CPlayerPed* player) {
        const bool menuVisible = D3DHook::IsMenuVisible();
        const bool active = MenuState::FreeFlyEnabled || (!menuVisible && IsAutoFlightHoldActive());
        ProcessFreeFlyProtection(player, active);

        if (!player || !active || menuVisible) {
            return;
        }

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
            GameLogic::MovePlayerRelative(player, forward, right, up);
        }
    }
}

namespace Controllers::Player {
    CPlayerPed* GetPlayer() {
        return FindPlayerPed();
    }

    void Process() {
        CPlayerPed* player = GetPlayer();
        if (!player) {
            // 玩家未创建时不写 Players[] / 不打补丁，III 启动与读档窗口尤其敏感
            ProcessGodMode(nullptr);
            ProcessFreeFly(nullptr);
            return;
        }

        ProcessGodMode(player);
        GameLogic::SetInfiniteSprint(MenuState::InfiniteSprint);
        GameLogic::SetKeepStuff(MenuState::KeepStuff);

        GameLogic::ProcessAutoHeal(player, MenuState::AutoHeal);
        GameLogic::ProcessHardMode(player, MenuState::HardMode);
        GameLogic::ProcessRespawnAtDeathPosition(player, MenuState::RespawnAtDeathPosition);
        GameLogic::ProcessFreezeWantedLevel(player, MenuState::FreezeWantedLevel, ClampWantedLevel(MenuState::WantedLevel));
        GameLogic::ProcessPlayerCheats(player);
        ProcessFreeFly(player);
    }

    void Heal() {
        GameLogic::SetHealth(GetPlayer(), 100.0f);
    }

    void GiveArmour() {
        GameLogic::SetArmour(GetPlayer(), 100.0f);
    }

    void GiveMoney() {
        GameLogic::GiveMoney(250000);
    }

    void Kill() {
        GameLogic::SetHealth(GetPlayer(), 0.0f);
    }

    int GetWantedLevel() {
        return ClampWantedLevel(GameLogic::GetWantedLevel(GetPlayer()));
    }

    void SetWantedLevel(int level) {
        GameLogic::SetWantedLevel(GetPlayer(), ClampWantedLevel(level));
    }

    void ClearWantedLevel() {
        SetWantedLevel(0);
    }

    int GetMoney() {
        return GameLogic::GetMoney();
    }

    void SetMoney(int amount) {
        GameLogic::SetMoney(amount);
    }

    float GetHealth() {
        return GameLogic::GetHealth(GetPlayer());
    }

    void SetHealth(float value) {
        GameLogic::SetHealth(GetPlayer(), ClampHealth(value));
    }

    float GetArmour() {
        return GameLogic::GetArmour(GetPlayer());
    }

    void SetArmour(float value) {
        GameLogic::SetArmour(GetPlayer(), value);
    }

    GameTypes::ProofState GetProofState() {
        return ToUiProof(GameLogic::GetPlayerProofState(GetPlayer()));
    }

    void SetProofState(const GameTypes::ProofState& state) {
        if (!MenuState::GodMode) {
            GameLogic::SetPlayerProofState(GetPlayer(), ToLogicProof(state));
        }
    }

    void CopyCoordinates() {
        const CVector pos = GameLogic::GetPlayerPosition(GetPlayer());
        char text[128];
        std::snprintf(text, sizeof(text), "%.3f, %.3f, %.3f", pos.x, pos.y, pos.z);
        const size_t length = std::strlen(text) + 1;
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, length);
        if (!memory) {
            return;
        }

        void* locked = GlobalLock(memory);
        if (!locked) {
            GlobalFree(memory);
            return;
        }

        std::memcpy(locked, text, length);
        GlobalUnlock(memory);

        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            SetClipboardData(CF_TEXT, memory);
            CloseClipboard();
        } else {
            GlobalFree(memory);
        }
    }

    void SetInfiniteSprint(bool enable) {
        GameLogic::SetInfiniteSprint(enable);
    }

    bool RequestSaveGame() {
        return GameLogic::RequestSaveGame();
    }

    void MoveForward(float distance) {
        GameLogic::MovePlayerRelative(GetPlayer(), distance, 0.0f, 0.0f);
    }

    void MoveUp(float distance) {
        GameLogic::MovePlayerRelative(GetPlayer(), 0.0f, 0.0f, distance);
    }

    void MoveDown(float distance) {
        GameLogic::MovePlayerRelative(GetPlayer(), 0.0f, 0.0f, -distance);
    }

    void SetKeepStuff(bool enable) {
        GameLogic::SetKeepStuff(enable);
    }

    bool GetFreeHealthcare() {
        return GameLogic::GetFreeHealthcare();
    }

    void SetFreeHealthcare(bool enable) {
        GameLogic::SetFreeHealthcare(enable);
    }

    bool GetFreeJail() {
        return GameLogic::GetFreeJail();
    }

    void SetFreeJail(bool enable) {
        GameLogic::SetFreeJail(enable);
    }

    bool SetSkin(unsigned int modelId) {
        return GameLogic::SetPlayerSkin(modelId);
    }

    bool SetCustomSkin(const char* name) {
        return GameLogic::SetPlayerCustomSkin(name);
    }

    bool ApplyClothes(int textureId, int modelId, int bodyPart) {
        return GameLogic::ApplyPlayerClothes(textureId, modelId, bodyPart);
    }

    bool SetStat(int statId, float value) {
        return GameLogic::SetPlayerStat(statId, value);
    }

    void MaxWeaponSkills() {
        GameLogic::MaxWeaponSkills();
    }

    void MaxVehicleSkills() {
        GameLogic::MaxVehicleSkills();
    }

    void ApplyAimSkinChanger() {
        GameLogic::ApplyAimSkinChanger();
    }
}