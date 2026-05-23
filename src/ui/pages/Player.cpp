#include "Player.h"
#include "controllers/Player.h"
#include "ui/MenuState.h"
#include "imgui/imgui.h"

namespace Pages::Player {
    void Process() {
        Controllers::Player::Process();
    }

    void Draw() {
        if (ImGui::Button((const char*)u8"复制坐标")) {
            Controllers::Player::CopyCoordinates();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"回满血量")) {
            Controllers::Player::Heal();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"补满护甲")) {
            Controllers::Player::GiveArmour();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"加 25 万现金")) {
            Controllers::Player::GiveMoney();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"立即倒地")) {
            Controllers::Player::Kill();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("PlayerTabs", ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem((const char*)u8"状态开关")) {
                ImGui::Checkbox((const char*)u8"无敌", &MenuState::GodMode);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"自动回血", &MenuState::AutoHeal);
                ImGui::SameLine();
                ImGui::Checkbox((const char*)u8"50 血量", &MenuState::HardMode);

                if (ImGui::Checkbox((const char*)u8"无限冲刺", &MenuState::InfiniteSprint)) {
                    Controllers::Player::SetInfiniteSprint(MenuState::InfiniteSprint);
                }
                ImGui::SameLine();
                if (ImGui::Checkbox((const char*)u8"死亡后回到原地", &MenuState::RespawnAtDeathPosition)) {
                }
                ImGui::SameLine();
                if (ImGui::Checkbox((const char*)u8"冻结通缉", &MenuState::FreezeWantedLevel)) {
                }

                if (ImGui::Checkbox((const char*)u8"住院/被捕保留装备", &MenuState::KeepStuff)) {
                    Controllers::Player::SetKeepStuff(MenuState::KeepStuff);
                }

                bool freeHealth = Controllers::Player::GetFreeHealthcare();
                if (ImGui::Checkbox((const char*)u8"住院不扣钱", &freeHealth)) {
                    Controllers::Player::SetFreeHealthcare(freeHealth);
                }
                ImGui::SameLine();
                bool freeJail = Controllers::Player::GetFreeJail();
                if (ImGui::Checkbox((const char*)u8"被捕不扣钱", &freeJail)) {
                    Controllers::Player::SetFreeJail(freeJail);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextUnformatted((const char*)u8"单项防护");

                GameLogic::ProofState proofs = Controllers::Player::GetProofState();
                MenuState::BulletProof = proofs.bullet;
                MenuState::CollisionProof = proofs.collision;
                MenuState::ExplosionProof = proofs.explosion;
                MenuState::FireProof = proofs.fire;
                MenuState::MeleeProof = proofs.melee;

                ImGui::BeginDisabled(MenuState::GodMode);
                bool changedProof = false;
                changedProof |= ImGui::Checkbox((const char*)u8"防弹", &MenuState::BulletProof);
                ImGui::SameLine();
                changedProof |= ImGui::Checkbox((const char*)u8"防撞", &MenuState::CollisionProof);
                ImGui::SameLine();
                changedProof |= ImGui::Checkbox((const char*)u8"防爆", &MenuState::ExplosionProof);
                changedProof |= ImGui::Checkbox((const char*)u8"防火", &MenuState::FireProof);
                ImGui::SameLine();
                changedProof |= ImGui::Checkbox((const char*)u8"防近战", &MenuState::MeleeProof);
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
                MenuState::PlayerHealth = Controllers::Player::GetHealth();
                ImGui::PushItemWidth(180);
                if (ImGui::InputFloat((const char*)u8"血量", &MenuState::PlayerHealth, 1.0f, 10.0f, "%.1f")) {
                    Controllers::Player::SetHealth(MenuState::PlayerHealth);
                }

                MenuState::PlayerArmour = Controllers::Player::GetArmour();
                if (ImGui::InputFloat((const char*)u8"护甲", &MenuState::PlayerArmour, 1.0f, 10.0f, "%.1f")) {
                    Controllers::Player::SetArmour(MenuState::PlayerArmour);
                }

                MenuState::PlayerMoney = Controllers::Player::GetMoney();
                if (ImGui::InputInt((const char*)u8"现金", &MenuState::PlayerMoney, 1000, 10000)) {
                    Controllers::Player::SetMoney(MenuState::PlayerMoney);
                }

                MenuState::WantedLevel = Controllers::Player::GetWantedLevel();
                if (ImGui::SliderInt((const char*)u8"通缉星级", &MenuState::WantedLevel, 0, 6)) {
                    Controllers::Player::SetWantedLevel(MenuState::WantedLevel);
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button((const char*)u8"消除通缉")) {
                    MenuState::WantedLevel = 0;
                    Controllers::Player::ClearWantedLevel();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}