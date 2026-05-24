#include "Player.h"
#include "controllers/Player.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/D3DHook.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"

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
        ImGui::SameLine();
        if (UI::Button(T("player.healFully"), 3)) {
            Controllers::Player::Heal();
        }
        ImGui::SameLine();
        if (UI::Button(T("player.refillArmor"), 3)) {
            Controllers::Player::GiveArmour();
        }
        if (UI::Button(T("player.saveAnywhere"), 3)) {
            if (Controllers::Player::RequestSaveGame()) {
                D3DHook::SetMenuVisible(false);
            }
        }
        ImGui::SameLine();
        if (UI::Button(T("player.addMoney"), 3)) {
            Controllers::Player::GiveMoney();
        }
        ImGui::SameLine();
        if (UI::Button(T("player.kill"), 3)) {
            Controllers::Player::Kill();
        }

        UI::SpacingSeparator();

        if (UI::BeginTabBar("PlayerTabs")) {
            if (ImGui::BeginTabItem(T("player.statusToggles"))) {
                ImGui::Columns(3, nullptr, false);
                ImGui::Checkbox(T("player.godMode"), &MenuState::GodMode);
                ImGui::NextColumn();
                ImGui::Checkbox(T("player.autoHeal"), &MenuState::AutoHeal);
                ImGui::NextColumn();
                ImGui::Checkbox(T("player.hardMode"), &MenuState::HardMode);
                ImGui::NextColumn();

                if (ImGui::Checkbox(T("player.infiniteSprint"), &MenuState::InfiniteSprint)) {
                    Controllers::Player::SetInfiniteSprint(MenuState::InfiniteSprint);
                }
                ImGui::NextColumn();
                ImGui::Checkbox(T("player.respawnAtDeathPosition"), &MenuState::RespawnAtDeathPosition);
                ImGui::NextColumn();
                ImGui::Checkbox(T("player.freezeWantedLevel"), &MenuState::FreezeWantedLevel);
                ImGui::NextColumn();

                if (ImGui::Checkbox(T("player.keepStuff"), &MenuState::KeepStuff)) {
                    Controllers::Player::SetKeepStuff(MenuState::KeepStuff);
                }
                ImGui::NextColumn();

                bool freeHealth = Controllers::Player::GetFreeHealthcare();
                if (ImGui::Checkbox(T("player.freeHospital"), &freeHealth)) {
                    Controllers::Player::SetFreeHealthcare(freeHealth);
                }
                ImGui::NextColumn();
                bool freeJail = Controllers::Player::GetFreeJail();
                if (ImGui::Checkbox(T("player.freeJail"), &freeJail)) {
                    Controllers::Player::SetFreeJail(freeJail);
                }
                ImGui::Columns(1);

                UI::SpacingSeparator();
                ImGui::TextUnformatted(T("player.proofFlags"));

                GameLogic::ProofState proofs = Controllers::Player::GetProofState();
                MenuState::BulletProof = proofs.bullet;
                MenuState::CollisionProof = proofs.collision;
                MenuState::ExplosionProof = proofs.explosion;
                MenuState::FireProof = proofs.fire;
                MenuState::MeleeProof = proofs.melee;

                ImGui::BeginDisabled(MenuState::GodMode);
                ImGui::Columns(3, nullptr, false);
                bool changedProof = false;
                changedProof |= ImGui::Checkbox(T("proof.bullet"), &MenuState::BulletProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox(T("proof.collision"), &MenuState::CollisionProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox(T("proof.explosion"), &MenuState::ExplosionProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox(T("proof.fire"), &MenuState::FireProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox(T("proof.melee"), &MenuState::MeleeProof);
                ImGui::Columns(1);
                ImGui::EndDisabled();

                if (changedProof) {
                    proofs.bullet = MenuState::BulletProof;
                    proofs.collision = MenuState::CollisionProof;
                    proofs.explosion = MenuState::ExplosionProof;
                    proofs.fire = MenuState::FireProof;
                    proofs.melee = MenuState::MeleeProof;
                    Controllers::Player::SetProofState(proofs);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(T("player.valueAdjustments"))) {
                static bool valuesInitialized = false;
                if (!valuesInitialized) {
                    MenuState::PlayerHealth = Controllers::Player::GetHealth();
                    MenuState::PlayerArmour = Controllers::Player::GetArmour();
                    MenuState::PlayerMoney = Controllers::Player::GetMoney();
                    MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                    valuesInitialized = true;
                }

                ImGui::PushItemWidth(180);
                ImGui::InputFloat(T("player.health"), &MenuState::PlayerHealth, 1.0f, 10.0f, "%.1f");
                ImGui::SameLine();
                if (UI::Button(T("player.setHealth"), 4)) {
                    if (MenuState::PlayerHealth > 0.0f && MenuState::PlayerHealth < 2.0f) {
                        MenuState::PlayerHealth = 2.0f;
                    }
                    Controllers::Player::SetHealth(MenuState::PlayerHealth);
                }

                ImGui::InputFloat(T("player.armor"), &MenuState::PlayerArmour, 1.0f, 10.0f, "%.1f");
                ImGui::SameLine();
                if (UI::Button(T("player.setArmor"), 4)) {
                    Controllers::Player::SetArmour(MenuState::PlayerArmour);
                }

                ImGui::InputInt(T("player.money"), &MenuState::PlayerMoney, 1000, 10000);
                ImGui::SameLine();
                if (UI::Button(T("player.setMoney"), 4)) {
                    Controllers::Player::SetMoney(MenuState::PlayerMoney);
                }

                ImGui::SliderInt(T("player.wantedLevel"), &MenuState::WantedLevel, 0, 6);
                ImGui::SameLine();
                if (UI::Button(T("player.setWanted"), 4)) {
                    Controllers::Player::SetWantedLevel(MenuState::WantedLevel);
                }
                ImGui::PopItemWidth();

                if (UI::Button(T("player.readCurrentValues"), 2)) {
                    MenuState::PlayerHealth = Controllers::Player::GetHealth();
                    MenuState::PlayerArmour = Controllers::Player::GetArmour();
                    MenuState::PlayerMoney = Controllers::Player::GetMoney();
                    MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                }
                ImGui::SameLine();
                if (UI::Button(T("player.clearWanted"), 2)) {
                    MenuState::WantedLevel = 0;
                    Controllers::Player::ClearWantedLevel();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}