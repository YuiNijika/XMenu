#include "Teleport.h"
#include "features/GameLogic.h"
#include "imgui/imgui.h"
#include "CPlayerPed.h"

namespace Controllers::Teleport {
    CVector GetCurrentPosition() {
        CPlayerPed* player = FindPlayerPed();
        return player ? player->GetPosition() : CVector(0.0f, 0.0f, 10.0f);
    }

    void To(float x, float y, float z, int interior) {
        GameLogic::TeleportPlayer(CVector(x, y, z), interior);
    }

    void Forward(float distance) {
        GameLogic::TeleportForward(distance);
    }

    void MapPosition(float x, float y, bool spawnUnderwater) {
        GameLogic::TeleportMapPosition(CVector(x, y, 0.0f), spawnUnderwater);
    }

    void Center() {
        GameLogic::TeleportPlayer(CVector(0.0f, 0.0f, 23.0f));
    }

    void Marker(bool spawnUnderwater) {
        GameLogic::TeleportMarker(spawnUnderwater);
    }

    void DrawLocations() {
        if (ImGui::Button((const char*)u8"斯唐顿岛")) {
            GameLogic::TeleportPlayer(CVector(150.0f, -150.0f, 15.0f));
        }
    }
}