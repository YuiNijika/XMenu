#pragma once

#include "Types.h"
#include "ValueTypes.h"

namespace XBase::Player {

struct RuntimeOptions {
    bool godMode = false;
    bool invisible = false;
    bool hardMode = false;
    bool autoHeal = false;
    bool respawnAtDeathPosition = false;
    bool freezeWantedLevel = false;
    int wantedLevel = 0;
    bool freeFlyProtection = false;
};

struct PlayerSnapshot {
    bool valid = false;
    Vec3 position;
    float health = 0.0f;
    float armour = 0.0f;
    int money = 0;
    int wantedLevel = 0;
    Types::ProofState proofs;
};

bool IsAvailable();
PlayerSnapshot GetSnapshot();
void Process();
void NotifyGameInit();
void Shutdown();
void SetRuntimeOptions(const RuntimeOptions& options);
RuntimeOptions GetRuntimeOptions();
bool MoveRelative(float forward, float right, float up);

void Heal();
void GiveArmour();
void GiveMoney(int amount);
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
Types::ProofState GetProofState();
void SetProofState(const Types::ProofState& state);

bool SetSkin(unsigned int modelId);
bool SetCustomSkin(const char* txdName);
bool ApplyClothes(int textureId, int modelId, int bodyPart);
void SetInfiniteSprint(bool enable);
bool SetKeepStuff(bool enable);
void SetFreeHealthcare(bool enable);
bool GetFreeHealthcare();
void SetFreeJail(bool enable);
bool GetFreeJail();
bool MaxVehicleSkills();
bool SetStat(int statId, float value);

bool SuperJump(bool enable);
bool SuperPunch(bool enable);
bool UnderwaterBreathing(bool enable);
bool SetCycleJump(bool enable);
bool SetNeverHungry(bool enable);
bool SetFastSprint(bool enable);
bool SetSprintEverywhere(bool enable);
bool SetDrunkEffect(bool enable);
bool SetNeverWanted(bool enable);
void SetGodMode(bool enable);
bool IsGodMode();
void SetInvisible(bool enable);
bool IsInvisible();
void SetHardMode(bool enable);
bool IsHardMode();
void SetFreeFly(bool enable);
bool IsFreeFly();

void CopyCoordinates();
bool RequestSaveGame();
void MoveForward(float distance);
void MoveUp(float distance);
void MoveDown(float distance);
bool ApplyAimSkinChanger();

} // namespace XBase::Player
