#include "Scene.h"
#include <XBase/Scene.h>
#include "ui/MenuState.h"

#include <cerrno>
#include <climits>
#include <cstdlib>

namespace Controllers::Scene {
    namespace {
        bool ParseInterior(const char* value, int& interior) {
            if (!value || !value[0]) {
                interior = 0;
                return true;
            }

            errno = 0;
            char* end = nullptr;
            const long parsed = std::strtol(value, &end, 10);
            if (errno != 0 || end == value || *end != '\0' || parsed < 0 || parsed > UCHAR_MAX) {
                return false;
            }
            interior = static_cast<int>(parsed);
            return true;
        }
    }

    bool PlayPlayerAnimation() {
        const bool ok = XBase::Scene::PlayAnimation(
            MenuState::SceneAnimGroup,
            MenuState::SceneAnimName,
            MenuState::SceneAnimLoop);
        MenuState::ShowNotice(ok ? "Animation started" : "Animation unavailable", 1.8);
        return ok;
    }

    void StopPlayerAnimation() {
        XBase::Scene::StopAnimation();
        MenuState::ShowNotice("Animation stopped", 1.5);
    }

    bool SpawnParticleAtPlayer() {
        const bool ok = XBase::Scene::PlayParticle(MenuState::SceneParticleName);
        MenuState::ShowNotice(ok ? "Particle spawned" : "Particle unavailable", 1.8);
        return ok;
    }

    bool StartCutscene() {
        int interior = 0;
        const bool ok = ParseInterior(MenuState::SceneCutsceneInterior, interior)
            && XBase::Scene::StartCutscene(MenuState::SceneCutsceneName, interior);
        MenuState::ShowNotice(ok ? "Cutscene started" : "Cutscene unavailable", 1.8);
        return ok;
    }

    void StopCutscene() {
        XBase::Scene::StopCutscene();
    }

    bool IsCutsceneRunning() {
        return XBase::Scene::IsCutsceneRunning();
    }

    const char* GetMissionStatus() {
        return XBase::Scene::GetMissionStatus();
    }

    bool StartMission(int missionId) {
        const bool ok = XBase::Scene::StartMission(missionId);
        MenuState::ShowNotice(ok ? "Mission started" : "Mission unavailable", 1.8);
        return ok;
    }

    void FailMission() {
        XBase::Scene::FailMission();
    }

    void SetFightingStyle(int styleIndex) {
        XBase::Scene::SetFightingStyle(styleIndex);
    }

    void SetWalkingStyle(int styleIndex) {
        XBase::Scene::SetWalkingStyle(styleIndex);
    }

    void RemoveAllParticles() {
        XBase::Scene::RemoveAllParticles();
    }

    void RemoveLatestParticle() {
        XBase::Scene::RemoveLatestParticle();
    }
}