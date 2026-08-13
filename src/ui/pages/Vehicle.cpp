#include "Vehicle.h"
#include "controllers/Vehicle.h"
#include <XBase/Types.h>
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "integration/XBaseBridge.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include <XBase/UI.h>
#include <cstring>
#include <cstdio>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    void DrawVehicleList() {
        const Resources::VehicleTable table = Resources::GetVehicles();
        if (table.count == 0) {
            XBase::UI::TextWrapped(T("vehicle.noListData"));
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
                    XBase::UI::Spacing();
                    XBase::UI::SeparatorText(translatedCategory);
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
                UI::SameLine();
            }
        }

#ifdef GTASA
        if (XBase::UI::CollapsingHeader(T("vehicle.paint"), true)) {
            const bool hasVehicle = static_cast<bool>(Controllers::Vehicle::GetCurrentVehicleId());
            if (!hasVehicle) {
                XBase::UI::TextDisabled(T("vehicle.notInVehicle"));
            } else {
                bool applyCarcols = false;
                XBase::UI::PushItemWidth(120);
                if (XBase::UI::Input(T("vehicle.color1"), MenuState::VehicleColorPrimary)) {
                    applyCarcols = true;
                }
                XBase::UI::SameLine();
                if (XBase::UI::Input(T("vehicle.color2"), MenuState::VehicleColorSecondary)) {
                    applyCarcols = true;
                }
                if (XBase::UI::Input(T("vehicle.color3"), MenuState::VehicleColorTertiary)) {
                    applyCarcols = true;
                }
                XBase::UI::SameLine();
                if (XBase::UI::Input(T("vehicle.color4"), MenuState::VehicleColorQuaternary)) {
                    applyCarcols = true;
                }
                const bool canPaintjob = XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePaintjob);
                const bool canUpgrades = XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleUpgrades);
                XBase::UI::Disabled(!canPaintjob, [&] {
                    XBase::UI::Input(T("vehicle.paintjob"), MenuState::VehiclePaintjob);
                });
                XBase::UI::SameLine();
                XBase::UI::Disabled(!canUpgrades, [&] {
                    XBase::UI::Input(T("vehicle.modId"), MenuState::VehicleModId);
                });
                XBase::UI::PopItemWidth();

                if (applyCarcols) {
                    Controllers::Vehicle::ApplyCarcols();
                }
                XBase::UI::Disabled(!canPaintjob && !canUpgrades, [&] {
                    if (UI::Button(T("vehicle.applyPaintjobMod"), 2)) {
                        Controllers::Vehicle::ApplyAppearance();
                    }
                });
                XBase::UI::SameLine();
                if (UI::Button(T("vehicle.resetColors"))) {
                    Controllers::Vehicle::ResetColors();
                }

                UI::SpacingSeparator();
                XBase::UI::PushItemWidth(120);
                XBase::UI::Input(T("vehicle.doorIndex"), MenuState::VehicleDoorIndex);
                XBase::UI::SameLine();
                XBase::UI::Input(T("vehicle.seatIndex"), MenuState::VehicleSeatIndex);
                XBase::UI::PopItemWidth();
                if (UI::Button(T("vehicle.openDoor"), 3)) {
                    Controllers::Vehicle::OpenDoor();
                }
                XBase::UI::SameLine();
                const bool canPopDoors = XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePopDoors);
                if (canPopDoors && UI::Button(T("vehicle.popDoor"), 3)) {
                    Controllers::Vehicle::PopDoor();
                }
                if (!canPopDoors) {
                    XBase::UI::TextDisabled("当前版本不支持弹门");
                }
                XBase::UI::SameLine();
                if (UI::Button(T("vehicle.warpToSeat"), 3)) {
                    Controllers::Vehicle::WarpToSeat();
                }
            }
        }
#endif
    }
}

namespace Pages::Vehicle {
    void Process() {
        Controllers::Vehicle::ProcessHost();
    }

    void Draw() {
        const XBase::VehicleId currentVehicle = Controllers::Vehicle::GetCurrentVehicleId();
        const bool hasVehicle = static_cast<bool>(currentVehicle);
        static XBase::VehicleId lastVehicle;
        static float vehicleHealth = 1000.0f;
        if (currentVehicle.value != lastVehicle.value) {
            lastVehicle = currentVehicle;
            vehicleHealth = Controllers::Vehicle::GetHealth();
        }

        if (UI::Button(T("vehicle.blowUpAll"))) {
            Controllers::Vehicle::BlowUpAll();
        }

        XBase::UI::Spacing();

        if (!hasVehicle) {
            XBase::UI::Text(T("vehicle.notInVehicle"));
            XBase::UI::Spacing();
        } else {
            if (UI::Button(T("vehicle.repair"), 6)) {
                Controllers::Vehicle::Repair();
            }
            XBase::UI::SameLine();
            if (UI::Button(T("vehicle.stop"), 6)) {
                Controllers::Vehicle::Stop();
            }
            XBase::UI::SameLine();
            if (UI::Button(T("vehicle.unflip"), 6)) {
                Controllers::Vehicle::Unflip();
            }
            XBase::UI::SameLine();
            if (UI::Button(T("vehicle.start"), 6)) {
                Controllers::Vehicle::Start();
            }
            XBase::UI::SameLine();
            if (UI::Button(T("vehicle.engineOn"), 6)) {
                Controllers::Vehicle::SetEngine(true);
            }
            XBase::UI::SameLine();
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

            XBase::Types::ProofState proofs = Controllers::Vehicle::GetProofState();
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
            const auto drawVehicleAttribute = [&](XBase::FeatureCapability capability, auto&& draw) {
                XBase::UI::Disabled(!XBaseBridge::HasCapability(capability), draw);
            };

            drawVehicleAttribute(XBase::FeatureCapability::VehicleAlwaysSkidMarks, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetAlwaysSkidMarks(value);
                if (UI::Checkbox(T("vehicle.alwaysSkidMarks"), &value)) {
                    Controllers::Vehicle::SetAlwaysSkidMarks(value);
                }
            });
            UI::SameLine();
            drawVehicleAttribute(XBase::FeatureCapability::VehicleDisableParticles, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetDisableParticles(value);
                if (UI::Checkbox(T("vehicle.disableParticles"), &value)) {
                    Controllers::Vehicle::SetDisableParticles(value);
                }
            });
            UI::SameLine();
            drawVehicleAttribute(XBase::FeatureCapability::VehicleDriverTargetable, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetDriverTargetable(value);
                if (UI::Checkbox(T("vehicle.driverTargetable"), &value)) {
                    Controllers::Vehicle::SetDriverTargetable(value);
                }
            });

            drawVehicleAttribute(XBase::FeatureCapability::VehicleHeatSeekingTargetable, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetHeatSeekingTargetable(value);
                if (UI::Checkbox(T("vehicle.missileTargetable"), &value)) {
                    Controllers::Vehicle::SetHeatSeekingTargetable(value);
                }
            });
            UI::SameLine();
            drawVehicleAttribute(XBase::FeatureCapability::VehiclePetrolTankWeakPoint, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetPetrolTankWeakPoint(value);
                if (UI::Checkbox(T("vehicle.petrolTankWeakness"), &value)) {
                    Controllers::Vehicle::SetPetrolTankWeakPoint(value);
                }
            });
            UI::SameLine();
            drawVehicleAttribute(XBase::FeatureCapability::VehicleSirenOrAlarm, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetSirenOrAlarm(value);
                if (UI::Checkbox(T("vehicle.sirenAlarm"), &value)) {
                    Controllers::Vehicle::SetSirenOrAlarm(value);
                }
            });
            UI::SameLine();
            drawVehicleAttribute(XBase::FeatureCapability::VehicleTakeLessDamage, [&] {
                bool value = false;
                Controllers::Vehicle::TryGetTakeLessDamage(value);
                if (UI::Checkbox(T("vehicle.takeLessDamage"), &value)) {
                    Controllers::Vehicle::SetTakeLessDamage(value);
                }
            });
#endif

            XBase::UI::PushItemWidth(160);
            XBase::UI::Slider(T("vehicle.health"), vehicleHealth, 0.0f, 1000.0f, "%.0f");
            XBase::UI::PopItemWidth();
            UI::SameLine();
            if (UI::Button(T("vehicle.setHealth"))) {
                Controllers::Vehicle::SetHealth(vehicleHealth);
            }
            UI::SameLine();
            if (UI::Button(T("vehicle.readHealth"))) {
                vehicleHealth = Controllers::Vehicle::GetHealth();
            }
        }

        XBase::UI::Spacing();

            XBase::UI::Tabs("VehicleTabs", [&] {
                XBase::UI::Tab("VehicleToggles", T("common.toggles"), [&] {
                UI::Checkbox(T("vehicle.noDamage"), &MenuState::VehicleNoDamage);
                UI::SameLine();
                UI::Checkbox(T("vehicle.autoUnflip"), &MenuState::VehicleAutoUnflip);
                UI::SameLine();
                UI::Checkbox(T("vehicle.heavy"), &MenuState::VehicleHeavy);
                UI::SameLine();
                UI::Checkbox(T("vehicle.watertight"), &MenuState::VehicleWatertight);
                UI::SameLine();
                UI::Checkbox(T("vehicle.flyingCars"), &MenuState::VehicleFlyingCars);
#if defined(GTASA) || defined(GTAVC)
                UI::SameLine();
                UI::Checkbox(T("vehicle.boatFly"), &MenuState::VehicleBoatFly);
#endif
#if defined(GTASA) || defined(GTAVC)
                UI::SameLine();
                UI::Checkbox(T("vehicle.driveWater"), &MenuState::VehicleDriveWater);
                UI::SameLine();
                UI::Checkbox(T("vehicle.greenLights"), &MenuState::VehicleGreenLights);
#endif
#if defined(GTASA) || defined(GTA3)
                UI::SameLine();
                UI::Checkbox(T("vehicle.perfectHandling"), &MenuState::VehiclePerfectHandling);
#endif
#ifdef GTASA
                UI::SameLine();
                UI::Checkbox(T("vehicle.bikeFly"), &MenuState::VehicleBikeFly);
                UI::SameLine();
                UI::Checkbox(T("vehicle.stayOnBike"), &MenuState::VehicleStayOnBike);
                UI::SameLine();
                UI::Checkbox(T("vehicle.tankMode"), &MenuState::VehicleTankMode);
                UI::SameLine();
                UI::Checkbox(T("vehicle.aimDrive"), &MenuState::VehicleAimDrive);
                UI::SameLine();
                UI::Checkbox(T("vehicle.noDerail"), &MenuState::VehicleNoDerail);
                UI::SameLine();
                UI::Checkbox(T("vehicle.flipNoBurn"), &MenuState::VehicleFlipNoBurn);
                UI::SameLine();
                UI::Checkbox(T("vehicle.infNitro"), &MenuState::VehicleInfNitro);
                UI::Checkbox(T("vehicle.neon"), &MenuState::VehicleNeon);
                if (MenuState::VehicleNeon) {
                    UI::PushItemWidth(200);
                    UI::SliderInt(T("vehicle.neonR"), &MenuState::VehicleNeonColorR, 0, 255);
                    UI::SliderInt(T("vehicle.neonG"), &MenuState::VehicleNeonColorG, 0, 255);
                    UI::SliderInt(T("vehicle.neonB"), &MenuState::VehicleNeonColorB, 0, 255);
                    UI::PopItemWidth();
                }

                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleAutoDrive),
                    [&] {
                        if (UI::Checkbox(T("vehicle.autoDrive"), &MenuState::VehicleAutoDrive)) {
                            if (!MenuState::VehicleAutoDrive) {
                                Controllers::Vehicle::WarpToSeat();
                            }
                        }
                    });
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
                });

            XBase::UI::Tab("VehicleSpawn", T("vehicle.spawnVehicle"), [&] {
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
                });

            });
    }
}