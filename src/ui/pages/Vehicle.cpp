#include "Vehicle.h"
#include "controllers/Vehicle.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include "extensions/ScriptCommands.h"
#include "CPools.h"
#include "CPlayerPed.h"
#include "plugin.h"
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

            static std::unordered_map<std::string, bool> categoryOpen;

            if (currentCategoryKey != vehicle.category) {
                currentCategoryKey = vehicle.category;
                index = 0;
            }
            
            bool isOpen = categoryOpen[currentCategoryKey];
            if (i == 0 || currentCategoryKey != table.entries->at(i - 1).category) {
                if (MenuState::UseNativeMenu) {
                    UI::CollapsingHeader(translatedCategory, categoryOpen[currentCategoryKey]);
                } else {
                    ImGui::Spacing();
                    ImGui::SeparatorText(translatedCategory);
                    categoryOpen[currentCategoryKey] = true; // Always show in native panel mode
                }
            }
            isOpen = categoryOpen[currentCategoryKey];

            if (isOpen) {
                const char* englishName = I18n::T(I18n::Language::En, vehicle.name.c_str());
                char buttonLabel[96];
                std::snprintf(buttonLabel, sizeof(buttonLabel), "%s (%d)", englishName, vehicle.id);
                if (UI::Button(buttonLabel, 3)) {
                    Controllers::Vehicle::Spawn(static_cast<unsigned int>(vehicle.id));
                }
                UI::SameLineEvery(index++, 3);
            }
        }

#ifdef GTASA
        if (ImGui::CollapsingHeader(T("vehicle.paint"), ImGuiTreeNodeFlags_DefaultOpen)) {
            CVehicle* vehicle = Controllers::Vehicle::GetCurrentVehicle();
            if (!vehicle) {
                ImGui::TextDisabled("%s", T("vehicle.notInVehicle"));
            } else {
                bool applyCarcols = false;
                ImGui::PushItemWidth(120);
                if (ImGui::InputInt(T("vehicle.color1"), &MenuState::VehicleColorPrimary)) {
                    applyCarcols = true;
                }
                ImGui::SameLine();
                if (ImGui::InputInt(T("vehicle.color2"), &MenuState::VehicleColorSecondary)) {
                    applyCarcols = true;
                }
                if (ImGui::InputInt(T("vehicle.color3"), &MenuState::VehicleColorTertiary)) {
                    applyCarcols = true;
                }
                ImGui::SameLine();
                if (ImGui::InputInt(T("vehicle.color4"), &MenuState::VehicleColorQuaternary)) {
                    applyCarcols = true;
                }
                ImGui::PopItemWidth();

                if (applyCarcols) {
                    Controllers::Vehicle::ApplyCarcols();
                }

                if (UI::Button(T("vehicle.resetColors"))) {
                    Controllers::Vehicle::ResetColors();
                }
            }
        }
#endif
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

            UI::SpacingSeparator();
            bool lights = Controllers::Vehicle::GetLights();
            if (UI::Checkbox(T("vehicle.lights"), &lights)) {
                Controllers::Vehicle::SetLights(lights);
            }
            UI::SameLine();
            bool locked = Controllers::Vehicle::GetLocked();
            if (UI::Checkbox(T("vehicle.lockDoors"), &locked)) {
                Controllers::Vehicle::SetLocked(locked);
            }
            UI::SameLine();
            bool visible = Controllers::Vehicle::GetVisible();
            bool invisible = !visible;
            if (UI::Checkbox(T("vehicle.invisible"), &invisible)) {
                Controllers::Vehicle::SetVisible(!invisible);
            }

            GameLogic::ProofState proofs = Controllers::Vehicle::GetProofState();
            if (UI::Checkbox(T("proof.bullet"), &proofs.bullet)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            UI::SameLine();
            if (UI::Checkbox(T("proof.collision"), &proofs.collision)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            UI::SameLine();
            if (UI::Checkbox(T("proof.explosion"), &proofs.explosion)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            UI::SameLine();
            if (UI::Checkbox(T("proof.fire"), &proofs.fire)) {
                Controllers::Vehicle::SetProofState(proofs);
            }
            UI::SameLine();
            if (UI::Checkbox(T("proof.melee"), &proofs.melee)) {
                Controllers::Vehicle::SetProofState(proofs);
            }

#if GTASA
            bool skidMarks = Controllers::Vehicle::GetAlwaysSkidMarks();
            if (UI::Checkbox(T("vehicle.alwaysSkidMarks"), &skidMarks)) {
                Controllers::Vehicle::SetAlwaysSkidMarks(skidMarks);
            }
            UI::SameLine();
            bool disableParticles = Controllers::Vehicle::GetDisableParticles();
            if (UI::Checkbox(T("vehicle.disableParticles"), &disableParticles)) {
                Controllers::Vehicle::SetDisableParticles(disableParticles);
            }
            UI::SameLine();
            bool driverTargetable = Controllers::Vehicle::GetDriverTargetable();
            if (UI::Checkbox(T("vehicle.driverTargetable"), &driverTargetable)) {
                Controllers::Vehicle::SetDriverTargetable(driverTargetable);
            }

            bool heatSeekingTargetable = Controllers::Vehicle::GetHeatSeekingTargetable();
            if (UI::Checkbox(T("vehicle.missileTargetable"), &heatSeekingTargetable)) {
                Controllers::Vehicle::SetHeatSeekingTargetable(heatSeekingTargetable);
            }
            UI::SameLine();
            bool petrolTankWeakPoint = Controllers::Vehicle::GetPetrolTankWeakPoint();
            if (UI::Checkbox(T("vehicle.petrolTankWeakness"), &petrolTankWeakPoint)) {
                Controllers::Vehicle::SetPetrolTankWeakPoint(petrolTankWeakPoint);
            }
            UI::SameLine();
            bool sirenOrAlarm = Controllers::Vehicle::GetSirenOrAlarm();
            if (UI::Checkbox(T("vehicle.sirenAlarm"), &sirenOrAlarm)) {
                Controllers::Vehicle::SetSirenOrAlarm(sirenOrAlarm);
            }
            UI::SameLine();
            bool takeLessDamage = Controllers::Vehicle::GetTakeLessDamage();
            if (UI::Checkbox(T("vehicle.takeLessDamage"), &takeLessDamage)) {
                Controllers::Vehicle::SetTakeLessDamage(takeLessDamage);
            }
#endif

            UI::PushItemWidth(160);
            UI::SliderFloat(T("vehicle.health"), &vehicleHealth, 0.0f, 1000.0f, "%.0f");
            UI::PopItemWidth();
            UI::SameLine();
            if (UI::Button(T("vehicle.setHealth"))) {
                Controllers::Vehicle::SetHealth(vehicleHealth);
            }
            UI::SameLine();
            if (UI::Button(T("vehicle.readHealth"))) {
                vehicleHealth = Controllers::Vehicle::GetHealth();
            }
        }

        ImGui::Spacing();

        if (UI::BeginTabBar("VehicleTabs")) {
            if (UI::BeginTab("VehicleToggles", T("common.toggles"))) {
                UI::Checkbox(T("vehicle.noDamage"), &MenuState::VehicleNoDamage);
                UI::SameLine();
                UI::Checkbox(T("vehicle.autoUnflip"), &MenuState::VehicleAutoUnflip);
                UI::SameLine();
                UI::Checkbox(T("vehicle.heavy"), &MenuState::VehicleHeavy);
                UI::SameLine();
                UI::Checkbox(T("vehicle.watertight"), &MenuState::VehicleWatertight);
                UI::SameLine();
#ifdef GTASA
                UI::Checkbox(T("vehicle.neon"), &MenuState::VehicleNeon);
                if (MenuState::VehicleNeon) {
                    UI::PushItemWidth(200);
                    UI::SliderInt(T("vehicle.neonR"), &MenuState::VehicleNeonColorR, 0, 255);
                    UI::SliderInt(T("vehicle.neonG"), &MenuState::VehicleNeonColorG, 0, 255);
                    UI::SliderInt(T("vehicle.neonB"), &MenuState::VehicleNeonColorB, 0, 255);
                    UI::PopItemWidth();
                }

                if (UI::Checkbox(T("vehicle.autoDrive"), &MenuState::VehicleAutoDrive)) {
                    if (!MenuState::VehicleAutoDrive) {
                        CVehicle* veh = Controllers::Vehicle::GetCurrentVehicle();
                        if (veh) {
                            plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(CPools::GetPedRef(FindPlayerPed()), CPools::GetVehicleRef(veh));
                        }
                    }
                }
#endif

                if (UI::Checkbox(T("vehicle.lockSpeed"), &MenuState::VehicleSpeedLock)) {
                    Controllers::Vehicle::ApplySpeedLock();
                }
                UI::PushItemWidth(150);
                if (UI::SliderFloat(T("vehicle.targetSpeed"), &MenuState::VehicleSpeed, 5.0f, 300.0f, "%.0f")) {
                    Controllers::Vehicle::ApplySpeedLock();
                }
                UI::PopItemWidth();
                UI::SameLine();
                if (UI::Button(T("vehicle.applyTargetSpeed"))) {
                    Controllers::Vehicle::ApplyTargetSpeed();
                }
                UI::SameLine();
                if (UI::Button(T("vehicle.restoreDefaultSpeed"))) {
                    Controllers::Vehicle::RestoreDefaultTargetSpeed();
                }
                UI::EndTab();
            }

            if (UI::BeginTab("VehicleSpawn", T("vehicle.spawnVehicle"))) {
                UI::Checkbox(T("vehicle.spawnAsDriver"), &MenuState::VehicleSpawnAsDriver);
                UI::SameLine();
                UI::Checkbox(T("vehicle.spawnAircraftInAir"), &MenuState::VehicleSpawnAircraftInAir);
                UI::SameLine();
                UI::Checkbox(T("vehicle.cleanupAfterSpawn"), &MenuState::VehicleCleanupAfterSpawn);

                UI::TextCentered(T("vehicle.spawnIdTip"));
                UI::PushItemWidth(160);
                UI::InputInt(T("vehicle.modelId"), &MenuState::VehicleSpawnModel);
                UI::PopItemWidth();
                UI::SameLine();
                if (UI::Button(T("vehicle.spawnById"), 2)) {
                    if (MenuState::VehicleSpawnModel >= 0) {
                        Controllers::Vehicle::Spawn(static_cast<unsigned int>(MenuState::VehicleSpawnModel));
                    }
                }

                UI::SpacingSeparator();
                DrawVehicleList();
                UI::EndTab();
            }

            UI::EndTabBar();
        }
    }
}