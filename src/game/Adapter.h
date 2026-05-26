#pragma once
#include "game/Runtime.h"
#include "game/Types.h"

namespace GameAdapter {
    struct Capabilities {
        bool menu = false;
        bool d3dHook = false;
        bool player = false;
        bool vehicle = false;
        bool weapon = false;
        bool world = false;
        bool teleport = false;
    };

    class IGameAdapter {
    public:
        virtual ~IGameAdapter() = default;
        virtual const char* Name() const = 0;
        virtual GameRuntime::Target Target() const = 0;
        virtual Capabilities GetCapabilities() const = 0;
        virtual bool Attach() = 0;
        virtual void Detach() = 0;
        virtual void OnGameInit() = 0;
        virtual void OnProcess() = 0;
        virtual GameTypes::PlayerState GetPlayerState() = 0;
        virtual GameTypes::ProofState GetPlayerProofState() = 0;
        virtual void SetPlayerProofState(const GameTypes::ProofState& state) = 0;
        virtual void SetPlayerInvincible(bool enable) = 0;
        virtual void SetPlayerHealth(float value) = 0;
        virtual void SetPlayerArmour(float value) = 0;
        virtual void SetWantedLevel(int level) = 0;
        virtual void GiveMoney(int amount) = 0;
        virtual void SetMoney(int amount) = 0;
        virtual void KillPlayer() = 0;
    };

    bool Bind(GameRuntime::Target target);
    IGameAdapter* Active();
    const Capabilities& ActiveCapabilities();
    void Detach();
}