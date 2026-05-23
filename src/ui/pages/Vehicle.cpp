#include "Vehicle.h"
#include "controllers/Vehicle.h"
#include "ui/MenuState.h"
#include "imgui/imgui.h"

namespace Pages::Vehicle {
    void Process() {
        Controllers::Vehicle::Process();
    }

    void Draw() {
        if (!Controllers::Vehicle::GetCurrentVehicle()) {
            ImGui::Text((const char*)u8"先上车，这页功能才会生效。");
            return;
        }

        if (ImGui::Button((const char*)u8"一键修车")) {
            Controllers::Vehicle::Repair();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"刹停载具")) {
            Controllers::Vehicle::Stop();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"点火")) {
            Controllers::Vehicle::SetEngine(true);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"熄火")) {
            Controllers::Vehicle::SetEngine(false);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox((const char*)u8"载具防炸防撞", &MenuState::VehicleNoDamage);
        ImGui::SameLine();
        ImGui::Checkbox((const char*)u8"车身加重", &MenuState::VehicleHeavy);
        ImGui::SameLine();
        ImGui::Checkbox((const char*)u8"落水不熄火", &MenuState::VehicleWatertight);

        ImGui::Checkbox((const char*)u8"锁住当前车速", &MenuState::VehicleSpeedLock);
        ImGui::PushItemWidth(150);
        ImGui::SliderFloat((const char*)u8"目标速度", &MenuState::VehicleSpeed, 5.0f, 300.0f, "%.0f");
        ImGui::PopItemWidth();
    }
}