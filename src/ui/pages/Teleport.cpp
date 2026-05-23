#include "Teleport.h"
#include "controllers/Teleport.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "imgui/imgui.h"
#include <cstdio>
#include <cstring>

namespace {
    void DrawLocationList() {
        const Resources::LocationTable table = Resources::GetLocations();
        const char* currentCategory = nullptr;
        int index = 0;

        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::LocationEntry& location = table.entries[i];
            if (!currentCategory || std::strcmp(currentCategory, location.category) != 0) {
                currentCategory = location.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(currentCategory);
            }

            if (ImGui::Button(location.label)) {
                Controllers::Teleport::To(location.x, location.y, location.z, location.interior);
            }
            if ((index + 1) % 2 != 0) {
                ImGui::SameLine();
            }
            ++index;
        }
    }
}

namespace Pages::Teleport {
    void Draw() {
        static char coordInput[128] = "0, 0, 10";
        static char locationName[128] = "";

        if (ImGui::BeginTabBar("TeleportTabs", ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem((const char*)u8"传送")) {
#ifdef GTASA
                ImGui::Columns(2, nullptr, false);
                ImGui::Checkbox((const char*)u8"插入当前坐标", &MenuState::TeleportInsertCoord);
                ImGui::Checkbox((const char*)u8"快速地图传送", &MenuState::QuickTeleport);
                ImGui::NextColumn();
                ImGui::Checkbox((const char*)u8"允许生成在水下", &MenuState::SpawnUnderwater);
                ImGui::Checkbox((const char*)u8"快捷键传送到标记点", &MenuState::TeleportMarker);
                ImGui::Columns(1);
#else
                ImGui::Checkbox((const char*)u8"插入当前坐标", &MenuState::TeleportInsertCoord);
#endif

                if (MenuState::TeleportInsertCoord) {
                    const CVector pos = Controllers::Teleport::GetCurrentPosition();
                    std::snprintf(coordInput, sizeof(coordInput), "%.0f, %.0f, %.0f", pos.x, pos.y, pos.z);
                }

                ImGui::Spacing();
                ImGui::InputTextWithHint((const char*)u8"坐标", "x, y, z", coordInput, sizeof(coordInput));
                ImGui::Spacing();

                if (ImGui::Button((const char*)u8"传送到坐标")) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(coordInput, "%f,%f,%f", &x, &y, &z) == 3) {
                        Controllers::Teleport::To(x, y, z + 1.0f);
                    }
                }
                ImGui::SameLine();
#ifdef GTASA
                if (ImGui::Button((const char*)u8"按地图位置传送")) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 10.0f;
                    if (std::sscanf(coordInput, "%f,%f,%f", &x, &y, &z) == 3) {
                        Controllers::Teleport::MapPosition(x, y, MenuState::SpawnUnderwater);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button((const char*)u8"传送到标记点")) {
                    Controllers::Teleport::Marker(MenuState::SpawnUnderwater);
                }
#else
                if (ImGui::Button((const char*)u8"地图中心")) {
                    Controllers::Teleport::Center();
                }
#endif
                ImGui::SameLine();
                if (ImGui::Button((const char*)u8"向前挪 5 米")) {
                    Controllers::Teleport::Forward(5.0f);
                }

#ifdef GTASA
                if (MenuState::QuickTeleport && ImGui::CollapsingHeader((const char*)u8"自定义地图尺寸")) {
                    ImGui::TextWrapped((const char*)u8"如果快速地图传送位置偏移，可以在这里调整地图宽高。默认值是 6000 x 6000。");
                    ImGui::InputFloat((const char*)u8"宽度", &MenuState::TeleportMapWidth, 1.0f, 100.0f, "%.1f");
                    ImGui::InputFloat((const char*)u8"高度", &MenuState::TeleportMapHeight, 1.0f, 100.0f, "%.1f");
                    if (ImGui::Button((const char*)u8"恢复默认")) {
                        MenuState::TeleportMapWidth = 6000.0f;
                        MenuState::TeleportMapHeight = 6000.0f;
                    }
                }
#endif
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"地点")) {
                ImGui::InputTextWithHint((const char*)u8"地点名称", (const char*)u8"自定义地点", locationName, sizeof(locationName));
                ImGui::InputTextWithHint((const char*)u8"地点坐标", "x, y, z", coordInput, sizeof(coordInput));
                if (ImGui::Button((const char*)u8"添加地点")) {
                    locationName[0] = '\0';
                }

                ImGui::Separator();
                DrawLocationList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}