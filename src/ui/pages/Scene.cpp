#include "Scene.h"
#include "controllers/Scene.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/DataManager.h"
#include "utils/I18n.h"
#include "utils/JsonLoader.h"
#include "imgui/imgui.h"
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
                list.entries.push_back(item);
            }
        }
    }

    void DrawAnimationList() {
        static LiveList<AnimationEntry> animations;
        LoadAnimations(animations);
        if (animations.entries.empty()) {
            ImGui::TextDisabled("%s", T("scene.noListData"));
            return;
        }

        std::string currentCategory;
        int index = 0;
        for (const auto& animation : animations.entries) {
            if (currentCategory != animation.category) {
                currentCategory = animation.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(T(currentCategory.c_str()));
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
            ImGui::TextDisabled("%s", T(emptyKey));
            return;
        }

        std::string currentCategory;
        int index = 0;
        for (const auto& entry : list.entries) {
            if (currentCategory != entry.category) {
                currentCategory = entry.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(T(currentCategory.c_str()));
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
        std::snprintf(MenuState::SceneCutsceneName, sizeof(MenuState::SceneCutsceneName), "%s", entry.name.c_str());
        // Cheat-Menu uses 'name' for cutscene id and 'interior' (which maps to 'value' here via LoadNamedValues) for interior ID
        std::snprintf(MenuState::SceneCutsceneInterior, sizeof(MenuState::SceneCutsceneInterior), "%s", entry.value.c_str());
        Controllers::Scene::StartCutscene();
    }
}

namespace Pages::Scene {
    void Process() {
        Controllers::Scene::Process();
    }

    void Draw() {
        if (UI::BeginTabBar("SceneTabs")) {
            if (ImGui::BeginTabItem(T("scene.animation"))) {
                ImGui::InputText(T("scene.animGroup"), MenuState::SceneAnimGroup, sizeof(MenuState::SceneAnimGroup));
                ImGui::InputText(T("scene.animName"), MenuState::SceneAnimName, sizeof(MenuState::SceneAnimName));
                ImGui::Checkbox(T("scene.loop"), &MenuState::SceneAnimLoop);
                if (UI::Button(T("scene.playAnim"), 2)) {
                    Controllers::Scene::PlayPlayerAnimation();
                }
                ImGui::SameLine();
                if (UI::Button(T("scene.stopAnim"), 2)) {
                    Controllers::Scene::StopPlayerAnimation();
                }
                UI::SpacingSeparator();
                ImGui::TextWrapped("%s", T("scene.animationListHint"));
                DrawAnimationList();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("scene.particle"))) {
                static LiveList<NamedValueEntry> particles;
                LoadNamedValues(particles, "particles.json", "particles", "effect");
                ImGui::InputText(T("scene.particleName"), MenuState::SceneParticleName, sizeof(MenuState::SceneParticleName));
                if (UI::Button(T("scene.spawnParticle"))) {
                    Controllers::Scene::SpawnParticleAtPlayer();
                }
                ImGui::SameLine();
                if (UI::Button("Remove Last")) {
                    Controllers::Scene::RemoveLatestParticle();
                }
                ImGui::SameLine();
                if (UI::Button("Remove All")) {
                    Controllers::Scene::RemoveAllParticles();
                }
                UI::SpacingSeparator();
                ImGui::TextWrapped("%s", T("scene.particleListHint"));
                DrawNamedValueList(particles, "scene.noListData", "particle", ApplyParticle);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("scene.cutscene"))) {
                static LiveList<NamedValueEntry> cutscenes;
                LoadNamedValues(cutscenes, "cutscenes.json", "cutscenes", "interior");
                ImGui::InputText(T("scene.cutsceneName"), MenuState::SceneCutsceneName, sizeof(MenuState::SceneCutsceneName));
                ImGui::InputText("Interior ID", MenuState::SceneCutsceneInterior, sizeof(MenuState::SceneCutsceneInterior));
                if (UI::Button(T("scene.startCutscene"), 2)) {
                    Controllers::Scene::StartCutscene();
                }
                ImGui::SameLine();
                if (UI::Button(T("scene.stopCutscene"), 2)) {
                    Controllers::Scene::StopCutscene();
                }
                ImGui::Text("%s: %s", T("scene.cutsceneRunning"), Controllers::Scene::IsCutsceneRunning() ? T("common.yes") : T("common.no"));
                UI::SpacingSeparator();
                ImGui::TextWrapped("%s", T("scene.cutsceneListHint"));
                DrawNamedValueList(cutscenes, "scene.noListData", "cutscene", ApplyCutscene);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("scene.mission"))) {
                ImGui::TextWrapped("%s", Controllers::Scene::GetMissionStatus());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
}