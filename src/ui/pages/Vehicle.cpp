#include "Vehicle.h"
#include "controllers/Vehicle.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include <cstring>
#include <cstdio>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    void DrawVehicleList() {
        const Resources::VehicleTable table = Resources::GetVehicles();
        if (table.count == 0) {
            ImGui::TextWrapped("%s", T("vehicle.noListData"));
            return;
        }

        std::string currentCategoryKey;
        int index = 0;
        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::VehicleEntry& vehicle = table.entries->at(i);

            const std::string categoryKey = "vehicle.category." + vehicle.category;
            const char* translatedCategory = I18n::T(categoryKey.c_str());

            if (currentCategoryKey != vehicle.category) {
                currentCategoryKey = vehicle.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(translatedCategory);
            }

            const char* translatedName = I18n::T(vehicle.name.c_str());
            char buttonLabel[96];
            std::snprintf(buttonLabel, sizeof(buttonLabel), "%s (%d)", translatedName, vehicle.id);
            if (UI::Button(buttonLabel, 3)) {
                Controllers::Vehicle::Spawn(static_cast<unsigned int>(vehicle.id));
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

        if (UI::Button(T("vehicle.blowUpAll"))) {
            Controllers::Vehicle::BlowUpAll();
        }

        ImGui::Spacing();

        if (!hasVehicle) {
            ImGui::Text("%s", T("vehicle.notInVehicle"));
            ImGui::Spacing();
        } else {
            if (UI::Button(T("vehicle.repair"), 6)) {
                Controllers::Vehicle::Repair();
            }
            ImGui::SameLine();
            if (UI::Button(T("vehicle.stop"), 6)) {
                Controllers::Vehicle::Stop();
            }
            ImGui::SameLine();
            if (UI::Button(T("vehicle.unflip"), 6)) {
                Controllers::Vehicle::Unflip();
            }
            ImGui::SameLine();
            if (UI::Button(T("vehicle.start"), 6)) {
                Controllers::Vehicle::Start();
            }
            ImGui::SameLine();
            if (UI::Button(T("vehicle.engineOn"), 6)) {
                Controllers::Vehicle::SetEngine(true);
            }
            ImGui::SameLine();
            if (UI::Button(T("vehicle.engineOff"), 6)) {
                Controllers::Vehicle::SetEngine(false);
            }

            ImGui::Spacing();
            bool lights = Controllers::Vehicle::GetLights();
            if (ImGui::Checkbox(T("vehicle.lights"), &lights)) {
                Controllers::Vehicle::SetLights(lights);
            }
            ImGui::SameLine();
            bool locked = Controllers::Vehicle::GetLocked();
            if (ImGui::Checkbox(T("vehicle.lockDoors"), &locked)) {
                Controllers::Vehicle::SetLocked(locked);
            }
            ImGui::SameLine();
            bool visible = Controllers::Vehicle::GetVisible();
            bool invisible = !visible;
            if (ImGui::Checkbox(T("vehicle.invisible"), &invisible)) {
                Controllers::Vehicle::SetVisible(!invisible);
            }

            GameLogic::ProofState proofs = Controllers::Vehicle::GetProofState();
            if (ImGui::Checkbox(T("proof.bullet"), &proofs.bullet)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(T("proof.collision"), &proofs.collision)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(T("proof.explosion"), &proofs.explosion)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(T("proof.fire"), &proofs.fire)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(T("proof.melee"), &proofs.melee)) {
                Controllers::Vehicle::SetProofState(proofs);
            }

#if GTASA
            bool skidMarks = Controllers::Vehicle::GetAlwaysSkidMarks();
            if (ImGui::Checkbox(T("vehicle.alwaysSkidMarks"), &skidMarks)) {
                Controllers::Vehicle::SetAlwaysSkidMarks(skidMarks);
            }
            ImGui::SameLine();
            bool disableParticles = Controllers::Vehicle::GetDisableParticles();
            if (ImGui::Checkbox(T("vehicle.disableParticles"), &disableParticles)) {
                Controllers::Vehicle::SetDisableParticles(disableParticles);
            }
            ImGui::SameLine();
            bool driverTargetable = Controllers::Vehicle::GetDriverTargetable();
            if (ImGui::Checkbox(T("vehicle.driverTargetable"), &driverTargetable)) {
                Controllers::Vehicle::SetDriverTargetable(driverTargetable);
            }

            bool heatSeekingTargetable = Controllers::Vehicle::GetHeatSeekingTargetable();
            if (ImGui::Checkbox(T("vehicle.missileTargetable"), &heatSeekingTargetable)) {
                Controllers::Vehicle::SetHeatSeekingTargetable(heatSeekingTargetable);
            }
            ImGui::SameLine();
            bool petrolTankWeakPoint = Controllers::Vehicle::GetPetrolTankWeakPoint();
            if (ImGui::Checkbox(T("vehicle.petrolTankWeakness"), &petrolTankWeakPoint)) {
                Controllers::Vehicle::SetPetrolTankWeakPoint(petrolTankWeakPoint);
            }
            ImGui::SameLine();
            bool sirenOrAlarm = Controllers::Vehicle::GetSirenOrAlarm();
            if (ImGui::Checkbox(T("vehicle.sirenAlarm"), &sirenOrAlarm)) {
                Controllers::Vehicle::SetSirenOrAlarm(sirenOrAlarm);
            }
            ImGui::SameLine();
            bool takeLessDamage = Controllers::Vehicle::GetTakeLessDamage();
            if (ImGui::Checkbox(T("vehicle.takeLessDamage"), &takeLessDamage)) {
                Controllers::Vehicle::SetTakeLessDamage(takeLessDamage);
            }
#endif

            ImGui::PushItemWidth(160);
            ImGui::SliderFloat(T("vehicle.health"), &vehicleHealth, 0.0f, 1000.0f, "%.0f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(T("vehicle.setHealth"))) {
                Controllers::Vehicle::SetHealth(vehicleHealth);
            }
            ImGui::SameLine();
            if (ImGui::Button(T("vehicle.readHealth"))) {
                vehicleHealth = Controllers::Vehicle::GetHealth();
            }
        }

        ImGui::Spacing();

        if (UI::BeginTabBar("VehicleTabs")) {
            if (ImGui::BeginTabItem(T("common.toggles"))) {
                ImGui::Checkbox(T("vehicle.noDamage"), &MenuState::VehicleNoDamage);
                ImGui::SameLine();
                ImGui::Checkbox(T("vehicle.autoUnflip"), &MenuState::VehicleAutoUnflip);
                ImGui::SameLine();
                ImGui::Checkbox(T("vehicle.heavy"), &MenuState::VehicleHeavy);
                ImGui::SameLine();
                ImGui::Checkbox(T("vehicle.watertight"), &MenuState::VehicleWatertight);

                ImGui::Checkbox(T("vehicle.lockSpeed"), &MenuState::VehicleSpeedLock);
                ImGui::PushItemWidth(150);
                ImGui::SliderFloat(T("vehicle.targetSpeed"), &MenuState::VehicleSpeed, 5.0f, 300.0f, "%.0f");
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("vehicle.spawnVehicle"))) {
                ImGui::Checkbox(T("vehicle.spawnAsDriver"), &MenuState::VehicleSpawnAsDriver);
                ImGui::SameLine();
                ImGui::Checkbox(T("vehicle.spawnAircraftInAir"), &MenuState::VehicleSpawnAircraftInAir);

                ImGui::TextWrapped("%s", T("vehicle.spawnIdTip"));
                ImGui::PushItemWidth(160);
                ImGui::InputInt(T("vehicle.modelId"), &MenuState::VehicleSpawnModel);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (UI::Button(T("vehicle.spawnById"), 2)) {
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