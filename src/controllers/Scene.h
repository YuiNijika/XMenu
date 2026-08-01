#pragma once

namespace Controllers::Scene {
    bool PlayPlayerAnimation();
    void StopPlayerAnimation();
    bool SpawnParticleAtPlayer();
    bool StartCutscene();
    void StopCutscene();
    bool IsCutsceneRunning();
    const char* GetMissionStatus();
    void RemoveAllParticles();
    void RemoveLatestParticle();
    void FailMission();
    void SetFightingStyle(int styleIndex);
    void SetWalkingStyle(int styleIndex);
}