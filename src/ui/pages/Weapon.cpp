#include "Weapon.h"
#include "controllers/Weapon.h"
#include "game/Runtime.h"
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include "utils/D3DHook.h"
#include "imgui/imgui.h"
#include <cstring>
#include <vector>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

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
        std::string currentCategoryKey;
        int index = 0;

        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::WeaponEntry& weapon = table.entries->at(i);
            
            // 翻译分类名称
            std::string categoryKey = "weapon.category." + weapon.category;
            const char* translatedCategory = I18n::T(categoryKey.c_str());
            
            if (!currentCategory || currentCategoryKey != weapon.category) {
                currentCategory = translatedCategory;
                currentCategoryKey = weapon.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(translatedCategory);
            }

            // 翻译武器名称
            const char* translatedName = I18n::T(weapon.name.c_str());

            if (weapon.isModel) {
                WeaponModelButton(translatedName, static_cast<unsigned int>(weapon.modelId));
            } else {
                WeaponButton(translatedName, static_cast<unsigned int>(weapon.id));
            }
            UI::SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Weapon {
    void Process() {
        Controllers::Weapon::Process();

        // Weapon cycler auto-give when menu is closed
        if (!MenuState::WeaponCyclerEnabled || D3DHook::IsMenuVisible()) {
            return;
        }

        // Cache weapon table
        static std::vector<Resources::WeaponEntry> s_cyclerWeapons;
        if (s_cyclerWeapons.empty()) {
            const Resources::WeaponTable table = Resources::GetWeapons();
            for (std::size_t i = 0; i < table.count; ++i) {
                const auto& w = table.entries->at(i);
                if (w.isModel || w.id > 0) {
                    s_cyclerWeapons.push_back(w);
                }
            }
        }
        if (s_cyclerWeapons.empty()) return;

        const float wheel = D3DHook::ConsumeRawWheelDelta();
        if (wheel > -0.1f && wheel < 0.1f) return;

        const int maxIdx = static_cast<int>(s_cyclerWeapons.size()) - 1;
        int& cyclerIdx = MenuState::WeaponCyclerId;
        if (cyclerIdx < 0 || cyclerIdx > maxIdx) cyclerIdx = 0;

        const unsigned int ammo = static_cast<unsigned int>(MenuState::WeaponAmmo);
        const int direction = (wheel > 0.1f) ? 1 : -1;
        float remaining = (wheel > 0) ? wheel : -wheel;
        while (remaining > 0.1f) {
            const auto& entry = s_cyclerWeapons[cyclerIdx];

            if (entry.isModel) {
                Controllers::Weapon::GiveModelSilent(static_cast<unsigned int>(entry.modelId), ammo);
            } else {
                Controllers::Weapon::GiveSilent(static_cast<unsigned int>(entry.id), ammo);
            }

            cyclerIdx += direction;
            if (cyclerIdx > maxIdx) cyclerIdx = 0;
            if (cyclerIdx < 0) cyclerIdx = maxIdx;
            remaining -= 1.0f;
        }
    }

    void Draw() {
        if (!Controllers::Weapon::HasPlayer()) {
            ImGui::Text("%s", T("weapon.playerNotReady"));
            return;
        }

        if (UI::Button(T("weapon.dropWeapon"), 4)) {
            Controllers::Weapon::DropWeapon();
        }
        ImGui::SameLine();
        if (UI::Button(T("weapon.clearWeapons"), 4)) {
            Controllers::Weapon::ClearAll();
        }
        ImGui::SameLine();
        if (UI::Button(T("weapon.removePickups"), 4)) {
            Controllers::Weapon::RemovePickups();
        }

        ImGui::Spacing();

        if (UI::BeginTabBar("WeaponTabs")) {
            if (ImGui::BeginTabItem(T("common.toggles"))) {
                bool weaponStatsChanged = false;

                ImGui::Columns(3, nullptr, false);
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.highDamage"), &MenuState::HugeWeaponDamage);
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("weapon.fastReload"), &MenuState::FastReload)) {
                    Controllers::Weapon::ResetStats();
                }
                ImGui::NextColumn();
                ImGui::Checkbox(T("weapon.infiniteAmmo"), &MenuState::InfiniteAmmo);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.longRange"), &MenuState::LongWeaponRange);
#ifdef GTASA
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.moveWhileAiming"), &MenuState::MoveAim);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.moveWhileFiring"), &MenuState::MoveFire);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.noSpread"), &MenuState::NoSpread);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.rapidFire"), &MenuState::RapidFire);
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.dualWield"), &MenuState::DualWield);
#else
                ImGui::NextColumn();
                weaponStatsChanged |= ImGui::Checkbox(T("weapon.noSpread"), &MenuState::NoSpread);
#endif
                ImGui::Columns(1);

                if (weaponStatsChanged) {
                    Controllers::Weapon::ResetStats();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("weapon.getWeapon"))) {
                ImGui::PushItemWidth(160);
                ImGui::InputInt(T("weapon.ammo"), &MenuState::WeaponAmmo);
                if (MenuState::WeaponAmmo < 0) MenuState::WeaponAmmo = 0;
                if (MenuState::WeaponAmmo > 99999) MenuState::WeaponAmmo = 99999;
                ImGui::PopItemWidth();

                ImGui::PushItemWidth(160);
#ifdef GTASA
                ImGui::InputInt(T("weapon.typeId"), &MenuState::WeaponSpawnId);
#else
                ImGui::InputInt(T("weapon.modelId"), &MenuState::WeaponSpawnId);
#endif
                if (MenuState::WeaponSpawnId < 0) MenuState::WeaponSpawnId = 0;
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (UI::Button(T("weapon.getById"), 2)) {
#ifdef GTASA
                    Controllers::Weapon::Give(static_cast<unsigned int>(MenuState::WeaponSpawnId), static_cast<unsigned int>(MenuState::WeaponAmmo));
#else
                    Controllers::Weapon::GiveModel(static_cast<unsigned int>(MenuState::WeaponSpawnId), static_cast<unsigned int>(MenuState::WeaponAmmo));
#endif
                }

                ImGui::Spacing();

                if (GameRuntime::Current().target == GameRuntime::Target::III) {
                    if (UI::Button(T("weapon.getAll"), 4)) {
                        Controllers::Weapon::GiveAll();
                    }
                } else {
                    ImGui::Checkbox(T("weapon.cyclerEnable"), &MenuState::WeaponCyclerEnabled);
                }

                ImGui::Checkbox(T("weapon.safeMode"), &MenuState::WeaponSafeMode);

                if (GameRuntime::Current().target != GameRuntime::Target::III) {
                    ImGui::SeparatorText(T("weapon.cyclerTitle"));

                    ImGui::PushItemWidth(160);
                    ImGui::InputInt("##cyclerInputId", &MenuState::WeaponCyclerInputId);
                    if (MenuState::WeaponCyclerInputId < 0) MenuState::WeaponCyclerInputId = 0;
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    if (UI::Button(T("weapon.cyclerGive"), 2)) {
                        const unsigned int ammo = static_cast<unsigned int>(MenuState::WeaponAmmo);
#ifdef GTASA
                        Controllers::Weapon::Give(static_cast<unsigned int>(MenuState::WeaponCyclerInputId), ammo);
#else
                        Controllers::Weapon::GiveModel(static_cast<unsigned int>(MenuState::WeaponCyclerInputId), ammo);
#endif
                    }

                    if (MenuState::WeaponCyclerEnabled) {
                        float wheel = ImGui::GetIO().MouseWheel;
                        if (wheel > 0.1f || wheel < -0.1f) {
                            const unsigned int ammo = static_cast<unsigned int>(MenuState::WeaponAmmo);
                            const unsigned int spawnId = static_cast<unsigned int>(MenuState::WeaponCyclerInputId);
                            const int steps = static_cast<int>((wheel > 0 ? wheel : -wheel) + 0.5f);
                            for (int i = 0; i < steps; ++i) {
#ifdef GTASA
                                Controllers::Weapon::GiveSilent(spawnId, ammo);
#else
                                Controllers::Weapon::GiveModelSilent(spawnId, ammo);
#endif
                            }
                            ImGui::GetIO().MouseWheel = 0.0f;
                        }
                    }
                }

                ImGui::Spacing();
                DrawWeaponList();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}