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
    GameLogic::ProofState savedPlayerProofs;
    bool hasSavedPlayerProofs = false;

    int ClampWantedLevel(int level) {
        if (level < 0 || level > 6) {
            return 0;
        }
        return level;
    }

    float ClampHealth(float value) {
        if (value <= 0.0f) {
            return 0.0f;
        }
        return value < 2.0f ? 2.0f : value;
    }

    void RestoreGodModeState() {
        if (!hasSavedPlayerProofs || !godModePlayer) {
            return;
        }

        GameLogic::SetPlayerProofState(godModePlayer, savedPlayerProofs);
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

        // 换角色或重新进档时先留一份原状态，关闭后尽量放回玩家原来的防护配置。
        if (!hasSavedPlayerProofs || godModePlayer != player) {
            RestoreGodModeState();
            savedPlayerProofs = GameLogic::GetPlayerProofState(player);
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

        if (!player) {
            return;
        }

        GameLogic::ProcessAutoHeal(player, MenuState::AutoHeal);
        GameLogic::ProcessHardMode(player, MenuState::HardMode);
        GameLogic::ProcessRespawnAtDeathPosition(player, MenuState::RespawnAtDeathPosition);
        GameLogic::ProcessFreezeWantedLevel(player, MenuState::FreezeWantedLevel);
    }

    void Heal() {
        GameLogic::HealPlayer(GetPlayer());
    }

    void GiveArmour() {
        GameLogic::GiveArmour(GetPlayer());
    }

    void GiveMoney() {
        GameLogic::GiveMoney(250000);
    }

    void Kill() {
        CPlayerPed* player = GetPlayer();
        if (player) {
            player->m_fHealth = 0.0f;
        }
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

    GameLogic::ProofState GetProofState() {
        return GameLogic::GetPlayerProofState(GetPlayer());
    }

    void SetProofState(const GameLogic::ProofState& state) {
        if (!MenuState::GodMode) {
            GameLogic::SetManualPlayerProof(GetPlayer(), state);
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
}