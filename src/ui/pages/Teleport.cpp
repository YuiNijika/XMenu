#include "Teleport.h"
#include "controllers/Teleport.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include <cstdio>
#include <cstring>
#include <string>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    std::string TranslateLocationName(const std::string& key) {
        const struct NumberedLocationPattern {
            const char* prefix;
            const char* labelKey;
        } patterns[] = {
            {"teleport.location.sa_stunt_jump_", "teleport.locationPrefix.sa_stunt_jump"},
            {"teleport.location.sa_snapshot_", "teleport.locationPrefix.sa_snapshot"},
            {"teleport.location.sa_oyster_", "teleport.locationPrefix.sa_oyster"}
        };

        for (const NumberedLocationPattern& pattern : patterns) {
            const std::size_t prefixLength = std::strlen(pattern.prefix);
            if (key.rfind(pattern.prefix, 0) == 0 && key.size() > prefixLength) {
                return std::string(I18n::T(pattern.labelKey)) + " " + key.substr(prefixLength);
            }
        }

        return I18n::T(key.c_str());
    }

    void DrawLocationList() {
        const Resources::LocationTable table = Resources::GetLocations();
        const char* currentCategory = nullptr;
        std::string currentCategoryKey;
        int index = 0;

        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::LocationEntry& location = table.entries->at(i);
            
            // 翻译分类名称
            std::string categoryKey = "teleport.category." + location.category;
            const char* translatedCategory = I18n::T(categoryKey.c_str());
            
            if (!currentCategory || currentCategoryKey != location.category) {
                currentCategory = translatedCategory;
                currentCategoryKey = location.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(translatedCategory);
            }

            // 翻译地点名称
            const std::string translatedName = TranslateLocationName(location.name);
            
            if (UI::Button(translatedName.c_str(), 2)) {
                Controllers::Teleport::To(location.x, location.y, location.z, location.interior);
            }
            UI::SameLineEvery(index++, 2);
        }
    }
}

namespace Pages::Teleport {
    void Draw() {
        static char coordInput[128] = "0, 0, 10";
        static char locationName[128] = "";

        if (UI::BeginTabBar("TeleportTabs")) {
            if (ImGui::BeginTabItem(T("tab.teleport"))) {
#ifdef GTASA
                ImGui::Columns(2, nullptr, false);
                ImGui::Checkbox(T("teleport.insertCurrentCoordinates"), &MenuState::TeleportInsertCoord);
                ImGui::Checkbox(T("teleport.quickMapTeleport"), &MenuState::QuickTeleport);
                ImGui::NextColumn();
                ImGui::Checkbox(T("teleport.allowUnderwaterLanding"), &MenuState::SpawnUnderwater);
                ImGui::Checkbox(T("teleport.quickMarkerTeleport"), &MenuState::TeleportMarker);
                ImGui::Columns(1);
#else
                ImGui::Checkbox(T("teleport.insertCurrentCoordinates"), &MenuState::TeleportInsertCoord);
#endif

                if (MenuState::TeleportInsertCoord) {
                    const CVector pos = Controllers::Teleport::GetCurrentPosition();
                    std::snprintf(coordInput, sizeof(coordInput), "%.0f, %.0f, %.0f", pos.x, pos.y, pos.z);
                }

                ImGui::Spacing();
                ImGui::InputTextWithHint(T("teleport.coordinates"), "x, y, z", coordInput, sizeof(coordInput));
                ImGui::Spacing();

                if (UI::Button(T("teleport.toCoordinates"), 4)) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(coordInput, "%f,%f,%f", &x, &y, &z) == 3) {
                        Controllers::Teleport::To(x, y, z + 1.0f);
                    }
                }
                ImGui::SameLine();
#ifdef GTASA
                if (UI::Button(T("teleport.byMapPosition"), 4)) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(coordInput, "%f,%f,%f", &x, &y, &z) == 3) {
                        Controllers::Teleport::MapPosition(x, y, MenuState::SpawnUnderwater);
                    }
                }
                ImGui::SameLine();
                if (UI::Button(T("teleport.toMarker"), 4)) {
                    Controllers::Teleport::Marker(MenuState::SpawnUnderwater);
                }
#else
                if (UI::Button(T("teleport.mapCenter"), 4)) {
                    Controllers::Teleport::Center();
                }
#endif
                ImGui::SameLine();
                if (UI::Button(T("teleport.moveForward5m"), 4)) {
                    Controllers::Teleport::Forward(5.0f);
                }

#ifdef GTASA
                if (MenuState::QuickTeleport && ImGui::CollapsingHeader(T("teleport.customMapSize"))) {
                    static float mapWidthDraft = MenuState::TeleportMapWidth;
                    static float mapHeightDraft = MenuState::TeleportMapHeight;
                    ImGui::TextWrapped("%s", T("teleport.customMapSizeHint"));
                    ImGui::InputFloat(T("teleport.width"), &mapWidthDraft, 1.0f, 100.0f, "%.1f");
                    ImGui::InputFloat(T("teleport.height"), &mapHeightDraft, 1.0f, 100.0f, "%.1f");
                    if (UI::Button(T("teleport.applyMapSize"), 2)) {
                        MenuState::TeleportMapWidth = mapWidthDraft;
                        MenuState::TeleportMapHeight = mapHeightDraft;
                    }
                    ImGui::SameLine();
                    if (UI::Button(T("teleport.restoreDefault"), 2)) {
                        mapWidthDraft = 6000.0f;
                        mapHeightDraft = 6000.0f;
                        MenuState::TeleportMapWidth = mapWidthDraft;
                        MenuState::TeleportMapHeight = mapHeightDraft;
                    }
                }
#endif
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("teleport.locations"))) {
                ImGui::InputTextWithHint(T("teleport.locationName"), T("teleport.customLocation"), locationName, sizeof(locationName));
                ImGui::InputTextWithHint(T("teleport.locationCoordinates"), "x, y, z", coordInput, sizeof(coordInput));
                if (UI::Button(T("teleport.addLocation"))) {
                    locationName[0] = '\0';
                }

                UI::SpacingSeparator();
                DrawLocationList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}