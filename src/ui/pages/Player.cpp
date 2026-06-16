#include "Player.h"
#include "controllers/Player.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/D3DHook.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
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
        UI::BeginPage("player", "PLAYER", "OPTIONS");
        
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
        if (UI::Button(T("player.saveAnywhere"), 3)) {
            if (Controllers::Player::RequestSaveGame()) {
                D3DHook::SetMenuVisible(false);
            }
        }
        UI::SameLine();
        if (UI::Button(T("player.addMoney"), 3)) {
            Controllers::Player::GiveMoney();
        }
        UI::SameLine();
        if (UI::Button(T("player.kill"), 3)) {
            Controllers::Player::Kill();
        }

        UI::SpacingSeparator();

        if (UI::BeginTabBar("PlayerTabBar")) {
            if (UI::BeginTab("player_toggles", T("common.toggles"))) {
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

                if (UI::Checkbox(T("player.keepStuff"), &MenuState::KeepStuff)) {
                    Controllers::Player::SetKeepStuff(MenuState::KeepStuff);
                }
                UI::NextColumn();
                UI::Checkbox(T("player.autoFlight"), &MenuState::FreeFlyEnabled);
                UI::NextColumn();

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
                if (!MenuState::UseNativeMenu) ImGui::TextUnformatted(T("player.autoFlightOptions"));
                UI::PushItemWidth(160);
                UI::SliderFloat(T("player.autoFlightSpeed"), &MenuState::FreeFlySpeed, 0.1f, 5.0f, "%.1f");
                UI::PopItemWidth();

                UI::SpacingSeparator();
                if (!MenuState::UseNativeMenu) ImGui::TextUnformatted(T("player.proofFlags"));

                GameTypes::ProofState proofs = Controllers::Player::GetProofState();
                MenuState::BulletProof = proofs.bullet;
                MenuState::CollisionProof = proofs.collision;
                MenuState::ExplosionProof = proofs.explosion;
                MenuState::FireProof = proofs.fire;
                MenuState::MeleeProof = proofs.melee;

                UI::BeginDisabled(MenuState::GodMode);
                UI::Columns(3, nullptr, false);
                bool changedProof = false;
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
                UI::EndDisabled();

                if (changedProof) {
                    proofs.bullet = MenuState::BulletProof;
                    proofs.collision = MenuState::CollisionProof;
                    proofs.explosion = MenuState::ExplosionProof;
                    proofs.fire = MenuState::FireProof;
                    proofs.melee = MenuState::MeleeProof;
                    Controllers::Player::SetProofState(proofs);
                }

                UI::EndTab();
            }

            if (UI::BeginTab("player_values", T("player.valueAdjustments"))) {
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

                UI::EndTab();
            }

#ifdef GTASA
            if (UI::BeginTab("player_skins", T("player.customSkins"))) {
                if (!MenuState::UseNativeMenu) {
                    ImGui::TextWrapped("%s", T("player.customSkinsTip"));
                    ImGui::Spacing();
                }

                static std::vector<std::string> customSkins;
                static bool skinsLoaded = false;
                static ImGuiTextFilter filter;

                if (!skinsLoaded) {
                    if (GetModuleHandleA("modloader.asi")) {
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
                    if (!MenuState::UseNativeMenu) ImGui::TextDisabled("%s", T("player.noCustomSkins"));
                    else UI::Button(T("player.noCustomSkins")); // Native 下作为提示
                } else {
                    if (!MenuState::UseNativeMenu) {
                        filter.Draw(T("common.search"));
                        ImGui::Spacing();
                        if (ImGui::BeginChild("CustomSkinsList", ImVec2(0, 0), true)) {
                            for (const std::string& skin : customSkins) {
                                if (filter.PassFilter(skin.c_str())) {
                                    if (ImGui::Selectable(skin.c_str())) {
                                        Controllers::Player::SetCustomSkin(skin.c_str());
                                    }
                                }
                            }
                        }
                        ImGui::EndChild();
                    } else {
                        // Native 模式下直接渲染成列表
                        for (const std::string& skin : customSkins) {
                            if (UI::Button(skin.c_str())) {
                                Controllers::Player::SetCustomSkin(skin.c_str());
                            }
                        }
                    }
                }

                UI::EndTab();
            }
#endif

            UI::EndTabBar();
        }

        UI::EndPage();
    }
}