#include "Overlay.h"
#include "Player.h"
#include "Vehicle.h"
#include "Teleport.h"
#include "World.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include "CVehicle.h"
#include <cstdio>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    const char* BoolText(bool value) {
        return T(value ? "common.yes" : "common.no");
    }

    void DrawDetailLine(const char* labelKey, const char* value) {
        ImGui::Text(T("overlay.detailLine"), T(labelKey), value ? value : "");
    }

    void DrawSectionTitle(const char* labelKey) {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", T(labelKey));
    }
}

namespace Controllers::Overlay {
    void Process() {
    }

    void Draw() {
        if (!MenuState::OverlayEnabled) {
            return;
        }

        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_Always);
        if (ImGui::Begin("XMenuOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            ImGui::TextUnformatted(T("overlay.title"));

            CVehicle* vehicle = Controllers::Vehicle::GetCurrentVehicle();
            int hour = 0;
            int minute = 0;
            Controllers::World::GetTime(hour, minute);

            if (MenuState::OverlayShowPosition) {
                const CVector pos = Controllers::Teleport::GetCurrentPosition();
                ImGui::Text(T("overlay.position"), pos.x, pos.y, pos.z);
            }
            if (MenuState::OverlayShowPlayer) {
                ImGui::Text(T("overlay.player"), Controllers::Player::GetHealth(), Controllers::Player::GetArmour(), Controllers::Player::GetWantedLevel(), Controllers::Player::GetMoney());
            }
            if (MenuState::OverlayShowVehicle) {
                ImGui::Text(T("overlay.vehicle"), vehicle ? T("common.yes") : T("common.no"));
            }
            if (MenuState::OverlayShowTime) {
                ImGui::Text(T("overlay.time"), hour, minute);
            }
            if (MenuState::OverlayShowWorld) {
                ImGui::Text(T("overlay.world"), hour, minute, Controllers::World::GetGameSpeed());
            }

            if (MenuState::OverlayShowDetails) {
                DrawSectionTitle("overlay.detailsTitle");
                DrawDetailLine("overlay.interiorLabel", "0");
                if (vehicle) {
                    char vehicleDetail[96] = {};
                    std::snprintf(vehicleDetail, sizeof(vehicleDetail), T("overlay.vehicleDetailValue"), vehicle->m_nModelIndex, Controllers::Vehicle::GetHealth());
                    DrawDetailLine("overlay.vehicleDetailLabel", vehicleDetail);
                }
            }

            if (MenuState::OverlayShowFeatures) {
                DrawSectionTitle("overlay.featuresTitle");
                DrawDetailLine("overlay.worldTimeLock", BoolText(MenuState::WorldLockTime));
                DrawDetailLine("overlay.worldWeatherLock", BoolText(MenuState::LockWeather));
                DrawDetailLine("overlay.worldFastClock", BoolText(MenuState::FasterClock));
                DrawDetailLine("overlay.vehicleSpeedLock", BoolText(MenuState::VehicleSpeedLock));
                DrawDetailLine("overlay.freeFlight", BoolText(MenuState::FreeFlyEnabled));
            }
            if (MenuState::OverlayShowFps) {
                ImGui::Text(T("overlay.fps"), ImGui::GetIO().Framerate);
            }
        }
        ImGui::End();
    }
}