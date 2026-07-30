#include "Scene.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"

#ifdef GTASA
#include "cutscene_sa.h"
#include "particle_sa.h"
#endif

namespace Controllers::Scene {
    void Process() {
    }

    bool PlayPlayerAnimation(const char* group, const char* name, bool loop) {
        const bool ok = GameLogic::PlayAnimationEx(group, name, loop, false, false);
        MenuState::ShowNotice(ok ? "Animation started" : "Animation unavailable", 1.8);
        return ok;
    }

    bool PlayPlayerAnimation() {
        const bool ok = GameLogic::PlayAnimationEx(
            MenuState::SceneAnimGroup,
            MenuState::SceneAnimName,
            MenuState::SceneAnimLoop,
            MenuState::SceneAnimSecondary,
            MenuState::SceneAnimOnPed);
        MenuState::ShowNotice(ok ? "Animation started" : "Animation unavailable", 1.8);
        return ok;
    }

    void StopPlayerAnimation() {
        GameLogic::StopPlayerAnimation();
        MenuState::ShowNotice("Animation stopped", 1.5);
    }

    bool SpawnParticleAtPlayer(const char* name) {
#ifdef GTASA
        Particle.Play(name);
        return true;
#else
        return GameLogic::SpawnParticleAtPlayer(name);
#endif
    }

    bool SpawnParticleAtPlayer() {
#ifdef GTASA
        Particle.Play(MenuState::SceneParticleName);
        MenuState::ShowNotice("Particle spawned", 1.8);
        return true;
#else
        const bool ok = GameLogic::SpawnParticleAtPlayer(MenuState::SceneParticleName);
        MenuState::ShowNotice(ok ? "Particle spawned" : "Particle unavailable", 1.8);
        return ok;
#endif
    }

    bool StartCutscene(const char* name) {
#ifdef GTASA
        Cutscene.Play(name, MenuState::SceneCutsceneInterior);
        return true;
#else
        return GameLogic::StartCutscene(name);
#endif
    }

    bool StartCutscene() {
#ifdef GTASA
        Cutscene.Play(MenuState::SceneCutsceneName, MenuState::SceneCutsceneInterior);
        return true;
#else
        const bool ok = GameLogic::StartCutscene(MenuState::SceneCutsceneName);
        MenuState::ShowNotice(ok ? "Cutscene started" : "Cutscene unavailable", 1.8);
        return ok;
#endif
    }

    void StopCutscene() {
#ifdef GTASA
        Cutscene.Stop();
#else
        GameLogic::StopCutscene();
#endif
    }

    bool IsCutsceneRunning() {
#ifdef GTASA
        return Cutscene.IsRunning();
#else
        return GameLogic::IsCutsceneRunning();
#endif
    }

    const char* GetMissionStatus() {
        return GameLogic::GetMissionStatus();
    }

    void StartMission(int missionId) {
        GameLogic::StartMission(missionId);
    }

    void FailMission() {
        GameLogic::FailMission();
    }

    void SetFightingStyle(int styleIndex) {
        GameLogic::SetFightingStyle(styleIndex);
    }

    void SetWalkingStyle(int styleIndex) {
        GameLogic::SetWalkingStyle(styleIndex);
    }

    void RemoveAllParticles() {
#ifdef GTASA
        Particle.RemoveAll();
#endif
    }

    void RemoveLatestParticle() {
#ifdef GTASA
        Particle.RemoveLatest();
#endif
    }
}