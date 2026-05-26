#include "Player.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "plugin.h"
#include "CPlayerPed.h"
#include <cstdio>
#include <cstring>
#include <windows.h>

namespace {
    CPlayerPed* godModePlayer = nullptr;
    GameTypes::ProofState savedPlayerProofs;
    bool hasSavedPlayerProofs = false;

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
        if (!hasSavedPlayerProofs || !godModePlayer) {
            return;
        }

        GameLogic::SetPlayerProofState(godModePlayer, ToLogicProof(savedPlayerProofs));
        godModePlayer = nullptr;
        hasSavedPlayerProofs = false;
    }

    void ProcessGodMode(CPlayerPed* player) {
        if (!MenuState::GodMode) {
            RestoreGodModeState();
            return;
        }

        if (!player) {
            return;
        }

        if (!hasSavedPlayerProofs || godModePlayer != player) {
            RestoreGodModeState();
            savedPlayerProofs = ToUiProof(GameLogic::GetPlayerProofState(player));
            godModePlayer = player;
            hasSavedPlayerProofs = true;
        }

        GameLogic::ApplyGodMode(player, true);
    }
}

namespace Controllers::Player {
    CPlayerPed* GetPlayer() {
        return FindPlayerPed();
    }

    void Process() {
        CPlayerPed* player = GetPlayer();
        ProcessGodMode(player);

        GameLogic::SetInfiniteSprint(MenuState::InfiniteSprint);
        GameLogic::SetKeepStuff(MenuState::KeepStuff);

        if (!player) {
            return;
        }

        GameLogic::ProcessAutoHeal(player, MenuState::AutoHeal);
        GameLogic::ProcessHardMode(player, MenuState::HardMode);
        GameLogic::ProcessRespawnAtDeathPosition(player, MenuState::RespawnAtDeathPosition);
        GameLogic::ProcessFreezeWantedLevel(player, MenuState::FreezeWantedLevel, ClampWantedLevel(MenuState::WantedLevel));
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

    bool ApplyClothes(int textureId, int modelId, int bodyPart) {
        return GameLogic::ApplyPlayerClothes(textureId, modelId, bodyPart);
    }

    bool SetStat(int statId, float value) {
        return GameLogic::SetPlayerStat(statId, value);
    }
}