#include "Teleport.h"
#include "features/GameLogic.h"
#include "imgui/imgui.h"

namespace Controllers::Teleport {
    void Forward(float distance) {
        GameLogic::TeleportForward(distance);
    }

    void DrawLocations() {
        if (ImGui::Button((const char*)u8"斯唐顿岛")) {
            GameLogic::TeleportPlayer(CVector(150.0f, -150.0f, 15.0f));
        }
    }
}