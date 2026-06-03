#pragma once
#include <string>

namespace Controllers::Scene {
    void Process();
    bool PlayPlayerAnimation();
    void StopPlayerAnimation();
    bool SpawnParticleAtPlayer();
    bool StartCutscene();
    void StopCutscene();
    bool IsCutsceneRunning();
    const char* GetMissionStatus();

    void RemoveAllParticles();
    void RemoveLatestParticle();
}