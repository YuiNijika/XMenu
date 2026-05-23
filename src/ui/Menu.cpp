#include "Menu.h"
#include <windows.h>
#include <cstdio>
#include "utils/Log.h"
#include "utils/D3DHook.h"
#include "ui/Widget.h"
#include "imgui/imgui.h"
#include "ui/pages/Player.h"
#include "ui/pages/Vehicle.h"
#include "ui/pages/Teleport.h"
#include "ui/pages/Weapon.h"
#include "ui/pages/World.h"

extern const char* XMENU_VERSION;
extern const char* XMENU_AUTHOR;
extern const char* XMENU_AUTHOR_TEST;
extern const char* XMENU_GITHUB;
extern const char* XMENU_QQ_GROUP;
extern const char* XMENU_TECH_STACK;
extern const char* XMENU_OPEN_SOURCE_LIBS;

void Menu::Process() {
    Pages::Player::Process();
    Pages::Vehicle::Process();
    Pages::Weapon::Process();
    Pages::World::Process();
}

void Menu::Draw() {
    bool menuVisible = D3DHook::IsMenuVisible();
    char windowTitle[160] = {};

    std::snprintf(windowTitle, sizeof(windowTitle), "XMenu %s 作者：%s - GTAMODX", XMENU_VERSION, XMENU_AUTHOR);
    ImGui::SetNextWindowSize(ImVec2(650, 480), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowTitle, &menuVisible, ImGuiWindowFlags_NoCollapse)) {
        if (UI::BeginTabBar("XMenuTabs")) {
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
            if (ImGui::BeginTabItem((const char*)u8"关于")) {
                UI::TextCentered((const char*)u8"XMenu");
                ImGui::TextWrapped((const char*)u8"版本：%s", XMENU_VERSION);
                ImGui::TextWrapped((const char*)u8"作者：%s", XMENU_AUTHOR);
                ImGui::TextWrapped((const char*)u8"测试：%s", XMENU_AUTHOR_TEST);
                ImGui::TextWrapped((const char*)u8"技术栈：%s", XMENU_TECH_STACK);
                ImGui::TextWrapped((const char*)u8"开源库：%s", XMENU_OPEN_SOURCE_LIBS);
                UI::SpacingSeparator();
                ImGui::TextWrapped((const char*)u8"1. 永久免费，禁止倒卖，禁止用于商业用途。");
                ImGui::TextWrapped((const char*)u8"2. 遇到问题可以加群或前往项目主页反馈。");
                ImGui::Spacing();
                if (UI::Button((const char*)u8"加群", 2)) {
                    ShellExecuteA(nullptr, "open", XMENU_QQ_GROUP, nullptr, nullptr, SW_SHOWNORMAL);
                }
                ImGui::SameLine();
                if (UI::Button((const char*)u8"项目主页", 2)) {
                    ShellExecuteA(nullptr, "open", XMENU_GITHUB, nullptr, nullptr, SW_SHOWNORMAL);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    if (!menuVisible) {
        D3DHook::SetMenuVisible(false);
    }
}