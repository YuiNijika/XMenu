#include "Scene.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"

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
        const bool ok = GameLogic::SpawnParticleAtPlayer(MenuState::SceneParticleName);
        MenuState::ShowNotice(ok ? "Particle spawned" : "Particle unavailable", 1.8);
        return ok;
    }

    bool StartCutscene() {
        const bool ok = GameLogic::StartCutscene(MenuState::SceneCutsceneName);
        MenuState::ShowNotice(ok ? "Cutscene started" : "Cutscene unavailable", 1.8);
        return ok;
    }

    void StopCutscene() {
        GameLogic::StopCutscene();
    }

    bool IsCutsceneRunning() {
        return GameLogic::IsCutsceneRunning();
    }

    const char* GetMissionStatus() {
        return GameLogic::GetMissionStatus();
    }
}