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
        if (ImGui::Button((const char*)u8"葛洛夫街")) {
            GameLogic::TeleportPlayer(CVector(2493.0f, -1667.0f, 13.34f));
        }
        if (ImGui::Button((const char*)u8"废弃机场")) {
            GameLogic::TeleportPlayer(CVector(424.0f, 2533.0f, 16.6f));
        }
    }
}