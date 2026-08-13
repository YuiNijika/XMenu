#pragma once

#include "ValueTypes.h"

namespace XBase::BulletAssist {

enum class AimPart {
    Head,
    Chest,
    Abdomen,
    Legs,
};

struct Config {
    bool tracking = false;
    bool throughWalls = false;
    bool hardLock = false;
    bool trackCivilian = true;
    bool trackFriend = false;
    bool trackHostile = true;
    bool trackNeutral = true;
    AimPart aimPart = AimPart::Chest;
    float lockRange = 100.0f;
    int maxTargets = 4;
    bool drawPedBounds = false;
    bool drawPedCollision = false;
    bool drawPedSkeleton = false;
    bool drawVehicleBounds = false;
    bool drawVehicleCollision = false;
};

void SetConfig(const Config& config);
Config GetConfig();
void Init();
bool IsInitialized();
void Process();
void Shutdown();
void Draw();
bool ShouldSuppressPedFire(PedId ped);

} // namespace XBase::BulletAssist
