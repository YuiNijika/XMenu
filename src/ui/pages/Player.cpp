#include "Player.h"
#include "controllers/Player.h"
#include "ui/MenuState.h"
#include "imgui/imgui.h"

namespace Pages::Player {
    void Process() {
        Controllers::Player::Process();
    }

    void Draw() {
        if (ImGui::Button((const char*)u8"回满血量")) {
            Controllers::Player::Heal();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"补满护甲")) {
            Controllers::Player::GiveArmour();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"加 25 万现金")) {
            Controllers::Player::GiveMoney();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"立即倒地")) {
            Controllers::Player::Kill();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox((const char*)u8"无敌", &MenuState::GodMode);
        ImGui::SameLine();
        ImGui::Checkbox((const char*)u8"自动回血", &MenuState::AutoHeal);
        ImGui::SameLine();
        ImGui::Checkbox((const char*)u8"低血挑战", &MenuState::HardMode);

        MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
        ImGui::PushItemWidth(150);
        if (ImGui::SliderInt((const char*)u8"通缉星级", &MenuState::WantedLevel, 0, 6)) {
            Controllers::Player::SetWantedLevel(MenuState::WantedLevel);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"消除通缉")) {
            MenuState::WantedLevel = 0;
            Controllers::Player::ClearWantedLevel();
        }

        if (ImGui::Checkbox((const char*)u8"无限冲刺", &MenuState::InfiniteSprint)) {
            Controllers::Player::SetInfiniteSprint(MenuState::InfiniteSprint);
        }

        bool freeHealth = Controllers::Player::GetFreeHealthcare();
        if (ImGui::Checkbox((const char*)u8"住院不扣钱", &freeHealth)) {
            Controllers::Player::SetFreeHealthcare(freeHealth);
        }

        bool freeJail = Controllers::Player::GetFreeJail();
        if (ImGui::Checkbox((const char*)u8"进局子不扣钱", &freeJail)) {
            Controllers::Player::SetFreeJail(freeJail);
        }
    }
}