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
        if (ImGui::Button((const char*)u8"维赛迪豪宅")) {
            GameLogic::TeleportPlayer(CVector(-379.0f, -535.0f, 17.28f));
        }
        if (ImGui::Button((const char*)u8"阳光车行")) {
            GameLogic::TeleportPlayer(CVector(-883.0f, -1335.0f, 12.0f));
        }
        if (ImGui::Button((const char*)u8"马里布俱乐部")) {
            GameLogic::TeleportPlayer(CVector(490.0f, -79.0f, 10.0f));
        }
    }
}