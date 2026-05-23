#include "World.h"
#include "controllers/World.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
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

            if (UI::Button((const char*)u8"同步现实时间")) {
                Controllers::World::SyncTimeWithSystemClock();
            }
        }

        if (ImGui::CollapsingHeader((const char*)u8"天气", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox((const char*)u8"锁住当前天气", &MenuState::LockWeather)) {
                Controllers::World::CaptureWeather();
            }
            Controllers::World::DrawWeatherButtons();
        }

        if (ImGui::CollapsingHeader((const char*)u8"游戏规则", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Columns(2, nullptr, false);
            if (ImGui::Checkbox((const char*)u8"禁用回放", &MenuState::DisableReplay)) {
                Controllers::World::SetDisableReplay(MenuState::DisableReplay);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox((const char*)u8"禁用作弊码", &MenuState::DisableCheats)) {
                Controllers::World::SetDisableCheats(MenuState::DisableCheats);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox((const char*)u8"加快时钟", &MenuState::FasterClock)) {
                Controllers::World::SetFasterClock(MenuState::FasterClock);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox((const char*)u8"冻结时间", &MenuState::FreezeTime)) {
                Controllers::World::SetFreezeTime(MenuState::FreezeTime);
            }
#ifdef GTASA
            ImGui::NextColumn();
            if (ImGui::Checkbox((const char*)u8"禁止通缉区域", &MenuState::ForbiddenAreaWanted)) {
                Controllers::World::SetForbiddenAreaWanted(MenuState::ForbiddenAreaWanted);
            }
            ImGui::NextColumn();
            if (ImGui::Checkbox((const char*)u8"免费喷漆店", &MenuState::FreePayNSpray)) {
                Controllers::World::SetFreePayNSpray(MenuState::FreePayNSpray);
            }
#endif
            ImGui::Columns(1);
            ImGui::Spacing();            int days = Controllers::World::GetDaysPassed();
            ImGui::PushItemWidth(150);
            if (ImGui::InputInt((const char*)u8"经过天数", &days)) {
                if (days < 0) days = 0;
                if (days > 9999) days = 9999;
                Controllers::World::SetDaysPassed(days);
            }

            float gravity = Controllers::World::GetGravity();
            if (ImGui::SliderFloat((const char*)u8"重力", &gravity, -1.0f, 1.0f, "%.3f")) {
                Controllers::World::SetGravity(gravity);
            }

            int fpsLimit = Controllers::World::GetFpsLimit();
            if (ImGui::InputInt((const char*)u8"FPS 限制", &fpsLimit)) {
                if (fpsLimit < 1) fpsLimit = 1;
                if (fpsLimit > 999) fpsLimit = 999;
                Controllers::World::SetFpsLimit(fpsLimit);
            }
            ImGui::PopItemWidth();
        }

        if (ImGui::CollapsingHeader((const char*)u8"游戏节奏", ImGuiTreeNodeFlags_DefaultOpen)) {
            float speed = Controllers::World::GetGameSpeed();
            if (ImGui::SliderFloat((const char*)u8"倍率", &speed, 0.1f, 5.0f, "%.1fx")) {
                Controllers::World::SetGameSpeed(speed);
            }
            if (UI::Button((const char*)u8"恢复原速")) {
                Controllers::World::SetGameSpeed(1.0f);
            }
        }
    }
}