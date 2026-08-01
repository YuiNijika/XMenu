#pragma once

#include "ValueTypes.h"

namespace XBase::Teleport {

bool TryGetCurrentPosition(Vec3& position);
Vec3 GetCurrentPosition();
bool To(float x, float y, float z, int interior = 0);
bool Forward(float distance);
bool MapPosition(float x, float y, bool spawnUnderwater = false);
bool Marker(bool spawnUnderwater = false);
bool Center();
void Process();

} // namespace XBase::Teleport
