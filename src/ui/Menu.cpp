#include "Menu.h"
#include <windows.h>
#include <cstdio>
#include "utils/Log.h"
#include "utils/D3DHook.h"
#include "imgui/imgui.h"
#include "ui/pages/Player.h"
#include "ui/pages/Vehicle.h"
#include "ui/pages/Teleport.h"
#include "ui/pages/Weapon.h"
#include "ui/pages/World.h"

extern const char* XMENU_VERSION;
extern const char* XMENU_AUTHOR;
extern const char* XMENU_GITHUB;
extern const char* XMENU_QQ_GROUP;

void Menu::Process() {
    Pages::Player::Process();
    Pages::Vehicle::Process();
    Pages::Weapon::Process();
    Pages::World::Process();
}

void Menu::Draw() {
    bool menuVisible = true;
    char windowTitle[160] = {};

    std::snprintf(windowTitle, sizeof(windowTitle), "XMenu %s 作者：%s [www.gtamodx.com]", XMENU_VERSION, XMENU_AUTHOR);
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowTitle, &menuVisible, ImGuiWindowFlags_NoCollapse)) {
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
            if (ImGui::BeginTabItem((const char*)u8"菜单")) {
                ImGui::Spacing();
                if (ImGui::Button((const char*)u8"关闭菜单")) {
                    Log::Info("用户从菜单页关闭菜单");
                    D3DHook::SetMenuVisible(false);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem((const char*)u8"关于")) {
                ImGui::TextWrapped((const char*)u8"XMenu %s", XMENU_VERSION);
                ImGui::TextWrapped((const char*)u8"作者：%s", XMENU_AUTHOR);
                ImGui::Spacing();
                if (ImGui::Button((const char*)u8"加群")) {
                    ShellExecuteA(nullptr, "open", XMENU_QQ_GROUP, nullptr, nullptr, SW_SHOWNORMAL);
                }
                if (ImGui::Button((const char*)u8"项目主页")) {
                    ShellExecuteA(nullptr, "open", XMENU_GITHUB, nullptr, nullptr, SW_SHOWNORMAL);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}