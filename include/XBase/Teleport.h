#pragma once

#include "ValueTypes.h"

namespace XBase::Teleport {

struct MapBounds {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
};

bool TryGetCurrentPosition(Vec3& position);
Vec3 GetCurrentPosition();
bool TryGetMapBounds(MapBounds& bounds);
bool To(float x, float y, float z, int interior = 0);
bool Forward(float distance);
bool MapPosition(float x, float y, bool spawnUnderwater = false);
bool Marker(bool spawnUnderwater = false);
bool Center();
void Process();

} // namespace XBase::Teleport
