#pragma once
#include "CVector.h"

namespace Controllers::Teleport {
    CVector GetCurrentPosition();
    void Process();
    void DrawQuickMap();
    void Forward(float distance);
    void To(float x, float y, float z, int interior = 0);
    void MapPosition(float x, float y, bool spawnUnderwater);
    void Center();
    void Marker(bool spawnUnderwater);
}