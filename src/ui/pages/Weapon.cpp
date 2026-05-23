#include "Weapon.h"
#include "controllers/Weapon.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "imgui/imgui.h"
#include <cstring>

namespace {
    void WeaponButton(const char* label, unsigned int weaponType) {
        if (ImGui::Button(label)) {
            Controllers::Weapon::Give(weaponType, static_cast<unsigned int>(MenuState::WeaponAmmo));
        }
    }

    void WeaponModelButton(const char* label, unsigned int weaponModel) {
        if (ImGui::Button(label)) {
            Controllers::Weapon::GiveModel(weaponModel, static_cast<unsigned int>(MenuState::WeaponAmmo));
        }
    }

    void SameLineEvery(int index, int columns) {
        if ((index + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }

    void DrawWeaponList() {
        const Resources::WeaponTable table = Resources::GetWeapons();
        const char* currentCategory = nullptr;
        int index = 0;

        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::WeaponEntry& weapon = table.entries[i];
            if (!currentCategory || std::strcmp(currentCategory, weapon.category) != 0) {
                currentCategory = weapon.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(currentCategory);
            }

            if (weapon.isModel) {
                WeaponModelButton(weapon.label, weapon.value);
            } else {
                WeaponButton(weapon.label, weapon.value);
            }
            SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Weapon {
    void Process() {
        Controllers::Weapon::Process();
    }

    void Draw() {
        if (!Controllers::Weapon::HasPlayer()) {
            ImGui::Text((const char*)u8"玩家还没准备好，稍等进档后再用。");
            return;
        }

        if (ImGui::Button((const char*)u8"丢到地上")) {
            Controllers::Weapon::DropWeapon();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"清空武器")) {
            Controllers::Weapon::ClearAll();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"移除当前武器")) {
            Controllers::Weapon::DropCurrent();
        }

        ImGui::Spacing();

        if (ImGui::BeginTabBar("WeaponTabs", ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem((const char*)u8"功能开关")) {
                bool weaponStatsChanged = false;

                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"伤害拉满", &MenuState::HugeWeaponDamage);
                if (ImGui::Checkbox((const char*)u8"快速换弹", &MenuState::FastReload)) {
                    Controllers::Weapon::ResetStats();
                }
#ifdef GTASA
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"无限弹药", &MenuState::InfiniteAmmo);
#else
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"无限弹药", &MenuState::InfiniteAmmo);
#endif
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"超远射程", &MenuState::LongWeaponRange);
#ifdef GTASA
                ImGui::SameLine();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"瞄准时可移动", &MenuState::MoveAim);
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"开火时可移动", &MenuState::MoveFire);
                ImGui::SameLine();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"零散射", &MenuState::NoSpread);
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"连续开火", &MenuState::RapidFire);
                ImGui::SameLine();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"双持", &MenuState::DualWield);
#else
                ImGui::SameLine();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"零散射", &MenuState::NoSpread);
#endif

                if (weaponStatsChanged) {
                    Controllers::Weapon::ResetStats();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"发放武器")) {
                ImGui::PushItemWidth(160);
                ImGui::InputInt((const char*)u8"弹药", &MenuState::WeaponAmmo);
                if (MenuState::WeaponAmmo < 0) MenuState::WeaponAmmo = 0;
                if (MenuState::WeaponAmmo > 99999) MenuState::WeaponAmmo = 99999;
                ImGui::PopItemWidth();

                ImGui::Spacing();
                DrawWeaponList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}