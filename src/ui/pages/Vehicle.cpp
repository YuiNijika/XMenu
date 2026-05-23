#include "Vehicle.h"
#include "controllers/Vehicle.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "imgui/imgui.h"
#include <cstring>

namespace {
    void SameLineEvery(int index, int columns) {
        if ((index + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }

    void DrawVehicleList() {
        const Resources::VehicleTable table = Resources::GetVehicles();
        if (table.count == 0) {
            ImGui::TextWrapped((const char*)u8"当前版本没有 Cheat-Menu 的载具列表数据，请用模型 ID 生成。");
            return;
        }

        const char* currentCategory = nullptr;
        int index = 0;
        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::VehicleEntry& vehicle = table.entries[i];
            if (!currentCategory || std::strcmp(currentCategory, vehicle.category) != 0) {
                currentCategory = vehicle.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(currentCategory);
            }

            if (ImGui::Button(vehicle.label)) {
                Controllers::Vehicle::Spawn(vehicle.model);
            }
            SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Vehicle {
    void Process() {
        Controllers::Vehicle::Process();
    }

    void Draw() {
        const bool hasVehicle = Controllers::Vehicle::GetCurrentVehicle() != nullptr;

        if (!hasVehicle) {
            ImGui::Text((const char*)u8"当前不在载具中，修车和载具状态功能暂不可用。");
            ImGui::Spacing();
        } else {
            if (ImGui::Button((const char*)u8"一键修车")) {
                Controllers::Vehicle::Repair();
            }
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"立即停车")) {
                Controllers::Vehicle::Stop();
            }
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"扶正载具")) {
                Controllers::Vehicle::Unflip();
            }
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"立即起步")) {
                Controllers::Vehicle::Start();
            }
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"点火")) {
                Controllers::Vehicle::SetEngine(true);
            }
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"熄火")) {
                Controllers::Vehicle::SetEngine(false);
            }
        }

        ImGui::Spacing();

        if (ImGui::BeginTabBar("VehicleTabs", ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem((const char*)u8"功能开关")) {
                ImGui::Checkbox((const char*)u8"载具无伤", &MenuState::VehicleNoDamage);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"自动扶正", &MenuState::VehicleAutoUnflip);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"车身加重", &MenuState::VehicleHeavy);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"载具防水", &MenuState::VehicleWatertight);

                ImGui::Checkbox((const char*)u8"锁定车速", &MenuState::VehicleSpeedLock);
                ImGui::PushItemWidth(150);
                ImGui::SliderFloat((const char*)u8"目标速度", &MenuState::VehicleSpeed, 5.0f, 300.0f, "%.0f");
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"生成载具")) {
                ImGui::Checkbox((const char*)u8"生成后坐上驾驶位", &MenuState::VehicleSpawnAsDriver);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"飞机/直升机生成在空中", &MenuState::VehicleSpawnAircraftInAir);

                ImGui::PushItemWidth(160);
                ImGui::InputInt((const char*)u8"模型 ID", &MenuState::VehicleSpawnModel);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button((const char*)u8"按 ID 生成")) {
                    if (MenuState::VehicleSpawnModel >= 0) {
                        Controllers::Vehicle::Spawn(static_cast<unsigned int>(MenuState::VehicleSpawnModel));
                    }
                }

                ImGui::Spacing();
                DrawVehicleList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}