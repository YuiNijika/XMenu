#include "Teleport.h"
#include "features/GameLogic.h"
#include "imgui/imgui.h"

namespace Controllers::Teleport {
    void Forward(float distance) {
        GameLogic::TeleportForward(distance);
    }

    void DrawLocations() {
        if (ImGui::Button((const char*)u8"葛洛夫街")) {
            GameLogic::TeleportPlayer(CVector(2493.0f, -1667.0f, 13.34f));
        }
        if (ImGui::Button((const char*)u8"废弃机场")) {
            GameLogic::TeleportPlayer(CVector(424.0f, 2533.0f, 16.6f));
        }
    }
}