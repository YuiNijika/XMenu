#pragma once

#include "Types.h"
#include "ValueTypes.h"

namespace XBase::Ped {

struct PedSnapshot {
    bool valid = false;
    PedId id;
    unsigned int modelId = 0;
    Vec3 position;
    float health = 0.0f;
    float armour = 0.0f;
    bool player = false;
    bool mission = false;
    bool cop = false;
    bool gang = false;
};

void Init();
void Process();
void NotifyGameInit();
void Shutdown();
void SetNoFire(bool enable);
bool GetNoFire();
void SetSpawnLimits(bool limitPolice, bool limitGangs, int maxPolice, int maxGangs);
void SetSmokingEffect(bool enable);
void SetFliesEffect(bool enable);

PedId GetLastSpawnedId();
PedSnapshot GetLastSpawnedSnapshot();
bool SpawnNearPlayer(unsigned int modelId, const Types::PedSpawnOptions& options);
bool SpawnAtMarker(unsigned int modelId, const Types::PedSpawnOptions& options);
void DeleteLastSpawned();

void SetElvisEverywhere(bool enable);
bool IsElvisEverywhere();
void SetEveryoneArmed(bool enable);
bool IsEveryoneArmed();
void SetPedsMayhem(bool enable);
bool IsPedsMayhem();
void SetPedsAtkRocket(bool enable);
bool IsPedsAtkRocket();
void SetPedsRiot(bool enable);
bool IsPedsRiot();
void SetSlutMagnet(bool enable);
bool IsSlutMagnet();
void SetBigHead(bool enable);
bool IsBigHead();
void SetThinBody(bool enable);
bool IsThinBody();
void SetNastyLimbs(bool enable);
bool IsNastyLimbs();
void SetNoProstitutes(bool enable);
bool IsNoProstitutes();

void SetGangsControl(bool enable);
bool IsGangsControl();
void SetGangsEverywhere(bool enable);
bool IsGangsEverywhere();
void SetGangWarsActive(bool enable);
bool IsGangWarsActive();
void StartGangWar(bool offensive);
void EndGangWar();
int GetGangZoneDensity(int gangId);
void SetGangZoneDensity(int gangId, int density);
unsigned int GetGangMemberModel(unsigned int gangId, unsigned int slot);
void SetGangMemberModel(unsigned int gangId, unsigned int slot, unsigned int modelId);
void ResetGangModels();
void SetGangWeapons(unsigned int gangId, int weapon1, int weapon2, int weapon3);

} // namespace XBase::Ped
