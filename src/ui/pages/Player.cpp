#include "Player.h"
#include "controllers/Player.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "imgui/imgui.h"

namespace Pages::Player {
    void Process() {
        Controllers::Player::Process();
    }

    void Draw() {
        if (UI::Button((const char*)u8"复制坐标", 3)) {
            Controllers::Player::CopyCoordinates();
        }
        ImGui::SameLine();
        if (UI::Button((const char*)u8"回满血量", 3)) {
            Controllers::Player::Heal();
        }
        ImGui::SameLine();
        if (UI::Button((const char*)u8"补满护甲", 3)) {
            Controllers::Player::GiveArmour();
        }
        if (UI::Button((const char*)u8"加 25 万现金", 2)) {
            Controllers::Player::GiveMoney();
        }
        ImGui::SameLine();
        if (UI::Button((const char*)u8"立即倒地", 2)) {
            Controllers::Player::Kill();
        }

        UI::SpacingSeparator();

        if (UI::BeginTabBar("PlayerTabs")) {
            if (ImGui::BeginTabItem((const char*)u8"状态开关")) {
                ImGui::Columns(3, nullptr, false);
                ImGui::Checkbox((const char*)u8"无敌", &MenuState::GodMode);
                ImGui::NextColumn();
                ImGui::Checkbox((const char*)u8"自动回血", &MenuState::AutoHeal);
                ImGui::NextColumn();
                ImGui::Checkbox((const char*)u8"50 血量", &MenuState::HardMode);
                ImGui::NextColumn();

                if (ImGui::Checkbox((const char*)u8"无限冲刺", &MenuState::InfiniteSprint)) {
                    Controllers::Player::SetInfiniteSprint(MenuState::InfiniteSprint);
                }
                ImGui::NextColumn();
                ImGui::Checkbox((const char*)u8"死亡后回到原地", &MenuState::RespawnAtDeathPosition);
                ImGui::NextColumn();
                ImGui::Checkbox((const char*)u8"冻结通缉", &MenuState::FreezeWantedLevel);
                ImGui::NextColumn();

                if (ImGui::Checkbox((const char*)u8"住院/被捕保留装备", &MenuState::KeepStuff)) {
                    Controllers::Player::SetKeepStuff(MenuState::KeepStuff);
                }
                ImGui::NextColumn();

                bool freeHealth = Controllers::Player::GetFreeHealthcare();
                if (ImGui::Checkbox((const char*)u8"住院不扣钱", &freeHealth)) {
                    Controllers::Player::SetFreeHealthcare(freeHealth);
                }
                ImGui::NextColumn();
                bool freeJail = Controllers::Player::GetFreeJail();
                if (ImGui::Checkbox((const char*)u8"被捕不扣钱", &freeJail)) {
                    Controllers::Player::SetFreeJail(freeJail);
                }
                ImGui::Columns(1);

                UI::SpacingSeparator();
                ImGui::TextUnformatted((const char*)u8"单项防护");

                GameLogic::ProofState proofs = Controllers::Player::GetProofState();
                MenuState::BulletProof = proofs.bullet;
                MenuState::CollisionProof = proofs.collision;
                MenuState::ExplosionProof = proofs.explosion;
                MenuState::FireProof = proofs.fire;
                MenuState::MeleeProof = proofs.melee;

                ImGui::BeginDisabled(MenuState::GodMode);
                ImGui::Columns(3, nullptr, false);
                bool changedProof = false;
                changedProof |= ImGui::Checkbox((const char*)u8"防弹", &MenuState::BulletProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox((const char*)u8"防撞", &MenuState::CollisionProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox((const char*)u8"防爆", &MenuState::ExplosionProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox((const char*)u8"防火", &MenuState::FireProof);
                ImGui::NextColumn();
                changedProof |= ImGui::Checkbox((const char*)u8"防近战", &MenuState::MeleeProof);
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

            if (ImGui::BeginTabItem((const char*)u8"数值调整")) {
                static bool valuesInitialized = false;
                if (!valuesInitialized) {
                    MenuState::PlayerHealth = Controllers::Player::GetHealth();
                    MenuState::PlayerArmour = Controllers::Player::GetArmour();
                    MenuState::PlayerMoney = Controllers::Player::GetMoney();
                    MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                    valuesInitialized = true;
                }

                ImGui::PushItemWidth(180);
                ImGui::InputFloat((const char*)u8"血量", &MenuState::PlayerHealth, 1.0f, 10.0f, "%.1f");
                ImGui::SameLine();
                if (UI::Button((const char*)u8"设置血量", 4)) {
                    if (MenuState::PlayerHealth > 0.0f && MenuState::PlayerHealth < 2.0f) {
                        MenuState::PlayerHealth = 2.0f;
                    }
                    Controllers::Player::SetHealth(MenuState::PlayerHealth);
                }

                ImGui::InputFloat((const char*)u8"护甲", &MenuState::PlayerArmour, 1.0f, 10.0f, "%.1f");
                ImGui::SameLine();
                if (UI::Button((const char*)u8"设置护甲", 4)) {
                    Controllers::Player::SetArmour(MenuState::PlayerArmour);
                }

                ImGui::InputInt((const char*)u8"现金", &MenuState::PlayerMoney, 1000, 10000);
                ImGui::SameLine();
                if (UI::Button((const char*)u8"设置现金", 4)) {
                    Controllers::Player::SetMoney(MenuState::PlayerMoney);
                }

                ImGui::SliderInt((const char*)u8"通缉星级", &MenuState::WantedLevel, 0, 6);
                ImGui::SameLine();
                if (UI::Button((const char*)u8"设置通缉", 4)) {
                    Controllers::Player::SetWantedLevel(MenuState::WantedLevel);
                }
                ImGui::PopItemWidth();

                if (UI::Button((const char*)u8"读取当前数值", 2)) {
                    MenuState::PlayerHealth = Controllers::Player::GetHealth();
                    MenuState::PlayerArmour = Controllers::Player::GetArmour();
                    MenuState::PlayerMoney = Controllers::Player::GetMoney();
                    MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                }
                ImGui::SameLine();
                if (UI::Button((const char*)u8"消除通缉", 2)) {
                    MenuState::WantedLevel = 0;
                    Controllers::Player::ClearWantedLevel();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}