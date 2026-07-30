#pragma once

#include "features/GameLogic.h"

class CPed;

namespace Controllers::Ped {
    using SpawnOptions = GameLogic::PedSpawnOptions;

    CPed* GetLastSpawnedPed();
    bool GetNoFire();
    void SetSpawnLimits(bool limitPolice, bool limitGangs, int maxPolice, int maxGangs);
    bool SpawnNearPlayer(const SpawnOptions& options);
    bool SpawnAtMarker(const SpawnOptions& options);
    bool SpawnNearPlayer();
    bool SpawnAtMarker();
    void DeleteLastSpawnedPed();
    void Process();
    void SetElvisEverywhere(bool enable);
    void SetEveryoneArmed(bool enable);
    void SetPedsMayhem(bool enable);
    void SetPedsAtkRocket(bool enable);
    void SetPedsRiot(bool enable);
    void SetPedsNoFire(bool enable);
    void SetSlutMagnet(bool enable);
    void SetGangsControl(bool enable);
    void SetGangsEverywhere(bool enable);
    void SetNoProstitutes(bool enable);
    void SetNastyLimbs(bool enable);
    void SetGangWarsActive(bool enable);
    void StartGangWar(bool offensive);
    void EndGangWar();
    int GetGangZoneDensity(int gangId);
    void SetGangZoneDensity(int gangId, int density);
    unsigned int GetGangMemberModel(unsigned int gangId, unsigned int memberId);
    void SetGangMemberModel(unsigned int gangId, unsigned int memberId, unsigned int model);
    void ResetGangModels();
    void SetGangWeapons(unsigned int gangId, int weapon1, int weapon2, int weapon3);
}