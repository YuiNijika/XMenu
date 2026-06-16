#pragma once
#include "game/Types.h"

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
    GameTypes::ProofState GetProofState();
    void SetProofState(const GameTypes::ProofState& state);
    void CopyCoordinates();
    bool RequestSaveGame();
    void MoveForward(float distance);
    void MoveUp(float distance);
    void MoveDown(float distance);
    void SetInfiniteSprint(bool enable);
    void SetKeepStuff(bool enable);
    bool GetFreeHealthcare();
    void SetFreeHealthcare(bool enable);
    bool GetFreeJail();
    void SetFreeJail(bool enable);
    bool SetSkin(unsigned int modelId);
    bool SetCustomSkin(const char* name);
    bool ApplyClothes(int textureId, int modelId, int bodyPart);
    bool SetStat(int statId, float value);
}