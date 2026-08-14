#include "Visual.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/DataManager.h"
#include "utils/I18n.h"
#include "utils/JsonLoader.h"
#include "integration/XBaseBridge.h"
#include <XBase/UI.h>
#include <XBase/Visual.h>
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
            XBase::UI::TextDisabled(T("visual.noListData"));
            return;
        }

        std::string currentCategory;
        int index = 0;
        for (const auto& entry : visuals.entries) {
            if (currentCategory != entry.category) {
                currentCategory = entry.category;
                index = 0;
                XBase::UI::Spacing();
                XBase::UI::SeparatorText(T(currentCategory.c_str()));
            }

            const char* translatedName = T(entry.name.c_str());
            char label[128];
            std::snprintf(label, sizeof(label), "%s (%d)##visual_%s_%d", translatedName, entry.id, entry.category.c_str(), entry.id);
            if (UI::Button(label, 3)) {
                MenuState::VisualFilter = true;
                MenuState::VisualFilterId = entry.id;
                XBase::Visual::SetFilter(MenuState::VisualFilterId, MenuState::VisualTimecycStrength);
            }
            UI::SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Visual {
    void Process() {
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VisualHudRadar)) {
            MenuState::VisualHud = true;
            MenuState::VisualRadar = true;
        }
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VisualFilter)) {
            MenuState::VisualFilter = false;
        } else {
            XBase::Visual::SetFilter(
                MenuState::VisualFilter ? MenuState::VisualFilterId : 0,
                MenuState::VisualTimecycStrength);
        }
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::VisualRadarOptions)) {
            MenuState::VisualSquareRadar = false;
            MenuState::VisualNoRadarRot = false;
            MenuState::VisualFullscreenMap = false;
            MenuState::VisualUnfogMap = false;
            MenuState::VisualHideAreaNames = false;
            MenuState::VisualHideVehicleNames = false;
            MenuState::VisualNightVision = false;
            MenuState::VisualInfrared = false;
        } else {
            XBase::Visual::RadarOptions radarOptions;
            radarOptions.square = MenuState::VisualSquareRadar;
            radarOptions.noRadarRot = MenuState::VisualNoRadarRot;
            radarOptions.fullscreenMap = MenuState::VisualFullscreenMap;
            radarOptions.unfogMap = MenuState::VisualUnfogMap;
            radarOptions.hideAreaNames = MenuState::VisualHideAreaNames;
            radarOptions.hideVehicleNames = MenuState::VisualHideVehicleNames;
            radarOptions.nightVision = MenuState::VisualNightVision;
            radarOptions.infrared = MenuState::VisualInfrared;
            XBase::Visual::SetRadarOptions(radarOptions);
        }
    }

    void Draw() {
        const bool hasHudRadar = XBaseBridge::HasCapability(XBase::FeatureCapability::VisualHudRadar);
        const bool hasFilter = XBaseBridge::HasCapability(XBase::FeatureCapability::VisualFilter);
        const bool hasRadarOptions = XBaseBridge::HasCapability(XBase::FeatureCapability::VisualRadarOptions);

        XBase::UI::Disabled(!hasHudRadar, [&] {
            if (XBase::UI::Checkbox(T("visual.hud"), MenuState::VisualHud)) {
                XBase::Visual::DisplayHud(MenuState::VisualHud);
            }
            XBase::UI::SameLine();
            if (XBase::UI::Checkbox(T("visual.radar"), MenuState::VisualRadar)) {
                XBase::Visual::DisplayRadar(MenuState::VisualRadar);
            }
        });

#ifdef GTASA
        UI::SpacingSeparator();
        XBase::UI::Disabled(!hasRadarOptions, [&] {
            XBase::UI::Columns(2, nullptr, false);
            XBase::UI::Checkbox(T("visual.squareRadar"), MenuState::VisualSquareRadar);
            XBase::UI::TextDisabled(T("visual.squareRadarHint"));
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.noRadarRot"), MenuState::VisualNoRadarRot);
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.fullscreenMap"), MenuState::VisualFullscreenMap);
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.unfogMap"), MenuState::VisualUnfogMap);
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.hideAreaNames"), MenuState::VisualHideAreaNames);
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.hideVehicleNames"), MenuState::VisualHideVehicleNames);
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.nightVision"), MenuState::VisualNightVision);
            XBase::UI::NextColumn();
            XBase::UI::Checkbox(T("visual.infrared"), MenuState::VisualInfrared);
            XBase::UI::Columns(1);
        });
#endif

        UI::SpacingSeparator();
        XBase::UI::Disabled(!hasFilter, [&] {
            XBase::UI::Checkbox(T("visual.filter"), MenuState::VisualFilter);
            XBase::UI::PushItemWidth(160.0f);
            XBase::UI::Input(T("visual.filterId"), MenuState::VisualFilterId);
            XBase::UI::Disabled(true, [&] {
                XBase::UI::Slider(T("visual.timecycStrength"), MenuState::VisualTimecycStrength, 0.0f, 2.0f, "%.2f");
            });
            XBase::UI::PopItemWidth();
            XBase::UI::TextDisabled(T("visual.timecycStrengthHint"));
            if (UI::Button(T("visual.applyFilter"))) {
                XBase::Visual::SetFilter(MenuState::VisualFilterId, MenuState::VisualTimecycStrength);
            }
            XBase::UI::TextWrapped(T("visual.filterHint"));

            UI::SpacingSeparator();
            XBase::UI::TextWrapped(T("visual.listHint"));
            DrawVisualList();
        });
    }
}