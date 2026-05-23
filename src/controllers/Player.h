#pragma once
#include "features/GameLogic.h"

class CPlayerPed;

namespace Controllers::Player {
    CPlayerPed* GetPlayer();
    void Process();
    void Heal();
    void GiveArmour();
    void GiveMoney();
    void Kill();
    int GetWantedLevel();
    void SetWantedLevel(int level);
    void ClearWantedLevel();
    int GetMoney();
    void SetMoney(int amount);
    float GetHealth();
    void SetHealth(float value);
    float GetArmour();
    void SetArmour(float value);
    GameLogic::ProofState GetProofState();
    void SetProofState(const GameLogic::ProofState& state);
    void CopyCoordinates();
    void SetInfiniteSprint(bool enable);
    void SetKeepStuff(bool enable);
    bool GetFreeHealthcare();
    void SetFreeHealthcare(bool enable);
    bool GetFreeJail();
    void SetFreeJail(bool enable);
}