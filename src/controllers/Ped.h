#pragma once

#include <XBase/Ped.h>
#include <XBase/Types.h>

namespace Controllers::Ped {
    using SpawnOptions = XBase::Types::PedSpawnOptions;

    XBase::PedId GetLastSpawnedId();
    bool SpawnNearPlayer();
    bool SpawnAtMarker();
    void DeleteLastSpawnedPed();
    void SetBigHead(bool enable);
    void SetThinBody(bool enable);
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