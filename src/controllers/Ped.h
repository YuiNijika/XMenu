#pragma once

class CPed;

namespace Controllers::Ped {
    CPed* GetLastSpawnedPed();
    bool SpawnNearPlayer();
    bool SpawnAtMarker();
    void DeleteLastSpawnedPed();
    void Process();
}