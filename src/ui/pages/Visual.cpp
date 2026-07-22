#include "Visual.h"
#include "controllers/Visual.h"
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
    struct VisualEntry {
        std::string category;
        std::string name;
        int id = 0;
    };

    struct VisualList {
        std::vector<VisualEntry> entries;
        std::filesystem::file_time_type lastWrite{};
        bool loaded = false;
    };

    const char* T(const char* key) {
        return I18n::T(key);
    }

    void LoadVisuals(VisualList& list) {
        const std::string path = DataManager::GetDataFilePath("visuals.json");
        std::error_code ec;
        const auto currentWrite = std::filesystem::exists(path, ec) ? std::filesystem::last_write_time(path, ec) : std::filesystem::file_time_type{};
        if (list.loaded && !ec && currentWrite == list.lastWrite) {
            return;
        }

        list.loaded = true;
        list.lastWrite = currentWrite;
        list.entries.clear();
        const JsonLoader::JsonValue data = JsonLoader::LoadFromFile(path);
        if (data.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        for (const auto& category : JsonLoader::GetArray(data, "visuals")) {
            if (category.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }
            const std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
            for (const auto& entry : JsonLoader::GetArray(category, "entries")) {
                if (entry.type != JsonLoader::JsonValue::OBJECT) {
                    continue;
                }
                VisualEntry item;
                item.category = categoryName;
                item.name = JsonLoader::GetString(entry, "name", "unknown");
                item.id = static_cast<int>(JsonLoader::GetNumber(entry, "id", 0));
                list.entries.push_back(item);
            }
        }
    }

    void DrawVisualList() {
        static VisualList visuals;
        LoadVisuals(visuals);
        if (visuals.entries.empty()) {
            ImGui::TextDisabled("%s", T("visual.noListData"));
            return;
        }

        std::string currentCategory;
        int index = 0;
        for (const auto& entry : visuals.entries) {
            if (currentCategory != entry.category) {
                currentCategory = entry.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(T(currentCategory.c_str()));
            }

            const char* translatedName = T(entry.name.c_str());
            char label[128];
            std::snprintf(label, sizeof(label), "%s (%d)##visual_%s_%d", translatedName, entry.id, entry.category.c_str(), entry.id);
            if (UI::Button(label, 3)) {
                MenuState::VisualFilter = true;
                MenuState::VisualFilterId = entry.id;
                Controllers::Visual::ApplyFilterState();
            }
            UI::SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Visual {
    void Process() {
        Controllers::Visual::Process();
    }

    void Draw() {
        if (ImGui::Checkbox(T("visual.hud"), &MenuState::VisualHud)) {
            Controllers::Visual::ApplyHudState();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox(T("visual.radar"), &MenuState::VisualRadar)) {
            Controllers::Visual::ApplyRadarState();
        }

#ifdef GTASA
        UI::SpacingSeparator();
        ImGui::Columns(2, nullptr, false);
        ImGui::Checkbox(T("visual.squareRadar"), &MenuState::VisualSquareRadar);
        ImGui::TextDisabled("%s", T("visual.squareRadarHint"));
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.noRadarRot"), &MenuState::VisualNoRadarRot);
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.fullscreenMap"), &MenuState::VisualFullscreenMap);
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.unfogMap"), &MenuState::VisualUnfogMap);
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.hideAreaNames"), &MenuState::VisualHideAreaNames);
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.hideVehicleNames"), &MenuState::VisualHideVehicleNames);
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.nightVision"), &MenuState::VisualNightVision);
        ImGui::NextColumn();
        ImGui::Checkbox(T("visual.infrared"), &MenuState::VisualInfrared);
        ImGui::Columns(1);
#endif

        UI::SpacingSeparator();
        ImGui::Checkbox(T("visual.filter"), &MenuState::VisualFilter);
        ImGui::PushItemWidth(160.0f);
        ImGui::InputInt(T("visual.filterId"), &MenuState::VisualFilterId);
        ImGui::BeginDisabled(true);
        ImGui::SliderFloat(T("visual.timecycStrength"), &MenuState::VisualTimecycStrength, 0.0f, 2.0f, "%.2f");
        ImGui::EndDisabled();
        ImGui::PopItemWidth();
        ImGui::TextDisabled("%s", T("visual.timecycStrengthHint"));
        if (UI::Button(T("visual.applyFilter"))) {
            Controllers::Visual::ApplyFilterState();
        }
        ImGui::TextWrapped("%s", T("visual.filterHint"));

        UI::SpacingSeparator();
        ImGui::TextWrapped("%s", T("visual.listHint"));
        DrawVisualList();
    }
}