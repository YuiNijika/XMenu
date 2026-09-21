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
                    categoryOpen[currentCategoryKey] = true;
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
    }

    void DrawSectionTitle(const char* key) {
        XBase::UI::Spacing();
        if (!MenuState::UseNativeMenu) {
            XBase::UI::Text(T(key));
        }
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

        if (UI::Button(T("vehicle.repair"), 6)) {
            Controllers::Vehicle::Repair();
        }
        UI::SameLine();
        if (UI::Button(T("vehicle.stop"), 6)) {
            Controllers::Vehicle::Stop();
        }
        UI::SameLine();
        if (UI::Button(T("vehicle.unflip"), 6)) {
            Controllers::Vehicle::Unflip();
        }
        UI::SameLine();
        if (UI::Button(T("vehicle.start"), 6)) {
            Controllers::Vehicle::Start();
        }
        UI::SameLine();
        if (UI::Button(T("vehicle.engineOn"), 6)) {
            Controllers::Vehicle::SetEngine(true);
        }
        UI::SameLine();
        if (UI::Button(T("vehicle.engineOff"), 6)) {
            Controllers::Vehicle::SetEngine(false);
        }

        if (!hasVehicle) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("vehicle.notInVehicle"));
        }

        UI::SpacingSeparator();

        XBase::UI::Tabs("VehicleTabs", [&] {
            XBase::UI::Tab("VehicleToggles", T("common.toggles"), [&] {
                DrawSectionTitle("vehicle.sectionRuntime");
                UI::Columns(4, nullptr, false);
                UI::Checkbox(T("vehicle.noDamage"), &MenuState::VehicleNoDamage);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.autoUnflip"), &MenuState::VehicleAutoUnflip);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.heavy"), &MenuState::VehicleHeavy);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.watertight"), &MenuState::VehicleWatertight);
                UI::Columns(1);

                if (hasVehicle) {
                    DrawSectionTitle("vehicle.sectionStatus");
                    UI::Columns(3, nullptr, false);
                    bool lights = Controllers::Vehicle::GetLights();
                    if (UI::Checkbox(T("vehicle.lights"), &lights)) {
                        Controllers::Vehicle::SetLights(lights);
                    }
                    UI::NextColumn();
                    bool locked = Controllers::Vehicle::GetLocked();
                    if (UI::Checkbox(T("vehicle.lockDoors"), &locked)) {
                        Controllers::Vehicle::SetLocked(locked);
                    }
                    UI::NextColumn();
                    bool visible = Controllers::Vehicle::GetVisible();
                    bool invisible = !visible;
                    if (UI::Checkbox(T("vehicle.invisible"), &invisible)) {
                        Controllers::Vehicle::SetVisible(!invisible);
                    }
                    UI::Columns(1);

                    DrawSectionTitle("vehicle.sectionProof");
                    XBase::Types::ProofState proofs = Controllers::Vehicle::GetProofState();
                    UI::Columns(5, nullptr, false);
                    if (UI::Checkbox(T("proof.bullet"), &proofs.bullet)) {
                        Controllers::Vehicle::SetProofState(proofs);
                    }
                    UI::NextColumn();
                    if (UI::Checkbox(T("proof.collision"), &proofs.collision)) {
                        Controllers::Vehicle::SetProofState(proofs);
                    }
                    UI::NextColumn();
                    if (UI::Checkbox(T("proof.explosion"), &proofs.explosion)) {
                        Controllers::Vehicle::SetProofState(proofs);
                    }
                    UI::NextColumn();
                    if (UI::Checkbox(T("proof.fire"), &proofs.fire)) {
                        Controllers::Vehicle::SetProofState(proofs);
                    }
                    UI::NextColumn();
                    if (UI::Checkbox(T("proof.melee"), &proofs.melee)) {
                        Controllers::Vehicle::SetProofState(proofs);
                    }
                    UI::Columns(1);

                    DrawSectionTitle("vehicle.sectionHealth");
                    UI::PushItemWidth(200);
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

                    XBase::UI::Disabled(
                        !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleAlwaysSkidMarks)
                            && !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleDisableParticles)
                            && !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleDriverTargetable)
                            && !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleHeatSeekingTargetable)
                            && !XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePetrolTankWeakPoint)
                            && !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleSirenOrAlarm)
                            && !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleTakeLessDamage),
                        [&] {
                    DrawSectionTitle("vehicle.sectionSpecial");
                    const auto drawVehicleAttribute = [&](XBase::FeatureCapability capability, auto&& draw) {
                        XBase::UI::Disabled(!XBaseBridge::HasCapability(capability), draw);
                    };
                    UI::Columns(4, nullptr, false);
                    drawVehicleAttribute(XBase::FeatureCapability::VehicleAlwaysSkidMarks, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetAlwaysSkidMarks(value);
                        if (UI::Checkbox(T("vehicle.alwaysSkidMarks"), &value)) {
                            Controllers::Vehicle::SetAlwaysSkidMarks(value);
                        }
                    });
                    UI::NextColumn();
                    drawVehicleAttribute(XBase::FeatureCapability::VehicleDisableParticles, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetDisableParticles(value);
                        if (UI::Checkbox(T("vehicle.disableParticles"), &value)) {
                            Controllers::Vehicle::SetDisableParticles(value);
                        }
                    });
                    UI::NextColumn();
                    drawVehicleAttribute(XBase::FeatureCapability::VehicleDriverTargetable, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetDriverTargetable(value);
                        if (UI::Checkbox(T("vehicle.driverTargetable"), &value)) {
                            Controllers::Vehicle::SetDriverTargetable(value);
                        }
                    });
                    UI::NextColumn();
                    drawVehicleAttribute(XBase::FeatureCapability::VehicleHeatSeekingTargetable, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetHeatSeekingTargetable(value);
                        if (UI::Checkbox(T("vehicle.missileTargetable"), &value)) {
                            Controllers::Vehicle::SetHeatSeekingTargetable(value);
                        }
                    });
                    UI::NextColumn();
                    drawVehicleAttribute(XBase::FeatureCapability::VehiclePetrolTankWeakPoint, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetPetrolTankWeakPoint(value);
                        if (UI::Checkbox(T("vehicle.petrolTankWeakness"), &value)) {
                            Controllers::Vehicle::SetPetrolTankWeakPoint(value);
                        }
                    });
                    UI::NextColumn();
                    drawVehicleAttribute(XBase::FeatureCapability::VehicleSirenOrAlarm, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetSirenOrAlarm(value);
                        if (UI::Checkbox(T("vehicle.sirenAlarm"), &value)) {
                            Controllers::Vehicle::SetSirenOrAlarm(value);
                        }
                    });
                    UI::NextColumn();
                    drawVehicleAttribute(XBase::FeatureCapability::VehicleTakeLessDamage, [&] {
                        bool value = false;
                        Controllers::Vehicle::TryGetTakeLessDamage(value);
                        if (UI::Checkbox(T("vehicle.takeLessDamage"), &value)) {
                            Controllers::Vehicle::SetTakeLessDamage(value);
                        }
                    });
                    UI::Columns(1);
                    });
                }

                DrawSectionTitle("vehicle.sectionCheat");
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleCheats), [&] {
                UI::Columns(4, nullptr, false);
                UI::Checkbox(T("vehicle.flyingCars"), &MenuState::VehicleFlyingCars);
                UI::NextColumn();
#if defined(GTASA) || defined(GTAVC)
                UI::Checkbox(T("vehicle.boatFly"), &MenuState::VehicleBoatFly);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.driveWater"), &MenuState::VehicleDriveWater);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.greenLights"), &MenuState::VehicleGreenLights);
                UI::NextColumn();
#endif
#if defined(GTASA) || defined(GTA3)
                UI::Checkbox(T("vehicle.perfectHandling"), &MenuState::VehiclePerfectHandling);
                UI::NextColumn();
#endif
#ifdef GTASA
                UI::Checkbox(T("vehicle.bikeFly"), &MenuState::VehicleBikeFly);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.stayOnBike"), &MenuState::VehicleStayOnBike);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.tankMode"), &MenuState::VehicleTankMode);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.aimDrive"), &MenuState::VehicleAimDrive);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.noDerail"), &MenuState::VehicleNoDerail);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.flipNoBurn"), &MenuState::VehicleFlipNoBurn);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.infNitro"), &MenuState::VehicleInfNitro);
                UI::NextColumn();
#endif
                UI::Columns(1);
                });

#ifdef GTASA
                DrawSectionTitle("vehicle.sectionEffect");
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

                DrawSectionTitle("vehicle.sectionSpeed");
                if (UI::Checkbox(T("vehicle.lockSpeed"), &MenuState::VehicleSpeedLock)) {
                    Controllers::Vehicle::ApplySpeedLock();
                }
                UI::PushItemWidth(200);
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
                UI::Columns(3, nullptr, false);
                UI::Checkbox(T("vehicle.spawnAsDriver"), &MenuState::VehicleSpawnAsDriver);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.spawnAircraftInAir"), &MenuState::VehicleSpawnAircraftInAir);
                UI::NextColumn();
                UI::Checkbox(T("vehicle.cleanupAfterSpawn"), &MenuState::VehicleCleanupAfterSpawn);
                UI::Columns(1);

                UI::PushItemWidth(160);
                UI::InputInt(T("vehicle.modelId"), &MenuState::VehicleSpawnModel);
                UI::PopItemWidth();
                UI::SameLine();
                if (UI::Button(T("vehicle.spawnById"), 2)) {
                    if (MenuState::VehicleSpawnModel >= 0) {
                        Controllers::Vehicle::Spawn(static_cast<unsigned int>(MenuState::VehicleSpawnModel));
                    }
                }
                UI::SameLine();
                XBase::UI::TextDisabled(T("vehicle.spawnIdTip"));

                UI::SpacingSeparator();
                DrawVehicleList();
                });

            XBase::UI::Tab("VehiclePaint", T("vehicle.paint"), [&] {
                if (!hasVehicle) {
                    XBase::UI::TextDisabled(T("vehicle.notInVehicle"));
                    return;
                }

                const bool canColors = XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleColors);
                const bool canPaintjob = XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePaintjob);
                const bool canUpgrades = XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleUpgrades);

                DrawSectionTitle("vehicle.sectionPaint");
                XBase::UI::Disabled(!canColors, [&] {
                    bool applyCarcols = false;
                    UI::Columns(2, nullptr, false);
                    UI::PushItemWidth(160);
                    if (XBase::UI::Input(T("vehicle.color1"), MenuState::VehicleColorPrimary)) {
                        applyCarcols = true;
                    }
                    UI::NextColumn();
                    if (XBase::UI::Input(T("vehicle.color2"), MenuState::VehicleColorSecondary)) {
                        applyCarcols = true;
                    }
                    UI::NextColumn();
                    if (XBase::UI::Input(T("vehicle.color3"), MenuState::VehicleColorTertiary)) {
                        applyCarcols = true;
                    }
                    UI::NextColumn();
                    if (XBase::UI::Input(T("vehicle.color4"), MenuState::VehicleColorQuaternary)) {
                        applyCarcols = true;
                    }
                    UI::NextColumn();
                    UI::PopItemWidth();
                    UI::Columns(1);
                    if (applyCarcols) {
                        Controllers::Vehicle::ApplyCarcols();
                    }
                });

                UI::PushItemWidth(160);
                XBase::UI::Disabled(!canPaintjob, [&] {
                    XBase::UI::Input(T("vehicle.paintjob"), MenuState::VehiclePaintjob);
                });
                UI::SameLine();
                XBase::UI::Disabled(!canUpgrades, [&] {
                    XBase::UI::Input(T("vehicle.modId"), MenuState::VehicleModId);
                });
                UI::PopItemWidth();

                XBase::UI::Disabled(!canPaintjob && !canUpgrades, [&] {
                    if (UI::Button(T("vehicle.applyPaintjobMod"), 2)) {
                        Controllers::Vehicle::ApplyAppearance();
                    }
                });
                UI::SameLine();
                XBase::UI::Disabled(!canColors, [&] {
                    if (UI::Button(T("vehicle.resetColors"))) {
                        Controllers::Vehicle::ResetColors();
                    }
                });

                DrawSectionTitle("vehicle.sectionDoors");
                UI::PushItemWidth(160);
                UI::InputInt(T("vehicle.doorIndex"), &MenuState::VehicleDoorIndex);
                UI::SameLine();
                UI::InputInt(T("vehicle.seatIndex"), &MenuState::VehicleSeatIndex);
                UI::PopItemWidth();
                const bool canOpenDoors = XBaseBridge::HasCapability(XBase::FeatureCapability::VehicleDoors);
                XBase::UI::Disabled(!canOpenDoors, [&] {
                    if (UI::Button(T("vehicle.openDoor"), 3)) {
                        Controllers::Vehicle::OpenDoor();
                    }
                });
                UI::SameLine();
                const bool canPopDoors = XBaseBridge::HasCapability(XBase::FeatureCapability::VehiclePopDoors);
                XBase::UI::Disabled(!canPopDoors, [&] {
                    if (UI::Button(T("vehicle.popDoor"), 3)) {
                        Controllers::Vehicle::PopDoor();
                    }
                });
                UI::SameLine();
                if (UI::Button(T("vehicle.warpToSeat"), 3)) {
                    Controllers::Vehicle::WarpToSeat();
                }
                });
        });
    }
}
