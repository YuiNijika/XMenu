#include "Player.h"
#include "controllers/Player.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include <XBase/Hooks.h>
#include <XBase/Platform.h>
#include <XBase/UI.h>
#include "utils/I18n.h"
#include "integration/XBaseBridge.h"
#include <filesystem>
#include <vector>
#include <string>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }
}

namespace Pages::Player {
    void Process() {
        Controllers::Player::Process();
    }

    void Draw() {
        if (UI::Button(T("player.copyCoordinates"), 3)) {
            Controllers::Player::CopyCoordinates();
        }
        UI::SameLine();
        if (UI::Button(T("player.healFully"), 3)) {
            Controllers::Player::Heal();
        }
        UI::SameLine();
        if (UI::Button(T("player.refillArmor"), 3)) {
            Controllers::Player::GiveArmour();
        }
        XBase::UI::Disabled(!XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerSaveGame), [&] {
            if (UI::Button(T("player.saveAnywhere"), 3)) {
                if (Controllers::Player::RequestSaveGame()) {
                    XBase::Hooks::SetMenuVisible(false);
                }
            }
        });
        UI::SameLine();
        if (UI::Button(T("player.addMoney"), 3)) {
            Controllers::Player::GiveMoney();
        }
        UI::SameLine();
        if (UI::Button(T("player.kill"), 3)) {
            Controllers::Player::Kill();
        }

        UI::SpacingSeparator();

        XBase::UI::Tabs("PlayerTabBar", [&] {
            XBase::UI::Tab("player_toggles", T("common.toggles"), [&] {
                UI::Columns(3, nullptr, false);
                UI::Checkbox(T("player.godMode"), &MenuState::GodMode);
                UI::NextColumn();
                UI::Checkbox(T("player.autoHeal"), &MenuState::AutoHeal);
                UI::NextColumn();
                UI::Checkbox(T("player.hardMode"), &MenuState::HardMode);
                UI::NextColumn();

                if (UI::Checkbox(T("player.infiniteSprint"), &MenuState::InfiniteSprint)) {
                    Controllers::Player::SetInfiniteSprint(MenuState::InfiniteSprint);
                }
                UI::NextColumn();
                UI::Checkbox(T("player.respawnAtDeathPosition"), &MenuState::RespawnAtDeathPosition);
                UI::NextColumn();
                UI::Checkbox(T("player.freezeWantedLevel"), &MenuState::FreezeWantedLevel);
                UI::NextColumn();

                XBase::UI::Disabled(!XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerKeepStuff), [&] {
                    if (UI::Checkbox(T("player.keepStuff"), &MenuState::KeepStuff)) {
                        Controllers::Player::SetKeepStuff(MenuState::KeepStuff);
                    }
                });
                UI::NextColumn();
                UI::Checkbox(T("player.autoFlight"), &MenuState::FreeFlyEnabled);
                UI::NextColumn();

                const auto drawPlayerToggle = [&](XBase::FeatureCapability capability, const char* label, bool& value) {
                    XBase::UI::Disabled(!XBaseBridge::HasCapability(capability), [&] {
                        UI::Checkbox(label, &value);
                    });
                    UI::NextColumn();
                };
                drawPlayerToggle(XBase::FeatureCapability::PlayerNeverWanted,
                    T("player.neverWanted"), MenuState::NeverWanted);
                drawPlayerToggle(XBase::FeatureCapability::PlayerSuperJump,
                    T("player.megaJump"), MenuState::MegaJump);
                drawPlayerToggle(XBase::FeatureCapability::PlayerSuperPunch,
                    T("player.megaPunch"), MenuState::MegaPunch);
                drawPlayerToggle(XBase::FeatureCapability::PlayerCycleJump,
                    T("player.cycleJump"), MenuState::CycleJump);
                drawPlayerToggle(XBase::FeatureCapability::PlayerUnderwaterBreathing,
                    T("player.infiniteOxygen"), MenuState::InfiniteOxygen);
                drawPlayerToggle(XBase::FeatureCapability::PlayerNeverHungry,
                    T("player.neverHungry"), MenuState::NeverHungry);
                drawPlayerToggle(XBase::FeatureCapability::PlayerFastSprint,
                    T("player.fastSprint"), MenuState::FastSprint);
                drawPlayerToggle(XBase::FeatureCapability::PlayerDrunkEffect,
                    T("player.drunkEffect"), MenuState::DrunkEffect);
                drawPlayerToggle(XBase::FeatureCapability::PlayerSprintEverywhere,
                    T("player.sprintEverywhere"), MenuState::SprintEverywhere);
                drawPlayerToggle(XBase::FeatureCapability::PlayerAimSkinChanger,
                    T("player.aimSkinChanger"), MenuState::AimSkinChanger);

                #ifdef GTASA
                XBase::UI::Disabled(!XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerRuntimeEffects), [&] {
                    UI::Checkbox(T("player.invisible"), &MenuState::InvisiblePlayer);
                });
                UI::NextColumn();
#endif

                bool freeHealth = Controllers::Player::GetFreeHealthcare();
                if (UI::Checkbox(T("player.freeHospital"), &freeHealth)) {
                    Controllers::Player::SetFreeHealthcare(freeHealth);
                }
                UI::NextColumn();
                bool freeJail = Controllers::Player::GetFreeJail();
                if (UI::Checkbox(T("player.freeJail"), &freeJail)) {
                    Controllers::Player::SetFreeJail(freeJail);
                }
                UI::Columns(1);

                UI::SpacingSeparator();
                if (!MenuState::UseNativeMenu) XBase::UI::Text(T("player.autoFlightOptions"));
                UI::PushItemWidth(160);
                UI::SliderFloat(T("player.autoFlightSpeed"), &MenuState::FreeFlySpeed, 0.1f, 5.0f, "%.1f");
                UI::PopItemWidth();

                UI::SpacingSeparator();
                if (!MenuState::UseNativeMenu) XBase::UI::Text(T("player.proofFlags"));

                GameTypes::ProofState proofs = Controllers::Player::GetProofState();
                MenuState::BulletProof = proofs.bullet;
                MenuState::CollisionProof = proofs.collision;
                MenuState::ExplosionProof = proofs.explosion;
                MenuState::FireProof = proofs.fire;
                MenuState::MeleeProof = proofs.melee;

                bool changedProof = false;
                XBase::UI::Disabled(MenuState::GodMode, [&] {
                    UI::Columns(3, nullptr, false);
                    changedProof |= UI::Checkbox(T("proof.bullet"), &MenuState::BulletProof);
                    UI::NextColumn();
                    changedProof |= UI::Checkbox(T("proof.collision"), &MenuState::CollisionProof);
                    UI::NextColumn();
                    changedProof |= UI::Checkbox(T("proof.explosion"), &MenuState::ExplosionProof);
                    UI::NextColumn();
                    changedProof |= UI::Checkbox(T("proof.fire"), &MenuState::FireProof);
                    UI::NextColumn();
                    changedProof |= UI::Checkbox(T("proof.melee"), &MenuState::MeleeProof);
                    UI::Columns(1);
                });

                if (changedProof) {
                    proofs.bullet = MenuState::BulletProof;
                    proofs.collision = MenuState::CollisionProof;
                    proofs.explosion = MenuState::ExplosionProof;
                    proofs.fire = MenuState::FireProof;
                    proofs.melee = MenuState::MeleeProof;
                    Controllers::Player::SetProofState(proofs);
                }

                });

            XBase::UI::Tab("player_values", T("player.valueAdjustments"), [&] {
                static bool valuesInitialized = false;
                if (!valuesInitialized) {
                    MenuState::PlayerHealth = Controllers::Player::GetHealth();
                    MenuState::PlayerArmour = Controllers::Player::GetArmour();
                    MenuState::PlayerMoney = Controllers::Player::GetMoney();
                    MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                    valuesInitialized = true;
                }

                UI::PushItemWidth(180);
                if (UI::InputFloat(T("player.health"), &MenuState::PlayerHealth, 1.0f, 10.0f, "%.1f")) {
                    if (MenuState::UseNativeMenu) Controllers::Player::SetHealth(MenuState::PlayerHealth);
                }
                UI::SameLine();
                if (UI::Button(T("player.setHealth"), 4)) {
                    if (MenuState::PlayerHealth > 0.0f && MenuState::PlayerHealth < 2.0f) {
                        MenuState::PlayerHealth = 2.0f;
                    }
                    Controllers::Player::SetHealth(MenuState::PlayerHealth);
                }

                if (UI::InputFloat(T("player.armor"), &MenuState::PlayerArmour, 1.0f, 10.0f, "%.1f")) {
                    if (MenuState::UseNativeMenu) Controllers::Player::SetArmour(MenuState::PlayerArmour);
                }
                UI::SameLine();
                if (UI::Button(T("player.setArmor"), 4)) {
                    Controllers::Player::SetArmour(MenuState::PlayerArmour);
                }

                if (UI::InputInt(T("player.money"), &MenuState::PlayerMoney, 1000, 10000)) {
                    if (MenuState::UseNativeMenu) Controllers::Player::SetMoney(MenuState::PlayerMoney);
                }
                UI::SameLine();
                if (UI::Button(T("player.setMoney"), 4)) {
                    Controllers::Player::SetMoney(MenuState::PlayerMoney);
                }

                if (UI::SliderInt(T("player.wantedLevel"), &MenuState::WantedLevel, 0, 6)) {
                    if (MenuState::UseNativeMenu) Controllers::Player::SetWantedLevel(MenuState::WantedLevel);
                }
                UI::SameLine();
                if (UI::Button(T("player.setWanted"), 4)) {
                    Controllers::Player::SetWantedLevel(MenuState::WantedLevel);
                }
                UI::PopItemWidth();

                if (UI::Button(T("player.readCurrentValues"), 2)) {
                    MenuState::PlayerHealth = Controllers::Player::GetHealth();
                    MenuState::PlayerArmour = Controllers::Player::GetArmour();
                    MenuState::PlayerMoney = Controllers::Player::GetMoney();
                    MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                }
                UI::SameLine();
                if (UI::Button(T("player.clearWanted"), 2)) {
                    MenuState::WantedLevel = 0;
                    Controllers::Player::ClearWantedLevel();
                }

                });

#ifdef GTASA
            XBase::UI::Tab("player_appearance", T("player.appearance"), [&] {
                const bool canAppearance =
                    XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerAppearance);
                XBase::UI::Disabled(!canAppearance, [&] {
                    UI::PushItemWidth(160);
                    UI::InputInt(T("player.skinModel"), &MenuState::PlayerSkinModel);
                    UI::PopItemWidth();
                    UI::SameLine();
                    if (UI::Button(T("player.applySkin"), 4)) {
                        Controllers::Player::SetSkin(static_cast<unsigned int>(
                            MenuState::PlayerSkinModel < 0 ? 0 : MenuState::PlayerSkinModel));
                    }
                });

                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerClothes), [&] {
                        UI::PushItemWidth(120);
                        UI::InputInt(T("player.clothesTexture"), &MenuState::PlayerClothesTexture);
                        UI::SameLine();
                        UI::InputInt(T("player.clothesModel"), &MenuState::PlayerClothesModel);
                        UI::SameLine();
                        UI::InputInt(T("player.clothesBodyPart"), &MenuState::PlayerClothesBodyPart);
                        UI::PopItemWidth();
                        if (UI::Button(T("player.applyClothes"), 2)) {
                            Controllers::Player::ApplyClothes(
                                MenuState::PlayerClothesTexture,
                                MenuState::PlayerClothesModel,
                                MenuState::PlayerClothesBodyPart);
                        }
                    });

                UI::SpacingSeparator();
                XBase::UI::Disabled(
                    !XBaseBridge::HasCapability(XBase::FeatureCapability::PlayerStats), [&] {
                        UI::PushItemWidth(160);
                        UI::InputInt(T("player.statId"), &MenuState::PlayerStatId);
                        UI::SameLine();
                        UI::InputFloat(T("player.statValue"), &MenuState::PlayerStatValue, 1.0f, 10.0f, "%.1f");
                        UI::PopItemWidth();
                        UI::SameLine();
                        if (UI::Button(T("player.setStat"), 4)) {
                            Controllers::Player::SetStat(MenuState::PlayerStatId, MenuState::PlayerStatValue);
                        }
                        if (UI::Button(T("player.maxWeaponSkills"), 2)) {
                            Controllers::Player::MaxWeaponSkills();
                        }
                        UI::SameLine();
                        if (UI::Button(T("player.maxVehicleSkills"), 2)) {
                            Controllers::Player::MaxVehicleSkills();
                        }
                    });
                });

            XBase::UI::Tab("player_skins", T("player.customSkins"), [&] {
                if (!MenuState::UseNativeMenu) {
                    XBase::UI::TextWrapped(T("player.customSkinsTip"));
                    XBase::UI::Spacing();
                }

                static std::vector<std::string> customSkins;
                static bool skinsLoaded = false;
                static char skinFilter[128] = {};

                if (!skinsLoaded) {
                    if (XBase::Platform::IsModuleLoaded("modloader.asi")) {
                        std::string path = "modloader/CustomSkinsLoader/";
                        if (std::filesystem::is_directory(path)) {
                            for (auto& p : std::filesystem::recursive_directory_iterator(path)) {
                                if (p.path().extension() == ".dff") {
                                    std::string name = p.path().stem().string();
                                    if (name.length() < 9) {
                                        customSkins.push_back(name);
                                    }
                                }
                            }
                        } else {
                            std::error_code ec;
                            std::filesystem::create_directory(path, ec);
                        }
                    }
                    skinsLoaded = true;
                }

                if (customSkins.empty()) {
                    if (!MenuState::UseNativeMenu) XBase::UI::TextDisabled(T("player.noCustomSkins"));
                    else UI::Button(T("player.noCustomSkins")); // Native 下作为提示
                } else {
                    if (!MenuState::UseNativeMenu) {
                        XBase::UI::InputText(T("common.search"), skinFilter, sizeof(skinFilter));
                        XBase::UI::Spacing();
                        XBase::UI::Child("CustomSkinsList", [&] {
                            for (const std::string& skin : customSkins) {
                                if (skinFilter[0] != '\0' && skin.find(skinFilter) == std::string::npos) {
                                    continue;
                                }
                                if (XBase::UI::Selectable(skin.c_str())) {
                                    Controllers::Player::SetCustomSkin(skin.c_str());
                                }
                            }
                        }, {}, true);
                    } else {
                        // Native 模式下直接渲染成列表
                        for (const std::string& skin : customSkins) {
                            if (UI::Button(skin.c_str())) {
                                Controllers::Player::SetCustomSkin(skin.c_str());
                            }
                        }
                    }
                }

                });
#endif

            });

    }
}