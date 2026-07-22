#include "Ped.h"
#include "controllers/Ped.h"
#include "resources/ResourceData.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include <cstdio>
#include <cstring>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    void DrawPedList() {
        const Resources::PedTable table = Resources::GetPeds();
        if (table.count == 0) {
            ImGui::TextWrapped("%s", T("ped.noListData"));
            return;
        }

        std::string currentCategoryKey;
        int index = 0;
        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::PedEntry& ped = table.entries->at(i);

            const std::string categoryKey = "ped.category." + ped.category;
            const char* translatedCategory = I18n::T(categoryKey.c_str());

            if (currentCategoryKey != ped.category) {
                currentCategoryKey = ped.category;
                index = 0;
                ImGui::Spacing();
                ImGui::SeparatorText(translatedCategory);
            }

            char buttonLabel[96];
            std::snprintf(buttonLabel, sizeof(buttonLabel), "%s (%d)", ped.name.c_str(), ped.id);
            if (UI::Button(buttonLabel, 3)) {
                MenuState::PedSpawnModel = ped.id;
                Controllers::Ped::SpawnNearPlayer();
            }
            UI::SameLineEvery(index++, 3);
        }
    }
}

namespace Pages::Ped {
    void Process() {
        Controllers::Ped::Process();
    }

    void Draw() {
        if (UI::BeginTabBar("PedTabs")) {
            if (UI::BeginTab("ped.toggles", T("common.toggles"))) {
                ImGui::Columns(2, nullptr, false);
#if defined(GTASA) || defined(GTA3)
                ImGui::Checkbox(T("ped.bigHeadMode"), &MenuState::BigHeadMode);
                ImGui::NextColumn();
#endif
#ifdef GTASA
                ImGui::Checkbox(T("ped.thinBodyMode"), &MenuState::ThinBodyMode);
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.elvisEverywhere"), &MenuState::ElvisEverywhere)) {
                    Controllers::Ped::SetElvisEverywhere(MenuState::ElvisEverywhere);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.everyoneArmed"), &MenuState::EveryoneArmed)) {
                    Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.pedsMayhem"), &MenuState::PedsMayhem)) {
                    Controllers::Ped::SetPedsMayhem(MenuState::PedsMayhem);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.pedsAtkRocket"), &MenuState::PedsAtkRocket)) {
                    Controllers::Ped::SetPedsAtkRocket(MenuState::PedsAtkRocket);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.pedsRiot"), &MenuState::PedsRiot)) {
                    Controllers::Ped::SetPedsRiot(MenuState::PedsRiot);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.slutMagnet"), &MenuState::SlutMagnet)) {
                    Controllers::Ped::SetSlutMagnet(MenuState::SlutMagnet);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.gangsControl"), &MenuState::GangsControl)) {
                    Controllers::Ped::SetGangsControl(MenuState::GangsControl);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.gangsEverywhere"), &MenuState::GangsEverywhere)) {
                    Controllers::Ped::SetGangsEverywhere(MenuState::GangsEverywhere);
                }
                ImGui::NextColumn();
#endif
#ifdef GTAVC
                if (ImGui::Checkbox(T("ped.everyoneArmed"), &MenuState::EveryoneArmed)) {
                    Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.slutMagnet"), &MenuState::SlutMagnet)) {
                    Controllers::Ped::SetSlutMagnet(MenuState::SlutMagnet);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.noProstitutes"), &MenuState::PedNoProstitutes)) {
                    Controllers::Ped::SetNoProstitutes(MenuState::PedNoProstitutes);
                }
                ImGui::NextColumn();
#endif
#ifdef GTA3
                if (ImGui::Checkbox(T("ped.everyoneArmed"), &MenuState::EveryoneArmed)) {
                    Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.pedsMayhem"), &MenuState::PedsMayhem)) {
                    Controllers::Ped::SetPedsMayhem(MenuState::PedsMayhem);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.pedsRiot"), &MenuState::PedsRiot)) {
                    Controllers::Ped::SetPedsRiot(MenuState::PedsRiot);
                }
                ImGui::NextColumn();
                if (ImGui::Checkbox(T("ped.nastyLimbs"), &MenuState::PedNastyLimbs)) {
                    Controllers::Ped::SetNastyLimbs(MenuState::PedNastyLimbs);
                }
                ImGui::NextColumn();
#endif
                ImGui::Columns(1);
                UI::EndTab();
            }

            if (UI::BeginTab("ped.spawnPed", T("ped.spawnPed"))) {
                ImGui::TextWrapped("%s", T("ped.hint"));
                ImGui::PushItemWidth(160.0f);
                ImGui::InputInt(T("ped.modelId"), &MenuState::PedSpawnModel);
                ImGui::InputInt(T("ped.type"), &MenuState::PedSpawnType);
                ImGui::InputInt(T("ped.gangType"), &MenuState::PedGangType);
                ImGui::InputInt(T("ped.weaponModel"), &MenuState::PedWeaponModel);
                ImGui::InputFloat(T("ped.health"), &MenuState::PedHealth, 1.0f, 10.0f, "%.1f");
                ImGui::InputFloat(T("ped.armour"), &MenuState::PedArmour, 1.0f, 10.0f, "%.1f");
                ImGui::PopItemWidth();

                ImGui::Checkbox(T("ped.asGang"), &MenuState::PedSpawnAsGang);
                ImGui::SameLine();
                ImGui::Checkbox(T("ped.freeze"), &MenuState::PedFreeze);
                ImGui::SameLine();
                ImGui::Checkbox(T("ped.hostile"), &MenuState::PedHostile);

                ImGui::Checkbox(T("ped.smoking"), &MenuState::SmokingEffect);
                ImGui::SameLine();
                ImGui::Checkbox(T("ped.flies"), &MenuState::FliesEffect);

                UI::SpacingSeparator();
                if (UI::Button(T("ped.spawnNear"), 3)) {
                    Controllers::Ped::SpawnNearPlayer();
                }
                ImGui::SameLine();
                if (UI::Button(T("ped.spawnMarker"), 3)) {
                    Controllers::Ped::SpawnAtMarker();
                }
                ImGui::SameLine();
                if (UI::Button(T("ped.deleteLast"), 3)) {
                    Controllers::Ped::DeleteLastSpawnedPed();
                }

                ImGui::TextDisabled("%s", Controllers::Ped::GetLastSpawnedPed() ? T("ped.lastSpawnedYes") : T("ped.lastSpawnedNo"));

                UI::SpacingSeparator();
                ImGui::TextWrapped("%s", T("ped.listHint"));
                UI::SpacingSeparator();
                DrawPedList();
                UI::EndTab();
            }

#ifdef GTASA
            if (UI::BeginTab("ped.gangs", T("ped.gangs"))) {
                if (ImGui::Checkbox(T("ped.gangWarsActive"), &MenuState::GangWarsActive)) {
                    Controllers::Ped::SetGangWarsActive(MenuState::GangWarsActive);
                }
                if (UI::Button(T("ped.startGangWar"), 3)) {
                    Controllers::Ped::StartGangWar(true);
                }
                ImGui::SameLine();
                if (UI::Button(T("ped.endGangWar"), 3)) {
                    Controllers::Ped::EndGangWar();
                }
                ImGui::SameLine();
                if (UI::Button(T("ped.resetGangModels"), 3)) {
                    Controllers::Ped::ResetGangModels();
                }

                UI::PushItemWidth(160);
                UI::SliderInt(T("ped.gangSelected"), &MenuState::GangSelected, 0, 9);
                const int density = Controllers::Ped::GetGangZoneDensity(MenuState::GangSelected);
                int densityEdit = density;
                if (UI::SliderInt(T("ped.gangDensity"), &densityEdit, 0, 127)) {
                    Controllers::Ped::SetGangZoneDensity(MenuState::GangSelected, densityEdit);
                }

                UI::SliderInt(T("ped.gangMemberIndex"), &MenuState::GangMemberIndex, 0, 2);
                int memberModel = static_cast<int>(Controllers::Ped::GetGangMemberModel(
                    static_cast<unsigned int>(MenuState::GangSelected),
                    static_cast<unsigned int>(MenuState::GangMemberIndex)));
                if (UI::InputInt(T("ped.gangMemberModel"), &memberModel)) {
                    if (memberModel < 0) memberModel = 0;
                    Controllers::Ped::SetGangMemberModel(
                        static_cast<unsigned int>(MenuState::GangSelected),
                        static_cast<unsigned int>(MenuState::GangMemberIndex),
                        static_cast<unsigned int>(memberModel));
                }

                UI::InputInt(T("ped.gangWeaponType"), &MenuState::GangWeaponType);
                UI::PopItemWidth();
                if (UI::Button(T("ped.applyGangWeapons"), 2)) {
                    // 简化：三个槽位用同一武器类型，足够日常调参
                    Controllers::Ped::SetGangWeapons(
                        static_cast<unsigned int>(MenuState::GangSelected),
                        MenuState::GangWeaponType,
                        MenuState::GangWeaponType,
                        MenuState::GangWeaponType);
                }
                UI::EndTab();
            }
#endif
            UI::EndTabBar();
        }
    }
}