#pragma once

class CPed;

namespace Controllers::Ped {
    CPed* GetLastSpawnedPed();
    bool SpawnNearPlayer();
    bool SpawnAtMarker();
    void DeleteLastSpawnedPed();
    void Process();
    void SetElvisEverywhere(bool enable);
    void SetEveryoneArmed(bool enable);
    void SetPedsMayhem(bool enable);
    void SetPedsAtkRocket(bool enable);
    void SetPedsRiot(bool enable);
    void SetSlutMagnet(bool enable);
    void SetGangsControl(bool enable);
    void SetGangsEverywhere(bool enable);
}