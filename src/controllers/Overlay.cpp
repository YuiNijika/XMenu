#include "Overlay.h"
#include "Player.h"
#include "Vehicle.h"
#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"

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
            ImGui::TextUnformatted(I18n::T("overlay.title"));
            if (MenuState::OverlayShowPosition) {
                const CVector pos = Controllers::Teleport::GetCurrentPosition();
                ImGui::Text(I18n::T("overlay.position"), pos.x, pos.y, pos.z);
            }
            if (MenuState::OverlayShowVehicle) {
                ImGui::Text(I18n::T("overlay.vehicle"), Controllers::Vehicle::GetCurrentVehicle() ? I18n::T("common.yes") : I18n::T("common.no"));
            }
            ImGui::Text(I18n::T("overlay.fps"), ImGui::GetIO().Framerate);
        }
        ImGui::End();
    }
}