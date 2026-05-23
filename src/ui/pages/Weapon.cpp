#include "Weapon.h"
#include "controllers/Weapon.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "imgui/imgui.h"
#include <cstring>

namespace {
    void WeaponButton(const char* label, unsigned int weaponType) {
        if (UI::Button(label, 3)) {
            Controllers::Weapon::Give(weaponType, static_cast<unsigned int>(MenuState::WeaponAmmo));
        }
    }

    void WeaponModelButton(const char* label, unsigned int weaponModel) {
        if (UI::Button(label, 3)) {
            Controllers::Weapon::GiveModel(weaponModel, static_cast<unsigned int>(MenuState::WeaponAmmo));
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
            UI::SameLineEvery(index++, 3);
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

        if (UI::Button((const char*)u8"获取所有武器", 4)) {
            Controllers::Weapon::GiveAll();
        }
        ImGui::SameLine();
        if (UI::Button((const char*)u8"丢出当前武器", 4)) {
            Controllers::Weapon::DropWeapon();
        }
        ImGui::SameLine();
        if (UI::Button((const char*)u8"清空武器", 4)) {
            Controllers::Weapon::ClearAll();
        }
        ImGui::SameLine();
        if (UI::Button((const char*)u8"移除当前武器", 4)) {
            Controllers::Weapon::DropCurrent();
        }

        ImGui::Spacing();

        if (UI::BeginTabBar("WeaponTabs")) {
            if (ImGui::BeginTabItem((const char*)u8"功能开关")) {
                bool weaponStatsChanged = false;

                ImGui::Columns(3, nullptr, false);
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"高伤害", &MenuState::HugeWeaponDamage);
                ImGui::NextColumn();
                if (ImGui::Checkbox((const char*)u8"快速换弹", &MenuState::FastReload)) {
                    Controllers::Weapon::ResetStats();
                }
                ImGui::NextColumn();
                ImGui::Checkbox((const char*)u8"无限弹药", &MenuState::InfiniteAmmo);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"远射程", &MenuState::LongWeaponRange);
#ifdef GTASA
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"瞄准时可移动", &MenuState::MoveAim);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"开火时可移动", &MenuState::MoveFire);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"零散射", &MenuState::NoSpread);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"快速连射", &MenuState::RapidFire);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"双持", &MenuState::DualWield);
#else
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox((const char*)u8"零散射", &MenuState::NoSpread);
#endif
                ImGui::Columns(1);

                if (weaponStatsChanged) {
                    Controllers::Weapon::ResetStats();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"获取武器")) {
                ImGui::PushItemWidth(160);
                ImGui::InputInt((const char*)u8"弹药", &MenuState::WeaponAmmo);
                if (MenuState::WeaponAmmo < 0) MenuState::WeaponAmmo = 0;
                if (MenuState::WeaponAmmo > 99999) MenuState::WeaponAmmo = 99999;
                ImGui::PopItemWidth();

                ImGui::PushItemWidth(160);
#ifdef GTASA
                ImGui::InputInt((const char*)u8"武器类型 ID", &MenuState::WeaponSpawnId);
#else
                ImGui::InputInt((const char*)u8"武器模型 ID", &MenuState::WeaponSpawnId);
#endif
                if (MenuState::WeaponSpawnId < 0) MenuState::WeaponSpawnId = 0;
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (UI::Button((const char*)u8"按 ID 获取", 2)) {
#ifdef GTASA
                    Controllers::Weapon::Give(static_cast<unsigned int>(MenuState::WeaponSpawnId), static_cast<unsigned int>(MenuState::WeaponAmmo));
#else
                    Controllers::Weapon::GiveModel(static_cast<unsigned int>(MenuState::WeaponSpawnId), static_cast<unsigned int>(MenuState::WeaponAmmo));
#endif
                }

                ImGui::Spacing();
                DrawWeaponList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}