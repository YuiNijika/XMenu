#include "Teleport.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include "utils/AppConfig.h"
#include "integration/XBaseBridge.h"
#include <XBase/Teleport.h>
#include <XBase/UI.h>
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

        if (key.rfind("teleport.", 0) != 0) {
            return key;
        }

        return I18n::T(key.c_str());
    }

    void WriteCurrentPosition(char* output, std::size_t outputSize) {
        XBase::Vec3 pos;
        if (!XBase::Teleport::TryGetCurrentPosition(pos)) {
            std::snprintf(output, outputSize, "--");
            return;
        }
        std::snprintf(output, outputSize, "%.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
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
                XBase::UI::Spacing();
                XBase::UI::SeparatorText(translatedCategory);
            }

            // 翻译地点名称
            const std::string translatedName = TranslateLocationName(location.name);
            
            if (UI::Button(translatedName.c_str(), 2)) {
                XBase::Teleport::To(location.x, location.y, location.z, location.interior);
            }
            UI::SameLineEvery(index++, 2);
        }
    }
}

namespace Pages::Teleport {
    void Draw() {
        static char coordInput[128] = "0, 0, 10";
        static char currentCoordText[128] = "";
        static char locationName[128] = "";

        if (XBaseBridge::HasCapability(XBase::FeatureCapability::TeleportBasic)) {
            WriteCurrentPosition(currentCoordText, sizeof(currentCoordText));
        } else {
            std::snprintf(currentCoordText, sizeof(currentCoordText), "--");
        }

        XBase::UI::Disabled(!XBaseBridge::HasCapability(XBase::FeatureCapability::TeleportBasic), [&] {
            XBase::UI::Tabs("TeleportTabs", [&] {
            XBase::UI::Tab("teleport.tab", T("tab.teleport"), [&] {
#ifdef GTASA
                XBase::UI::Columns(2, nullptr, false);
                XBase::UI::Checkbox(T("teleport.quickMapTeleport"), MenuState::QuickTeleport);
                XBase::UI::NextColumn();
                XBase::UI::Checkbox(T("teleport.allowUnderwaterLanding"), MenuState::SpawnUnderwater);
                XBase::UI::Checkbox(T("teleport.quickMarkerTeleport"), MenuState::TeleportMarker);
                XBase::UI::Columns(1);
#endif

                XBase::UI::Spacing();
                XBase::UI::InputText(T("teleport.currentCoordinates"), currentCoordText, sizeof(currentCoordText), "x, y, z", true);
                XBase::UI::PushItemWidth(-142.0f);
                XBase::UI::InputText(T("teleport.coordinates"), coordInput, sizeof(coordInput), "x, y, z");
                XBase::UI::PopItemWidth();
                XBase::UI::SameLine();
                if (XBase::UI::Button(T("teleport.readCurrentValues"), {132.0f, 0.0f})) {
                    std::snprintf(coordInput, sizeof(coordInput), "%s", currentCoordText);
                }
                XBase::UI::Spacing();

                if (UI::Button(T("teleport.toCoordinates"), 4)) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(coordInput, "%f,%f,%f", &x, &y, &z) == 3) {
                        XBase::Teleport::To(x, y, z + 1.0f);
                    }
                }
                XBase::UI::SameLine();
#ifdef GTASA
                if (UI::Button(T("teleport.byMapPosition"), 4)) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(coordInput, "%f,%f,%f", &x, &y, &z) == 3) {
                        XBase::Teleport::MapPosition(x, y, MenuState::SpawnUnderwater);
                    }
                }
                XBase::UI::SameLine();
                if (UI::Button(T("teleport.toMarker"), 4)) {
                    XBase::Teleport::Marker(MenuState::SpawnUnderwater);
                }
#else
                if (UI::Button(T("teleport.mapCenter"), 4)) {
                    XBase::Teleport::Center();
                }
#endif
                XBase::UI::SameLine();
                if (UI::Button(T("teleport.moveForward"), 4)) {
                    XBase::Teleport::Forward(MenuState::TeleportForwardDistance);
                }

                XBase::UI::Spacing();
                bool teleportSettingsChanged = false;
                XBase::UI::PushItemWidth(180.0f);
                teleportSettingsChanged |= XBase::UI::Input(T("teleport.forwardDistance"), MenuState::TeleportForwardDistance, 1.0f, 5.0f, "%.1f");
                XBase::UI::PopItemWidth();
                if (MenuState::TeleportForwardDistance < 0.1f) {
                    MenuState::TeleportForwardDistance = 0.1f;
                    teleportSettingsChanged = true;
                }
                teleportSettingsChanged |= XBase::UI::Checkbox(T("teleport.forwardHold"), MenuState::TeleportForwardHold);
                XBase::UI::TextDisabled(T("teleport.forwardHoldHint"));
                if (teleportSettingsChanged) {
                    AppConfig::Save();
                }

#ifdef GTASA
                if (MenuState::QuickTeleport && XBase::UI::CollapsingHeader(T("teleport.customMapSize"))) {
                    static float mapWidthDraft = MenuState::TeleportMapWidth;
                    static float mapHeightDraft = MenuState::TeleportMapHeight;
                    XBase::UI::TextWrapped(T("teleport.customMapSizeHint"));
                    XBase::UI::Input(T("teleport.width"), mapWidthDraft, 1.0f, 100.0f, "%.1f");
                    XBase::UI::Input(T("teleport.height"), mapHeightDraft, 1.0f, 100.0f, "%.1f");
                    if (UI::Button(T("teleport.applyMapSize"), 2)) {
                        MenuState::TeleportMapWidth = mapWidthDraft;
                        MenuState::TeleportMapHeight = mapHeightDraft;
                    }
                    XBase::UI::SameLine();
                    if (UI::Button(T("teleport.restoreDefault"), 2)) {
                        mapWidthDraft = 6000.0f;
                        mapHeightDraft = 6000.0f;
                        MenuState::TeleportMapWidth = mapWidthDraft;
                        MenuState::TeleportMapHeight = mapHeightDraft;
                    }
                }
#endif
                });

            XBase::UI::Tab("teleport.locations", T("teleport.locations"), [&] {
                XBase::UI::InputText(T("teleport.locationName"), locationName, sizeof(locationName), T("teleport.customLocation"));
                XBase::UI::InputText(T("teleport.locationCoordinates"), currentCoordText, sizeof(currentCoordText), "x, y, z", true);
                if (UI::Button(T("teleport.addLocation"))) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(currentCoordText, "%f,%f,%f", &x, &y, &z) == 3 && Resources::AddCustomLocation(locationName, x, y, z, 0)) {
                        locationName[0] = '\0';
                    }
                }

                UI::SpacingSeparator();
                DrawLocationList();
                });

            });
        });
    }
}