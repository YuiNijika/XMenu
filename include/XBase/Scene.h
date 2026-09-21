#pragma once

namespace XBase::Scene {

struct AnimationOptions {
    bool loop = false;
    bool secondary = false;
    bool onTargetPed = false;
};

void Process();
void NotifyGameInit();
void Shutdown();
bool PlayAnimation(const char* group, const char* name, bool loop);
bool PlayAnimation(const char* group, const char* name, const AnimationOptions& options);
bool StopAnimation();
bool PlayParticle(const char* name);
bool RemoveAllParticles();
bool RemoveLatestParticle();
bool StartCutscene(const char* name);
bool StartCutscene(const char* name, int interior);
bool StopCutscene();
bool IsCutsceneRunning();

const char* GetMissionStatus();
bool FailMission();
bool StartMission(int missionId);
bool SetFightingStyle(int style);
bool SetWalkingStyle(int style);

} // namespace XBase::Scene
