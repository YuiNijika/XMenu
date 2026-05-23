#include "Weapon.h"
#include "controllers/Weapon.h"
#include "ui/MenuState.h"
#include "imgui/imgui.h"

namespace Pages::Weapon {
    void Process() {
        Controllers::Weapon::Process();
    }

    void Draw() {
        if (!Controllers::Weapon::HasPlayer()) {
            ImGui::Text((const char*)u8"玩家还没准备好，稍等进档后再用。");
            return;
        }

        if (ImGui::Button((const char*)u8"拿齐常用武器")) {
            Controllers::Weapon::GiveAll();
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"清空武器")) {
            Controllers::Weapon::ClearAll();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox((const char*)u8"无限弹药", &MenuState::InfiniteAmmo);
    }
}