#pragma once

#include "ValueTypes.h"

namespace XBase::BulletAssist {

void Init();
void Process();
void Shutdown();
void Draw();
bool ShouldSuppressPedFire(PedId ped);

} // namespace XBase::BulletAssist
