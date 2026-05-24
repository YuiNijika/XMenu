#include "Teleport.h"
#include "features/GameLogic.h"
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
}