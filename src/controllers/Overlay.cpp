#include "Overlay.h"
#include "Vehicle.h"
#include <XBase/Teleport.h>
#include <XBase/Types.h>
#include <XBase/UI.h>
#include <XBase/Player.h>
#include <XBase/Vehicle.h>
#include "Teleport.h"
#include "World.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "utils/UpdateChecker.h"
#include <cstdio>
#include <string>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    const char* VersionStatusText(UpdateChecker::VersionStatus status) {
        switch (status) {
        case UpdateChecker::VersionStatus::RemoteNewer:
            return T("status.remoteNewer");
        case UpdateChecker::VersionStatus::LocalNewer:
            return T("status.localNewer");
        case UpdateChecker::VersionStatus::Equal:
            return T("status.upToDate");
        case UpdateChecker::VersionStatus::Unknown:
        default:
            return T("status.remoteUnknown");
        }
    }

    void DrawDetailLine(const char* labelKey, const char* value) {
        XBase::UI::TextWrapped(T("overlay.detailLine"), T(labelKey), value ? value : "");
    }

    void DrawSectionTitle(const char* labelKey) {
        XBase::UI::Spacing();
        XBase::UI::TextDisabled(T(labelKey));
    }

    void AppendEnabled(std::string& value, const char* labelKey) {
        if (!value.empty()) {
            value += "  ";
        }
        value += T(labelKey);
    }

    void AppendProof(std::string& value, const char* labelKey, bool enabled) {
        if (enabled) {
            AppendEnabled(value, labelKey);
        }
    }

    void DrawEnabledFeature(std::string& features, const char* labelKey, bool enabled) {
        if (enabled) {
            AppendEnabled(features, labelKey);
        }
    }
}

namespace Controllers::Overlay {
    void Draw() {
        if (!MenuState::OverlayEnabled) {
            return;
        }

        constexpr XBase::UI::WindowFlags overlayFlags =
            XBase::UI::Flag(XBase::UI::WindowFlag::NoTitleBar) |
            XBase::UI::Flag(XBase::UI::WindowFlag::NoResize) |
            XBase::UI::Flag(XBase::UI::WindowFlag::AlwaysAutoResize) |
            XBase::UI::Flag(XBase::UI::WindowFlag::NoMove) |
            XBase::UI::Flag(XBase::UI::WindowFlag::NoSavedSettings) |
            XBase::UI::Flag(XBase::UI::WindowFlag::NoFocusOnAppearing) |
            XBase::UI::Flag(XBase::UI::WindowFlag::NoNavigation);

        XBase::UI::SetNextWindowBackgroundAlpha(0.35f);
        XBase::UI::SetNextWindowPosition({12.0f, 12.0f}, true);
        XBase::UI::Window("XMenuOverlay", "XMenuOverlay", [&] {
            XBase::UI::Text(T("overlay.title"));

            const XBase::Player::PlayerSnapshot player = XBase::Player::GetSnapshot();
            const XBase::Vehicle::VehicleSnapshot vehicle = XBase::Vehicle::GetSnapshot();
            int hour = 0;
            int minute = 0;
            Controllers::World::GetTime(hour, minute);

            if (MenuState::OverlayShowFps) {
                XBase::UI::Text(T("overlay.fps"), XBase::UI::GetFrameRate());
            }
            if (MenuState::OverlayShowPosition) {
                XBase::Vec3 pos;
                if (XBase::Teleport::TryGetCurrentPosition(pos)) {
                    XBase::UI::Text(T("overlay.position"), pos.x, pos.y, pos.z);
                }
            }
            if (MenuState::OverlayShowTime) {
                XBase::UI::Text(T("overlay.time"), hour, minute);
            }
            if (MenuState::OverlayShowPlayer) {
                XBase::UI::Text(T("overlay.player"), player.health, player.armour, player.wantedLevel, player.money);
            }

            const UpdateChecker::UpdateInfo updateInfo = UpdateChecker::GetUpdateInfo();
            if (updateInfo.status == UpdateChecker::VersionStatus::RemoteNewer || updateInfo.status == UpdateChecker::VersionStatus::LocalNewer) {
                char versionDetail[192] = {};
                std::snprintf(versionDetail, sizeof(versionDetail), T("overlay.versionStatusValue"), VersionStatusText(updateInfo.status), updateInfo.currentVersion.c_str(), updateInfo.latestVersion.empty() ? T("status.remoteUnknown") : updateInfo.latestVersion.c_str());
                DrawDetailLine("overlay.versionStatusLabel", versionDetail);
            }

            if (MenuState::OverlayShowDetails) {
                DrawSectionTitle("overlay.detailsTitle");

                if (MenuState::OverlayShowWorld) {
                    char worldDetail[128] = {};
                    std::snprintf(worldDetail, sizeof(worldDetail), T("overlay.worldDetailValue"), Controllers::World::GetGameSpeed(), Controllers::World::GetGravity(), Controllers::World::GetDaysPassed(), Controllers::World::GetFpsLimit());
                    DrawDetailLine("overlay.worldDetailLabel", worldDetail);
                }

                std::string teleportDetail;
                if (MenuState::TeleportForwardHold) {
                    AppendEnabled(teleportDetail, "overlay.teleportContinuous");
                }
                if (MenuState::QuickTeleport) {
                    AppendEnabled(teleportDetail, "overlay.teleportQuickMap");
                }
                if (MenuState::SpawnUnderwater) {
                    AppendEnabled(teleportDetail, "overlay.teleportUnderwater");
                }
                if (!teleportDetail.empty()) {
                    DrawDetailLine("overlay.teleportDetailLabel", teleportDetail.c_str());
                }

                if (MenuState::OverlayShowPlayer) {
                    const XBase::Types::ProofState& playerProofs = player.proofs;
                    std::string playerProofDetail;
                    AppendProof(playerProofDetail, "overlay.proofBullet", playerProofs.bullet);
                    AppendProof(playerProofDetail, "overlay.proofCollision", playerProofs.collision);
                    AppendProof(playerProofDetail, "overlay.proofExplosion", playerProofs.explosion);
                    AppendProof(playerProofDetail, "overlay.proofFire", playerProofs.fire);
                    AppendProof(playerProofDetail, "overlay.proofMelee", playerProofs.melee);
                    AppendProof(playerProofDetail, "overlay.proofNonPlayer", playerProofs.nonPlayer);
                    if (!playerProofDetail.empty()) {
                        DrawDetailLine("overlay.playerProofLabel", playerProofDetail.c_str());
                    }
                }

                if (MenuState::OverlayShowVehicle) {
                    if (vehicle.id) {
                        char vehicleDetail[96] = {};
                        std::snprintf(vehicleDetail, sizeof(vehicleDetail), T("overlay.vehicleDetailValue"), vehicle.modelId, vehicle.health);
                        DrawDetailLine("overlay.vehicleDetailLabel", vehicleDetail);

                        std::string vehicleState;
                        if (vehicle.lights) {
                            AppendEnabled(vehicleState, "overlay.vehicleLights");
                        }
                        if (vehicle.locked) {
                            AppendEnabled(vehicleState, "overlay.vehicleLocked");
                        }
                        if (!vehicleState.empty()) {
                            DrawDetailLine("overlay.vehicleStateLabel", vehicleState.c_str());
                        }

                        const XBase::Types::ProofState vehicleProofs = Controllers::Vehicle::GetProofState();
                        std::string vehicleProofDetail;
                        AppendProof(vehicleProofDetail, "overlay.proofBullet", vehicleProofs.bullet);
                        AppendProof(vehicleProofDetail, "overlay.proofCollision", vehicleProofs.collision);
                        AppendProof(vehicleProofDetail, "overlay.proofExplosion", vehicleProofs.explosion);
                        AppendProof(vehicleProofDetail, "overlay.proofFire", vehicleProofs.fire);
                        AppendProof(vehicleProofDetail, "overlay.proofMelee", vehicleProofs.melee);
                        AppendProof(vehicleProofDetail, "overlay.proofNonPlayer", vehicleProofs.nonPlayer);
                        if (!vehicleProofDetail.empty()) {
                            DrawDetailLine("overlay.vehicleProofLabel", vehicleProofDetail.c_str());
                        }
                    } else {
                        DrawDetailLine("overlay.vehicleDetailLabel", T("common.no"));
                    }
                }
            }

            if (MenuState::OverlayShowFeatures) {
                std::string features;
                DrawEnabledFeature(features, "overlay.godMode", MenuState::GodMode);
                DrawEnabledFeature(features, "overlay.autoHeal", MenuState::AutoHeal);
                DrawEnabledFeature(features, "overlay.infiniteSprint", MenuState::InfiniteSprint);
                DrawEnabledFeature(features, "overlay.freeFlight", MenuState::FreeFlyEnabled);
                DrawEnabledFeature(features, "overlay.vehicleNoDamage", MenuState::VehicleNoDamage);
                DrawEnabledFeature(features, "overlay.vehicleSpeedLock", MenuState::VehicleSpeedLock);
                DrawEnabledFeature(features, "overlay.vehicleFlyingCars", MenuState::VehicleFlyingCars);
                DrawEnabledFeature(features, "overlay.neverWanted", MenuState::NeverWanted);
                DrawEnabledFeature(features, "overlay.driveWater", MenuState::VehicleDriveWater);
                DrawEnabledFeature(features, "overlay.weaponAutoAim", MenuState::WeaponAutoAim);
                DrawEnabledFeature(features, "overlay.infiniteAmmo", MenuState::InfiniteAmmo);
                DrawEnabledFeature(features, "overlay.rapidFire", MenuState::RapidFire);
                DrawEnabledFeature(features, "overlay.fireRate", MenuState::WeaponFireRateEnabled);
                DrawEnabledFeature(features, "overlay.gangWars", MenuState::GangWarsActive);
                DrawEnabledFeature(features, "overlay.worldTimeLock", MenuState::WorldLockTime || MenuState::FreezeTime);
                DrawEnabledFeature(features, "overlay.worldWeatherLock", MenuState::LockWeather);
                DrawEnabledFeature(features, "overlay.worldFastClock", MenuState::FasterClock);
                DrawEnabledFeature(features, "overlay.solidWater", MenuState::SolidWater);
                DrawEnabledFeature(features, "overlay.noWaterPhysics", MenuState::NoWaterPhysics);
                DrawEnabledFeature(features, "overlay.disableReplay", MenuState::DisableReplay);
                DrawEnabledFeature(features, "overlay.disableCheats", MenuState::DisableCheats);

                if (!features.empty()) {
                    DrawSectionTitle("overlay.featuresTitle");
                    XBase::UI::TextWrapped(features.c_str());
                }
            }
        }, nullptr, overlayFlags);
    }
}