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

    bool PlayPlayerAnimation() {
        const bool ok = GameLogic::PlayPlayerAnimation(MenuState::SceneAnimGroup, MenuState::SceneAnimName, MenuState::SceneAnimLoop);
        MenuState::ShowNotice(ok ? "Animation started" : "Animation unavailable", 1.8);
        return ok;
    }

    void StopPlayerAnimation() {
        GameLogic::StopPlayerAnimation();
        MenuState::ShowNotice("Animation stopped", 1.5);
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