#include "Player.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "plugin.h"
#include "CPlayerPed.h"

namespace {
    CPlayerPed* godModePlayer = nullptr;
    GameLogic::ProofState savedPlayerProofs;
    bool hasSavedPlayerProofs = false;

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
        return GameLogic::GetWantedLevel(GetPlayer());
    }

    void SetWantedLevel(int level) {
        GameLogic::SetWantedLevel(GetPlayer(), level);
    }

    void ClearWantedLevel() {
        SetWantedLevel(0);
    }

    void SetInfiniteSprint(bool enable) {
        GameLogic::SetInfiniteSprint(enable);
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