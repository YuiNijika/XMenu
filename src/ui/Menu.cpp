#include "Menu.h"
#include "imgui/imgui.h"
#include "ui/pages/Player.h"
#include "ui/pages/Vehicle.h"
#include "ui/pages/Teleport.h"
#include "ui/pages/Weapon.h"
#include "ui/pages/World.h"

void Menu::Process() {
    Pages::Player::Process();
    Pages::Vehicle::Process();
    Pages::Weapon::Process();
    Pages::World::Process();
}

void Menu::Draw() {
    bool menuVisible = true;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin((const char*)u8"XMenu 极简中文菜单", &menuVisible, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("XMenuTabs")) {
            if (ImGui::BeginTabItem((const char*)u8"玩家")) {
                Pages::Player::Draw();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem((const char*)u8"载具")) {
                Pages::Vehicle::Draw();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem((const char*)u8"传送")) {
                Pages::Teleport::Draw();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem((const char*)u8"武器")) {
                Pages::Weapon::Draw();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem((const char*)u8"世界")) {
                Pages::World::Draw();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}