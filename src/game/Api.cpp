#include "Api.h"
#include "game/Adapter.h"

namespace {
    GameAdapter::IGameAdapter* Adapter() {
        return GameAdapter::Active();
    }
}

namespace GameApi {
    bool IsReady() {
        return Adapter() != nullptr;
    }

    GameTypes::PlayerState GetPlayerState() {
        if (auto* adapter = Adapter()) {
            return adapter->GetPlayerState();
        }
        return GameTypes::PlayerState{};
    }

    GameTypes::ProofState GetPlayerProofState() {
        if (auto* adapter = Adapter()) {
            return adapter->GetPlayerProofState();
        }
        return GameTypes::ProofState{};
    }

    void SetPlayerProofState(const GameTypes::ProofState& state) {
        if (auto* adapter = Adapter()) {
            adapter->SetPlayerProofState(state);
        }
    }

    void SetPlayerInvincible(bool enable) {
        if (auto* adapter = Adapter()) {
            adapter->SetPlayerInvincible(enable);
        }
    }

    void SetPlayerHealth(float value) {
        if (auto* adapter = Adapter()) {
            adapter->SetPlayerHealth(value);
        }
    }

    void SetPlayerArmour(float value) {
        if (auto* adapter = Adapter()) {
            adapter->SetPlayerArmour(value);
        }
    }

    void SetWantedLevel(int level) {
        if (auto* adapter = Adapter()) {
            adapter->SetWantedLevel(level);
        }
    }

    void GiveMoney(int amount) {
        if (auto* adapter = Adapter()) {
            adapter->GiveMoney(amount);
        }
    }

    void SetMoney(int amount) {
        if (auto* adapter = Adapter()) {
            adapter->SetMoney(amount);
        }
    }

    void KillPlayer() {
        if (auto* adapter = Adapter()) {
            adapter->KillPlayer();
        }
    }
}