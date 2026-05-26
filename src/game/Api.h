#pragma once
#include "game/Types.h"

namespace GameApi {
    bool IsReady();

    GameTypes::PlayerState GetPlayerState();
    GameTypes::ProofState GetPlayerProofState();
    void SetPlayerProofState(const GameTypes::ProofState& state);
    void SetPlayerInvincible(bool enable);
    void SetPlayerHealth(float value);
    void SetPlayerArmour(float value);
    void SetWantedLevel(int level);
    void GiveMoney(int amount);
    void SetMoney(int amount);
    void KillPlayer();
}