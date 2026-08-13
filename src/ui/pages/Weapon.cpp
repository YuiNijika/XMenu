#include "Weapon.h"
#include "controllers/Weapon.h"
#include "integration/XBaseBridge.h"
#include <XBase/Runtime.h>
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "utils/I18n.h"
#include <XBase/Hooks.h>
#include <XBase/UI.h>
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
                XBase::UI::Spacing();
                XBase::UI::SeparatorText(translatedCategory);
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
        if (!MenuState::WeaponCyclerEnabled || XBase::Hooks::IsMenuVisible()) {
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

        const float wheel = XBase::Hooks::ConsumeWheelDelta();
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
        if (!XBaseBridge::HasCapability(XBase::FeatureCapability::WeaponBasic)) {
            XBase::UI::TextDisabled(T("weapon.unavailable"));
            return;
        }
        if (!Controllers::Weapon::HasPlayer()) {
            XBase::UI::Text(T("weapon.playerNotReady"));
            return;
        }

        if (UI::Button(T("weapon.dropWeapon"), 4)) {
            Controllers::Weapon::DropWeapon();
        }
        XBase::UI::SameLine();
        if (UI::Button(T("weapon.clearWeapons"), 4)) {
            Controllers::Weapon::ClearAll();
        }
        XBase::UI::SameLine();
        if (UI::Button(T("weapon.removePickups"), 4)) {
            Controllers::Weapon::RemovePickups();
        }

        XBase::UI::Spacing();

        XBase::UI::Tabs("WeaponTabs", [&] {
            XBase::UI::Tab("weapon.toggles", T("common.toggles"), [&] {
                const bool hasRuntimeEffects = XBaseBridge::HasCapability(XBase::FeatureCapability::WeaponRuntimeEffects);
                const bool hasStatOverrides = XBaseBridge::HasCapability(XBase::FeatureCapability::WeaponStatOverrides);
                bool weaponStatsChanged = false;

                XBase::UI::Columns(3, nullptr, false);
                XBase::UI::Disabled(!hasStatOverrides, [&] {
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.highDamage"), MenuState::HugeWeaponDamage);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasRuntimeEffects, [&] {
                    if (XBase::UI::Checkbox(T("weapon.fastReload"), MenuState::FastReload)) {
                        Controllers::Weapon::ResetStats();
                    }
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasRuntimeEffects, [&] {
                    XBase::UI::Checkbox(T("weapon.infiniteAmmo"), MenuState::InfiniteAmmo);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasStatOverrides, [&] {
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.longRange"), MenuState::LongWeaponRange);
                });
#ifdef GTASA
                XBase::UI::Disabled(!hasStatOverrides, [&] {
                    XBase::UI::NextColumn();
                    XBase::UI::Checkbox(T("weapon.autoAim"), MenuState::WeaponAutoAim);
                    XBase::UI::NextColumn();
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.moveWhileAiming"), MenuState::MoveAim);
                    XBase::UI::NextColumn();
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.moveWhileFiring"), MenuState::MoveFire);
                    XBase::UI::NextColumn();
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.noSpread"), MenuState::NoSpread);
                    XBase::UI::NextColumn();
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.rapidFire"), MenuState::RapidFire);
                    XBase::UI::NextColumn();
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.dualWield"), MenuState::DualWield);
                });
#else
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasStatOverrides, [&] {
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.noSpread"), MenuState::NoSpread);
                });
#endif
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasStatOverrides, [&] {
                    weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.fireRate"), MenuState::WeaponFireRateEnabled);
                });
                const auto hasBulletAssist = [](XBase::FeatureCapability capability) {
                    return XBaseBridge::HasCapability(capability);
                };
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistPedBounds), [&] {
                    XBase::UI::Checkbox(T("weapon.pedEsp"), MenuState::WeaponPedEsp);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistPedCollision), [&] {
                    XBase::UI::Checkbox(T("weapon.pedColEsp"), MenuState::WeaponPedColEsp);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistPedSkeleton), [&] {
                    XBase::UI::Checkbox(T("weapon.pedSkeleton"), MenuState::WeaponPedSkeleton);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistVehicleBounds), [&] {
                    XBase::UI::Checkbox(T("weapon.vehicleEsp"), MenuState::WeaponVehicleEsp);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistVehicleCollision), [&] {
                    XBase::UI::Checkbox(T("weapon.vehicleColEsp"), MenuState::WeaponVehicleColEsp);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistTracking), [&] {
                    XBase::UI::Checkbox(T("weapon.bulletTrack"), MenuState::WeaponBulletTrack);
                });
                XBase::UI::NextColumn();
                XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistThroughWalls), [&] {
                    XBase::UI::Checkbox(T("weapon.bulletThroughWalls"), MenuState::WeaponBulletThroughWalls);
                });
                XBase::UI::Columns(1);

                if (MenuState::WeaponFireRateEnabled) {
                    XBase::UI::Spacing();
                    XBase::UI::PushItemWidth(220.0f);
                    if (XBase::UI::Slider(T("weapon.fireRateValue"), MenuState::WeaponFireRate, 0.25f, 10.0f, "x%.2f")) {
                        weaponStatsChanged = true;
                    }
                    XBase::UI::PopItemWidth();
                    if (MenuState::WeaponFireRate < 0.25f) {
                        MenuState::WeaponFireRate = 0.25f;
                    }
                    if (MenuState::WeaponFireRate > 10.0f) {
                        MenuState::WeaponFireRate = 10.0f;
                    }
                }

                if (hasBulletAssist(XBase::FeatureCapability::BulletAssistTracking)
                    && MenuState::WeaponBulletTrack) {
                    XBase::UI::Spacing();
                    XBase::UI::Text(T("weapon.trackFilter"));
                    XBase::UI::Columns(5, nullptr, false);
                    XBase::UI::Checkbox(T("weapon.trackCivilian"), MenuState::WeaponTrackCivilian);
                    XBase::UI::NextColumn();
                    XBase::UI::Checkbox(T("weapon.trackFriend"), MenuState::WeaponTrackFriend);
                    XBase::UI::NextColumn();
                    XBase::UI::Checkbox(T("weapon.trackHostile"), MenuState::WeaponTrackHostile);
                    XBase::UI::NextColumn();
                    XBase::UI::Checkbox(T("weapon.trackNeutral"), MenuState::WeaponTrackNeutral);
                    XBase::UI::NextColumn();
                    XBase::UI::Disabled(!hasBulletAssist(XBase::FeatureCapability::BulletAssistHardLock), [&] {
                        XBase::UI::Checkbox(T("weapon.bulletHardLock"), MenuState::WeaponBulletHardLock);
                    });
                    XBase::UI::Columns(1);

                    XBase::UI::Spacing();
                    XBase::UI::Text(T("weapon.aimPart"));
                    XBase::UI::Columns(4, nullptr, false);
                    XBase::UI::Choice(T("weapon.aimHead"), MenuState::WeaponBulletAimPart, 0);
                    XBase::UI::NextColumn();
                    XBase::UI::Choice(T("weapon.aimChest"), MenuState::WeaponBulletAimPart, 1);
                    XBase::UI::NextColumn();
                    XBase::UI::Choice(T("weapon.aimBelly"), MenuState::WeaponBulletAimPart, 2);
                    XBase::UI::NextColumn();
                    XBase::UI::Choice(T("weapon.aimLegs"), MenuState::WeaponBulletAimPart, 3);
                    XBase::UI::Columns(1);
                    if (MenuState::WeaponBulletAimPart < 0) {
                        MenuState::WeaponBulletAimPart = 0;
                    }
                    if (MenuState::WeaponBulletAimPart > 3) {
                        MenuState::WeaponBulletAimPart = 3;
                    }

                    XBase::UI::Spacing();
                    XBase::UI::PushItemWidth(220.0f);
                    XBase::UI::Slider(T("weapon.bulletLockRange"), MenuState::WeaponBulletLockRange, 10.0f, 300.0f, "%.0f");
                    XBase::UI::Slider(T("weapon.bulletMaxTargets"), MenuState::WeaponBulletMaxTargets, 1, 16);
                    XBase::UI::PopItemWidth();
                    if (MenuState::WeaponBulletLockRange < 10.0f) {
                        MenuState::WeaponBulletLockRange = 10.0f;
                    }
                    if (MenuState::WeaponBulletLockRange > 300.0f) {
                        MenuState::WeaponBulletLockRange = 300.0f;
                    }
                    if (MenuState::WeaponBulletMaxTargets < 1) {
                        MenuState::WeaponBulletMaxTargets = 1;
                    }
                    if (MenuState::WeaponBulletMaxTargets > 16) {
                        MenuState::WeaponBulletMaxTargets = 16;
                    }
                }

                if (weaponStatsChanged) {
                    Controllers::Weapon::ResetStats();
                }

                });

            XBase::UI::Tab("weapon.getWeapon", T("weapon.getWeapon"), [&] {
                XBase::UI::PushItemWidth(160);
                XBase::UI::Input(T("weapon.ammo"), MenuState::WeaponAmmo);
                if (MenuState::WeaponAmmo < 0) MenuState::WeaponAmmo = 0;
                if (MenuState::WeaponAmmo > 99999) MenuState::WeaponAmmo = 99999;
                XBase::UI::PopItemWidth();

                XBase::UI::Checkbox(T("weapon.safeMode"), MenuState::WeaponSafeMode);

                XBase::UI::PushItemWidth(160);
#ifdef GTASA
                XBase::UI::Input(T("weapon.typeId"), MenuState::WeaponSpawnId);
#else
                XBase::UI::Input(T("weapon.modelId"), MenuState::WeaponSpawnId);
#endif
                if (MenuState::WeaponSpawnId < 0) MenuState::WeaponSpawnId = 0;
                XBase::UI::PopItemWidth();
                XBase::UI::SameLine();
                if (UI::Button(T("weapon.getById"), 2)) {
#ifdef GTASA
                    Controllers::Weapon::Give(static_cast<unsigned int>(MenuState::WeaponSpawnId), static_cast<unsigned int>(MenuState::WeaponAmmo));
#else
                    Controllers::Weapon::GiveModel(static_cast<unsigned int>(MenuState::WeaponSpawnId), static_cast<unsigned int>(MenuState::WeaponAmmo));
#endif
                }

                XBase::UI::Spacing();

                if (XBase::Runtime::GetGameTarget() == XBase::Runtime::GameTarget::III) {
                    if (UI::Button(T("weapon.getAll"), 4)) {
                        Controllers::Weapon::GiveAll();
                    }
                } else {
                    XBase::UI::Checkbox(T("weapon.cyclerEnable"), MenuState::WeaponCyclerEnabled);
                }

                if (XBase::Runtime::GetGameTarget() != XBase::Runtime::GameTarget::III) {
                    XBase::UI::SeparatorText(T("weapon.cyclerTitle"));

                    XBase::UI::PushItemWidth(160);
                    XBase::UI::Input("##cyclerInputId", MenuState::WeaponCyclerInputId);
                    if (MenuState::WeaponCyclerInputId < 0) MenuState::WeaponCyclerInputId = 0;
                    XBase::UI::PopItemWidth();
                    XBase::UI::SameLine();
                    if (UI::Button(T("weapon.cyclerGive"), 2)) {
                        const unsigned int ammo = static_cast<unsigned int>(MenuState::WeaponAmmo);
#ifdef GTASA
                        Controllers::Weapon::Give(static_cast<unsigned int>(MenuState::WeaponCyclerInputId), ammo);
#else
                        Controllers::Weapon::GiveModel(static_cast<unsigned int>(MenuState::WeaponCyclerInputId), ammo);
#endif
                    }

                    if (MenuState::WeaponCyclerEnabled) {
                        const float wheel = XBase::Hooks::ConsumeWheelDelta();
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
                        }
                    }
                }

                XBase::UI::Spacing();
                DrawWeaponList();
                });
            });
    }
}