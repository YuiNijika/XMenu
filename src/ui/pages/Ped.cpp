#include "Ped.h"
#include "controllers/Ped.h"
#include "resources/ResourceData.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include <XBase/UI.h>
#include <cstdio>
#include <cstring>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    bool Checkbox(const char* label, bool* value) {
        return value && XBase::UI::Checkbox(label, *value);
    }

    bool InputInt(const char* label, int* value) {
        return value && XBase::UI::Input(label, *value);
    }

    bool InputFloat(const char* label, float* value, float step, float fastStep, const char* format) {
        return value && XBase::UI::Input(label, *value, step, fastStep, format);
    }

    void DrawPedList() {
        const Resources::PedTable table = Resources::GetPeds();
        if (table.count == 0) {
            XBase::UI::TextWrapped(T("ped.noListData"));
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
                XBase::UI::Spacing();
                XBase::UI::SeparatorText(translatedCategory);
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
    void Draw() {
        XBase::UI::Tabs("PedTabs", [&] {
            XBase::UI::Tab("ped.toggles", T("common.toggles"), [&] {
                XBase::UI::Columns(2, nullptr, false);
#if defined(GTASA) || defined(GTA3)
                Checkbox(T("ped.bigHeadMode"), &MenuState::BigHeadMode);
                XBase::UI::NextColumn();
#endif
#ifdef GTASA
                Checkbox(T("ped.thinBodyMode"), &MenuState::ThinBodyMode);
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.elvisEverywhere"), &MenuState::ElvisEverywhere)) {
                    Controllers::Ped::SetElvisEverywhere(MenuState::ElvisEverywhere);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.everyoneArmed"), &MenuState::EveryoneArmed)) {
                    Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.pedsMayhem"), &MenuState::PedsMayhem)) {
                    Controllers::Ped::SetPedsMayhem(MenuState::PedsMayhem);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.pedsAtkRocket"), &MenuState::PedsAtkRocket)) {
                    Controllers::Ped::SetPedsAtkRocket(MenuState::PedsAtkRocket);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.pedsRiot"), &MenuState::PedsRiot)) {
                    Controllers::Ped::SetPedsRiot(MenuState::PedsRiot);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.slutMagnet"), &MenuState::SlutMagnet)) {
                    Controllers::Ped::SetSlutMagnet(MenuState::SlutMagnet);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.gangsControl"), &MenuState::GangsControl)) {
                    Controllers::Ped::SetGangsControl(MenuState::GangsControl);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.gangsEverywhere"), &MenuState::GangsEverywhere)) {
                    Controllers::Ped::SetGangsEverywhere(MenuState::GangsEverywhere);
                }
                XBase::UI::NextColumn();
#endif
#ifdef GTAVC
                if (Checkbox(T("ped.everyoneArmed"), &MenuState::EveryoneArmed)) {
                    Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.slutMagnet"), &MenuState::SlutMagnet)) {
                    Controllers::Ped::SetSlutMagnet(MenuState::SlutMagnet);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.noProstitutes"), &MenuState::PedNoProstitutes)) {
                    Controllers::Ped::SetNoProstitutes(MenuState::PedNoProstitutes);
                }
                XBase::UI::NextColumn();
#endif
#ifdef GTA3
                if (Checkbox(T("ped.everyoneArmed"), &MenuState::EveryoneArmed)) {
                    Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.pedsMayhem"), &MenuState::PedsMayhem)) {
                    Controllers::Ped::SetPedsMayhem(MenuState::PedsMayhem);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.pedsRiot"), &MenuState::PedsRiot)) {
                    Controllers::Ped::SetPedsRiot(MenuState::PedsRiot);
                }
                XBase::UI::NextColumn();
                if (Checkbox(T("ped.nastyLimbs"), &MenuState::PedNastyLimbs)) {
                    Controllers::Ped::SetNastyLimbs(MenuState::PedNastyLimbs);
                }
                XBase::UI::NextColumn();
#endif
                XBase::UI::Columns(1);

                // Peds No Fire
                XBase::UI::SeparatorText(T("ped.pedsNoFire"));
                if (Checkbox(T("ped.pedsNoFire"), &MenuState::PedsNoFire)) {
                    Controllers::Ped::SetPedsNoFire(MenuState::PedsNoFire);
                }
                if (MenuState::PedsNoFire) {
                    XBase::UI::Indented([&] {
                        Checkbox(T("ped.pedsNoFireCivilians"), &MenuState::PedsNoFireCivilians);
                        Checkbox(T("ped.pedsNoFireGangs"), &MenuState::PedsNoFireGangs);
                        Checkbox(T("ped.pedsNoFirePolice"), &MenuState::PedsNoFirePolice);
                        Checkbox(T("ped.pedsNoFireMission"), &MenuState::PedsNoFireMission);
                    });
                }

                });

            XBase::UI::Tab("ped.spawnPed", T("ped.spawnPed"), [&] {
                XBase::UI::TextWrapped(T("ped.hint"));
                XBase::UI::PushItemWidth(160.0f);
                InputInt(T("ped.modelId"), &MenuState::PedSpawnModel);
                InputInt(T("ped.type"), &MenuState::PedSpawnType);
                InputInt(T("ped.gangType"), &MenuState::PedGangType);
                InputInt(T("ped.weaponModel"), &MenuState::PedWeaponModel);
                InputFloat(T("ped.health"), &MenuState::PedHealth, 1.0f, 10.0f, "%.1f");
                InputFloat(T("ped.armour"), &MenuState::PedArmour, 1.0f, 10.0f, "%.1f");
                XBase::UI::PopItemWidth();

                Checkbox(T("ped.asGang"), &MenuState::PedSpawnAsGang);
                XBase::UI::SameLine();
                Checkbox(T("ped.freeze"), &MenuState::PedFreeze);
                XBase::UI::SameLine();
                Checkbox(T("ped.hostile"), &MenuState::PedHostile);

                Checkbox(T("ped.smoking"), &MenuState::SmokingEffect);
                XBase::UI::SameLine();
                Checkbox(T("ped.flies"), &MenuState::FliesEffect);

                UI::SpacingSeparator();
                if (UI::Button(T("ped.spawnNear"), 3)) {
                    Controllers::Ped::SpawnNearPlayer();
                }
                XBase::UI::SameLine();
                if (UI::Button(T("ped.spawnMarker"), 3)) {
                    Controllers::Ped::SpawnAtMarker();
                }
                XBase::UI::SameLine();
                if (UI::Button(T("ped.deleteLast"), 3)) {
                    Controllers::Ped::DeleteLastSpawnedPed();
                }

                XBase::UI::TextDisabled(Controllers::Ped::GetLastSpawnedId() ? T("ped.lastSpawnedYes") : T("ped.lastSpawnedNo"));

                UI::SpacingSeparator();
                XBase::UI::TextWrapped(T("ped.listHint"));
                UI::SpacingSeparator();
                DrawPedList();
                });

#ifdef GTASA
            XBase::UI::Tab("ped.gangs", T("ped.gangs"), [&] {
                if (Checkbox(T("ped.gangWarsActive"), &MenuState::GangWarsActive)) {
                    Controllers::Ped::SetGangWarsActive(MenuState::GangWarsActive);
                }
                if (UI::Button(T("ped.startGangWar"), 3)) {
                    Controllers::Ped::StartGangWar(true);
                }
                XBase::UI::SameLine();
                if (UI::Button(T("ped.endGangWar"), 3)) {
                    Controllers::Ped::EndGangWar();
                }
                XBase::UI::SameLine();
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
                });
#endif
            });
    }
}