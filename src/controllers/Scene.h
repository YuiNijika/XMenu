#pragma once
#include "features/GameLogic.h"
#include <cstring>

namespace Controllers::Scene {
    void Process();
    bool PlayPlayerAnimation(const char* group, const char* name, bool loop);
    bool PlayPlayerAnimation();
    void StopPlayerAnimation();
    bool SpawnParticleAtPlayer(const char* name);
    bool SpawnParticleAtPlayer();
    bool StartCutscene(const char* name);
    bool StartCutscene();
    void StopCutscene();
    bool IsCutsceneRunning();
    const char* GetMissionStatus();
    void RemoveAllParticles();
    void RemoveLatestParticle();
    void FailMission();
    void StartMission(int missionId);
    void SetFightingStyle(int styleIndex);
    void SetWalkingStyle(int styleIndex);
}