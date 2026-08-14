#include "Scene.h"
#include "controllers/Scene.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/DataManager.h"
#include "utils/I18n.h"
#include "utils/JsonLoader.h"
#include <XBase/Capabilities.h>
#include <XBase/UI.h>
#include "integration/XBaseBridge.h"
#include "resources/ResourceData.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {
    struct AnimationEntry {
        std::string category;
        std::string name;
        std::string value;
        std::string group;
    };

    struct NamedValueEntry {
        std::string category;
        std::string name;
        std::string value;
        std::string metadata;
    };

    template <typename T>
    struct LiveList {
        std::vector<T> entries;
        std::filesystem::file_time_type lastWrite{};
        bool loaded = false;
    };

    const char* T(const char* key) {
        return I18n::T(key);
    }

    bool ShouldReload(const std::string& path, std::filesystem::file_time_type& lastWrite, bool loaded) {
        std::error_code ec;
        const auto currentWrite = std::filesystem::exists(path, ec) ? std::filesystem::last_write_time(path, ec) : std::filesystem::file_time_type{};
        if (ec) {
            return !loaded;
        }
        if (!loaded || currentWrite != lastWrite) {
            lastWrite = currentWrite;
            return true;
        }
        return false;
    }

    void LoadAnimations(LiveList<AnimationEntry>& list) {
        const std::string path = DataManager::GetDataFilePath("animations.json");
        if (!ShouldReload(path, list.lastWrite, list.loaded)) {
            return;
        }

        list.loaded = true;
        list.entries.clear();
        const JsonLoader::JsonValue data = JsonLoader::LoadFromFile(path);
        if (data.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        for (const auto& category : JsonLoader::GetArray(data, "animations")) {
            if (category.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }
            const std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
            for (const auto& entry : JsonLoader::GetArray(category, "entries")) {
                if (entry.type != JsonLoader::JsonValue::OBJECT) {
                    continue;
                }
                AnimationEntry animation;
                animation.category = categoryName;
                animation.name = JsonLoader::GetString(entry, "name", "unknown");
                animation.value = JsonLoader::GetString(entry, "value", animation.name);
                animation.group = JsonLoader::GetString(entry, "group", categoryName);
                list.entries.push_back(animation);
            }
        }
    }

    void LoadNamedValues(LiveList<NamedValueEntry>& list, const char* filename, const char* rootKey, const char* valueKey) {
        const std::string path = DataManager::GetDataFilePath(filename);
        if (!ShouldReload(path, list.lastWrite, list.loaded)) {
            return;
        }

        list.loaded = true;
        list.entries.clear();
        const JsonLoader::JsonValue data = JsonLoader::LoadFromFile(path);
        if (data.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        for (const auto& category : JsonLoader::GetArray(data, rootKey)) {
            if (category.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }
            const std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
            for (const auto& entry : JsonLoader::GetArray(category, "entries")) {
                if (entry.type != JsonLoader::JsonValue::OBJECT) {
                    continue;
                }
                NamedValueEntry item;
                item.category = categoryName;
                item.name = JsonLoader::GetString(entry, "name", "unknown");
                item.value = JsonLoader::GetString(entry, "value", JsonLoader::GetString(entry, valueKey, ""));
                if (item.value.empty()) {
                    item.value = JsonLoader::GetString(entry, valueKey, "");
                }
                item.metadata = JsonLoader::GetString(entry, valueKey, "");
                list.entries.push_back(item);
            }
        }
    }

    void DrawMissionList() {
        XBase::UI::TextDisabled(T("scene.noMissions"));
    }
    void DrawAnimationList() {
        static LiveList<AnimationEntry> animations;
        LoadAnimations(animations);
        if (animations.entries.empty()) {
            XBase::UI::TextDisabled(T("scene.noListData"));
            return;
        }

        std::string currentCategory;
        int index = 0;
        for (const auto& animation : animations.entries) {
            if (currentCategory != animation.category) {
                currentCategory = animation.category;
                index = 0;
                XBase::UI::Spacing();
                XBase::UI::SeparatorText(T(currentCategory.c_str()));
            }

            const char* translatedName = T(animation.name.c_str());
            char label[128];
            std::snprintf(label, sizeof(label), "%s##anim_%s_%s", translatedName, animation.group.c_str(), animation.value.c_str());
            if (UI::Button(label, 3)) {
                std::snprintf(MenuState::SceneAnimGroup, sizeof(MenuState::SceneAnimGroup), "%s", animation.group.c_str());
                std::snprintf(MenuState::SceneAnimName, sizeof(MenuState::SceneAnimName), "%s", animation.value.c_str());
                Controllers::Scene::PlayPlayerAnimation();
            }
            UI::SameLineEvery(index++, 3);
        }
    }

    void DrawNamedValueList(LiveList<NamedValueEntry>& list, const char* emptyKey, const char* labelPrefix, void (*apply)(const NamedValueEntry&)) {
        if (list.entries.empty()) {
            XBase::UI::TextDisabled(T(emptyKey));
            return;
        }

        std::string currentCategory;
        int index = 0;
        for (const auto& entry : list.entries) {
            if (currentCategory != entry.category) {
                currentCategory = entry.category;
                index = 0;
                XBase::UI::Spacing();
                XBase::UI::SeparatorText(T(currentCategory.c_str()));
            }

            const char* translatedName = T(entry.name.c_str());
            char label[128];
            std::snprintf(label, sizeof(label), "%s##%s_%s_%s", translatedName, labelPrefix, entry.category.c_str(), entry.value.c_str());
            if (UI::Button(label, 3)) {
                apply(entry);
            }
            UI::SameLineEvery(index++, 3);
        }
    }

    void ApplyParticle(const NamedValueEntry& entry) {
        std::snprintf(MenuState::SceneParticleName, sizeof(MenuState::SceneParticleName), "%s", entry.value.c_str());
        Controllers::Scene::SpawnParticleAtPlayer();
    }

    void ApplyCutscene(const NamedValueEntry& entry) {
        std::snprintf(MenuState::SceneCutsceneName, sizeof(MenuState::SceneCutsceneName), "%s", entry.value.c_str());
        std::snprintf(
            MenuState::SceneCutsceneInterior,
            sizeof(MenuState::SceneCutsceneInterior),
            "%s",
            entry.metadata.empty() ? "0" : entry.metadata.c_str());
        Controllers::Scene::StartCutscene();
    }
}

namespace Pages::Scene {
    void Draw() {
        XBase::UI::Tabs("SceneTabs", [&] {
            XBase::UI::Tab("scene.animation", T("scene.animation"), [&] {
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::SceneAnimation), [&] {
                XBase::UI::InputText(T("scene.animGroup"), MenuState::SceneAnimGroup, sizeof(MenuState::SceneAnimGroup));
                XBase::UI::InputText(T("scene.animName"), MenuState::SceneAnimName, sizeof(MenuState::SceneAnimName));
                XBase::UI::Checkbox(T("scene.loop"), MenuState::SceneAnimLoop);
#ifdef GTASA
                XBase::UI::SameLine();
                XBase::UI::Checkbox(T("scene.secondary"), MenuState::SceneAnimSecondary);
                XBase::UI::SameLine();
                XBase::UI::Checkbox(T("scene.onTargetPed"), MenuState::SceneAnimOnPed);
#endif
                if (UI::Button(T("scene.playAnim"), 2)) {
                    Controllers::Scene::PlayPlayerAnimation();
                }
                XBase::UI::SameLine();
                if (UI::Button(T("scene.stopAnim"), 2)) {
                    Controllers::Scene::StopPlayerAnimation();
                }
                UI::SpacingSeparator();
                XBase::UI::TextWrapped(T("scene.animationListHint"));
                DrawAnimationList();
                });
                });

#ifdef GTASA
            XBase::UI::Tab("scene.styles", T("scene.styles"), [&] {
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::SceneAnimation), [&] {
                UI::PushItemWidth(180);
                UI::SliderInt(T("scene.fightStyle"), &MenuState::SceneFightStyle, 0, 15);
                UI::SliderInt(T("scene.walkStyle"), &MenuState::SceneWalkStyle, 0, 21);
                UI::PopItemWidth();
                if (UI::Button(T("scene.applyFightStyle"), 2)) {
                    Controllers::Scene::SetFightingStyle(MenuState::SceneFightStyle);
                }
                XBase::UI::SameLine();
                if (UI::Button(T("scene.applyWalkStyle"), 2)) {
                    Controllers::Scene::SetWalkingStyle(MenuState::SceneWalkStyle);
                }
                });
                });
#endif

            XBase::UI::Tab("scene.particle", T("scene.particle"), [&] {
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::SceneParticle), [&] {
                static LiveList<NamedValueEntry> particles;
                LoadNamedValues(particles, "particles.json", "particles", "effect");
                XBase::UI::InputText(T("scene.particleName"), MenuState::SceneParticleName, sizeof(MenuState::SceneParticleName));
                if (UI::Button(T("scene.spawnParticle"))) {
                    Controllers::Scene::SpawnParticleAtPlayer();
                }
                XBase::UI::SameLine();
                if (UI::Button(T("scene.removeLastParticle"))) {
                    Controllers::Scene::RemoveLatestParticle();
                }
                XBase::UI::SameLine();
                if (UI::Button(T("scene.removeAllParticles"))) {
                    Controllers::Scene::RemoveAllParticles();
                }
                UI::SpacingSeparator();
                XBase::UI::TextWrapped(T("scene.particleListHint"));
                DrawNamedValueList(particles, "scene.noListData", "particle", ApplyParticle);
                });
                });

            XBase::UI::Tab("scene.cutscene", T("scene.cutscene"), [&] {
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::SceneCutscene), [&] {
                static LiveList<NamedValueEntry> cutscenes;
                LoadNamedValues(cutscenes, "cutscenes.json", "cutscenes", "interior");
                XBase::UI::InputText(T("scene.cutsceneName"), MenuState::SceneCutsceneName, sizeof(MenuState::SceneCutsceneName));
                XBase::UI::InputText(T("scene.interiorId"), MenuState::SceneCutsceneInterior, sizeof(MenuState::SceneCutsceneInterior));
                if (UI::Button(T("scene.startCutscene"), 2)) {
                    Controllers::Scene::StartCutscene();
                }
                XBase::UI::SameLine();
                if (UI::Button(T("scene.stopCutscene"), 2)) {
                    Controllers::Scene::StopCutscene();
                }
                XBase::UI::Text("%s: %s", T("scene.cutsceneRunning"), Controllers::Scene::IsCutsceneRunning() ? T("common.yes") : T("common.no"));
                UI::SpacingSeparator();
                XBase::UI::TextWrapped(T("scene.cutsceneListHint"));
                DrawNamedValueList(cutscenes, "scene.noListData", "cutscene", ApplyCutscene);
                });
                });

            XBase::UI::Tab("scene.missions", T("scene.missions"), [&] {
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::SceneMission), [&] {
                XBase::UI::TextWrapped(Controllers::Scene::GetMissionStatus());
                XBase::UI::Spacing();

                UI::PushItemWidth(120);
                UI::InputInt(T("scene.missionIndex"), &MenuState::SceneMissionIndex, 1, 1);
                UI::PopItemWidth();
                XBase::UI::SameLine();
                if (UI::Button(T("scene.startMission"), 1)) {
                    Controllers::Scene::StartMission(MenuState::SceneMissionIndex);
                }
                
                if (UI::Button(T("scene.failMission"), 1)) {
                    Controllers::Scene::FailMission();
                }

                UI::SpacingSeparator();
                DrawMissionList();
                });
                });
            });
    }
}