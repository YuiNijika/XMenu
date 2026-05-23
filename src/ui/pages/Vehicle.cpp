#include "Vehicle.h"
#include "controllers/Vehicle.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "imgui/imgui.h"
#include <cstring>

namespace {
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

            if (UI::Button(vehicle.label, 3)) {
                Controllers::Vehicle::Spawn(vehicle.model);
            }
            UI::SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Vehicle {
    void Process() {
        Controllers::Vehicle::Process();
    }

    void Draw() {
        CVehicle* currentVehicle = Controllers::Vehicle::GetCurrentVehicle();
        const bool hasVehicle = currentVehicle != nullptr;
        static CVehicle* lastVehicle = nullptr;
        static float vehicleHealth = 1000.0f;
        if (currentVehicle != lastVehicle) {
            lastVehicle = currentVehicle;
            vehicleHealth = Controllers::Vehicle::GetHealth();
        }

        if (UI::Button((const char*)u8"炸毁所有载具")) {
            Controllers::Vehicle::BlowUpAll();
        }

        ImGui::Spacing();

        if (!hasVehicle) {
            ImGui::Text((const char*)u8"当前不在载具中，修车和载具状态功能暂不可用。");
            ImGui::Spacing();
        } else {
            if (UI::Button((const char*)u8"一键修车", 6)) {
                Controllers::Vehicle::Repair();
            }
            ImGui::SameLine();
            if (UI::Button((const char*)u8"立即停车", 6)) {
                Controllers::Vehicle::Stop();
            }
            ImGui::SameLine();
            if (UI::Button((const char*)u8"扶正载具", 6)) {
                Controllers::Vehicle::Unflip();
            }
            ImGui::SameLine();
            if (UI::Button((const char*)u8"立即起步", 6)) {
                Controllers::Vehicle::Start();
            }
            ImGui::SameLine();
            if (UI::Button((const char*)u8"点火", 6)) {
                Controllers::Vehicle::SetEngine(true);
            }
            ImGui::SameLine();
            if (UI::Button((const char*)u8"熄火", 6)) {
                Controllers::Vehicle::SetEngine(false);
            }

            ImGui::Spacing();
            bool lights = Controllers::Vehicle::GetLights();
            if (ImGui::Checkbox((const char*)u8"车灯", &lights)) {
                Controllers::Vehicle::SetLights(lights);
            }
            ImGui::SameLine();
            bool locked = Controllers::Vehicle::GetLocked();
            if (ImGui::Checkbox((const char*)u8"车门上锁", &locked)) {
                Controllers::Vehicle::SetLocked(locked);
            }
            ImGui::SameLine();
            bool visible = Controllers::Vehicle::GetVisible();
            bool invisible = !visible;
            if (ImGui::Checkbox((const char*)u8"隐形载具", &invisible)) {
                Controllers::Vehicle::SetVisible(!invisible);
            }

            GameLogic::ProofState proofs = Controllers::Vehicle::GetProofState();
            if (ImGui::Checkbox((const char*)u8"防弹", &proofs.bullet)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox((const char*)u8"防撞", &proofs.collision)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox((const char*)u8"防爆", &proofs.explosion)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox((const char*)u8"防火", &proofs.fire)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox((const char*)u8"防近战", &proofs.melee)) {
                Controllers::Vehicle::SetProofState(proofs);
            }

#if GTASA
            bool skidMarks = Controllers::Vehicle::GetAlwaysSkidMarks();
            if (ImGui::Checkbox((const char*)u8"始终留下刹车痕", &skidMarks)) {
                Controllers::Vehicle::SetAlwaysSkidMarks(skidMarks);
            }
            ImGui::SameLine();
            bool disableParticles = Controllers::Vehicle::GetDisableParticles();
            if (ImGui::Checkbox((const char*)u8"禁用粒子效果", &disableParticles)) {
                Controllers::Vehicle::SetDisableParticles(disableParticles);
            }
            ImGui::SameLine();
            bool driverTargetable = Controllers::Vehicle::GetDriverTargetable();
            if (ImGui::Checkbox((const char*)u8"驾驶员可被瞄准", &driverTargetable)) {
                Controllers::Vehicle::SetDriverTargetable(driverTargetable);
            }

            bool heatSeekingTargetable = Controllers::Vehicle::GetHeatSeekingTargetable();
            if (ImGui::Checkbox((const char*)u8"可被导弹锁定", &heatSeekingTargetable)) {
                Controllers::Vehicle::SetHeatSeekingTargetable(heatSeekingTargetable);
            }
            ImGui::SameLine();
            bool petrolTankWeakPoint = Controllers::Vehicle::GetPetrolTankWeakPoint();
            if (ImGui::Checkbox((const char*)u8"油箱弱点", &petrolTankWeakPoint)) {
                Controllers::Vehicle::SetPetrolTankWeakPoint(petrolTankWeakPoint);
            }
            ImGui::SameLine();
            bool sirenOrAlarm = Controllers::Vehicle::GetSirenOrAlarm();
            if (ImGui::Checkbox((const char*)u8"警笛/警报", &sirenOrAlarm)) {
                Controllers::Vehicle::SetSirenOrAlarm(sirenOrAlarm);
            }
            ImGui::SameLine();
            bool takeLessDamage = Controllers::Vehicle::GetTakeLessDamage();
            if (ImGui::Checkbox((const char*)u8"降低受损", &takeLessDamage)) {
                Controllers::Vehicle::SetTakeLessDamage(takeLessDamage);
            }
#endif

            ImGui::PushItemWidth(160);
            ImGui::SliderFloat((const char*)u8"车身健康值", &vehicleHealth, 0.0f, 1000.0f, "%.0f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"设置健康值")) {
                Controllers::Vehicle::SetHealth(vehicleHealth);
            }
            ImGui::SameLine();
            if (ImGui::Button((const char*)u8"读取健康值")) {
                vehicleHealth = Controllers::Vehicle::GetHealth();
            }
        }

        ImGui::Spacing();

        if (UI::BeginTabBar("VehicleTabs")) {
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
                if (UI::Button((const char*)u8"按 ID 生成", 2)) {
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