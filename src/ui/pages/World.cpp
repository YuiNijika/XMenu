#include "World.h"
#include "controllers/World.h"
#include "ui/MenuState.h"
#include "imgui/imgui.h"

namespace Pages::World {
    void Process() {
        Controllers::World::Process();
    }

    void Draw() {
        if (ImGui::CollapsingHeader((const char*)u8"时间", ImGuiTreeNodeFlags_DefaultOpen)) {
            int hour = 0;
            int minute = 0;
            Controllers::World::GetTime(hour, minute);

            ImGui::PushItemWidth(150);
            bool timeChanged = false;
            if (ImGui::SliderInt((const char*)u8"小时", &hour, 0, 23)) {
                timeChanged = true;
            }
            if (ImGui::SliderInt((const char*)u8"分钟", &minute, 0, 59)) {
                timeChanged = true;
            }
            ImGui::PopItemWidth();

            if (timeChanged) {
                Controllers::World::SetTime(hour, minute);
            }

            if (ImGui::Button((const char*)u8"同步现实时间")) {
                Controllers::World::SyncTimeWithSystemClock();
            }
        }

        if (ImGui::CollapsingHeader((const char*)u8"天气", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox((const char*)u8"锁住当前天气", &MenuState::LockWeather)) {
                Controllers::World::CaptureWeather();
            }
            Controllers::World::DrawWeatherButtons();
        }

        if (ImGui::CollapsingHeader((const char*)u8"游戏节奏", ImGuiTreeNodeFlags_DefaultOpen)) {
            float speed = Controllers::World::GetGameSpeed();
            if (ImGui::SliderFloat((const char*)u8"倍率", &speed, 0.1f, 5.0f, "%.1fx")) {
                Controllers::World::SetGameSpeed(speed);
            }
            if (ImGui::Button((const char*)u8"恢复原速")) {
                Controllers::World::SetGameSpeed(1.0f);
            }
        }
    }
}