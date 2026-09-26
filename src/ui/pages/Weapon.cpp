#include "Weapon.h"
#include "controllers/Weapon.h"
#include "integration/XBaseBridge.h"
#include <XBase/Runtime.h>
#include "ui/MenuState.h"
#include "resources/ResourceData.h"
#include "ui/UiSchema.h"
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

        // Wheel cycling runs only with the menu closed. While it is armed the
        // The wheel weapon switch of the game is blocked so the list order wins.
        const bool cyclerActive = MenuState::WeaponCyclerEnabled && !XBase::Hooks::IsMenuVisible();
        XBase::Hooks::SetWheelInputSuppressed(cyclerActive);
        if (!cyclerActive) {
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

        const int lastIdx = static_cast<int>(s_cyclerWeapons.size()) - 1;
        int& cyclerIdx = MenuState::WeaponCyclerId;
        const int direction = wheel > 0.0f ? 1 : -1;
        float remaining = wheel > 0.0f ? wheel : -wheel;
        const unsigned int ammo = static_cast<unsigned int>(MenuState::WeaponAmmo);

        while (remaining > 0.1f) {
            if (cyclerIdx < 0 || cyclerIdx > lastIdx) {
                cyclerIdx = direction > 0 ? -1 : 0;
            }
            cyclerIdx += direction;
            if (cyclerIdx > lastIdx) cyclerIdx = 0;
            if (cyclerIdx < 0) cyclerIdx = lastIdx;

            const Resources::WeaponEntry& entry = s_cyclerWeapons[cyclerIdx];
            if (entry.isModel) {
                Controllers::Weapon::GiveModelSilent(static_cast<unsigned int>(entry.modelId), ammo);
            } else {
                Controllers::Weapon::GiveSilent(static_cast<unsigned int>(entry.id), ammo);
            }
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
                const bool hasBulletAssist = XBaseBridge::HasCapability(XBase::FeatureCapability::BulletAssistTracking);
                bool weaponStatsChanged = false;

                // 运行时效果、属性改写与子弹辅助都交给界面注册表，网页界面读的是同一份配置
                if (!UiSchema::DrawSection("weapon", "weaponMain", "runtime")) {
                    XBase::UI::Columns(3, nullptr, false);
                    XBase::UI::Disabled(!hasRuntimeEffects, [&] {
                        if (XBase::UI::Checkbox(T("weapon.fastReload"), MenuState::FastReload)) {
                            Controllers::Weapon::ResetStats();
                        }
                    });
                    XBase::UI::NextColumn();
                    XBase::UI::Disabled(!hasRuntimeEffects, [&] {
                        XBase::UI::Checkbox(T("weapon.infiniteAmmo"), MenuState::InfiniteAmmo);
                    });
                    XBase::UI::Columns(1);
                }

                if (!UiSchema::DrawSection("weapon", "weaponMain", "statOverrides")) {
                    XBase::UI::Columns(3, nullptr, false);
                    XBase::UI::Disabled(!hasStatOverrides, [&] {
                        weaponStatsChanged |= XBase::UI::Checkbox(T("weapon.highDamage"), MenuState::HugeWeaponDamage);
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
                }

                if (!UiSchema::DrawSection("weapon", "weaponMain", "bulletAssist")) {
                    XBase::UI::Columns(3, nullptr, false);
                    const XBase::FeatureCapability pedCol = XBase::FeatureCapability::BulletAssistPedCollision;
                    const XBase::FeatureCapability pedSkel = XBase::FeatureCapability::BulletAssistPedSkeleton;
                    const XBase::FeatureCapability vehBounds = XBase::FeatureCapability::BulletAssistVehicleBounds;
                    const XBase::FeatureCapability vehCol = XBase::FeatureCapability::BulletAssistVehicleCollision;
                    const XBase::FeatureCapability tracking = XBase::FeatureCapability::BulletAssistTracking;
                    const XBase::FeatureCapability throughWalls = XBase::FeatureCapability::BulletAssistThroughWalls;

                    const auto drawBulletAssist = [&](XBase::FeatureCapability capability, bool& value, const char* label) {
                        XBase::UI::NextColumn();
                        XBase::UI::Disabled(!XBaseBridge::HasCapability(capability), [&] {
                            XBase::UI::Checkbox(label, value);
                        });
                    };
                    XBase::UI::Disabled(!XBaseBridge::HasCapability(XBase::FeatureCapability::BulletAssistPedBounds), [&] {
                        XBase::UI::Checkbox(T("weapon.pedEsp"), MenuState::WeaponPedEsp);
                    });
                    drawBulletAssist(pedCol, MenuState::WeaponPedColEsp, T("weapon.pedColEsp"));
                    drawBulletAssist(pedSkel, MenuState::WeaponPedSkeleton, T("weapon.pedSkeleton"));
                    drawBulletAssist(vehBounds, MenuState::WeaponVehicleEsp, T("weapon.vehicleEsp"));
                    drawBulletAssist(vehCol, MenuState::WeaponVehicleColEsp, T("weapon.vehicleColEsp"));
                    drawBulletAssist(tracking, MenuState::WeaponBulletTrack, T("weapon.bulletTrack"));
                    drawBulletAssist(throughWalls, MenuState::WeaponBulletThroughWalls, T("weapon.bulletThroughWalls"));

                    if (hasBulletAssist && MenuState::WeaponBulletTrack) {
                        XBase::UI::Columns(1);
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
                        XBase::UI::Disabled(!XBaseBridge::HasCapability(XBase::FeatureCapability::BulletAssistHardLock), [&] {
                            XBase::UI::Checkbox(T("weapon.bulletHardLock"), MenuState::WeaponBulletHardLock);
                        });
                        XBase::UI::Columns(1);

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
                    XBase::UI::Columns(1);
                }

                // 优先锁定部位已登记成下拉放进 bulletAssist 分区，由注册表绘制

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

                // 安全模式与滚轮刷出走界面注册表，III 上的获取全部武器同样是注册表的动作
                if (!UiSchema::DrawSection("weapon", "weaponMain", "giveOptions")) {
                    XBase::UI::Checkbox(T("weapon.safeMode"), MenuState::WeaponSafeMode);
                    if (XBase::Runtime::GetGameTarget() == XBase::Runtime::GameTarget::III) {
                        if (UI::Button(T("weapon.getAll"), 4)) {
                            Controllers::Weapon::GiveAll();
                        }
                    }
                }

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

                // 滚轮刷出与后面的输入刷出是一组，留在原生代码里，不拆进注册表
                if (XBase::Runtime::GetGameTarget() != XBase::Runtime::GameTarget::III) {
                    XBase::UI::Checkbox(T("weapon.cyclerEnable"), MenuState::WeaponCyclerEnabled);
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
                }

                XBase::UI::Spacing();
                DrawWeaponList();
                });
            });
    }
}