#pragma once

namespace XBase::Types {

struct ProofState {
    bool bullet = false;
    bool collision = false;
    bool explosion = false;
    bool fire = false;
    bool melee = false;
    bool nonPlayer = false;
};

struct PedSpawnOptions {
    unsigned int modelId = 7;
    int pedType = 4;
    int gangType = 0;
    bool asGang = false;
    float health = 100.0f;
    float armour = 0.0f;
    bool freeze = false;
    bool hostile = false;
    unsigned int weaponModel = 0;
};

struct SpawnVehicleOptions {
    bool asDriver = true;
    bool aircraftInAir = true;
    bool cleanupPrevious = true;
};

struct PickupOptions {
    unsigned int modelId = 1240;
    unsigned char type = 3;
    unsigned int quantity = 1;
    unsigned int moneyPerDay = 0;
    bool empty = false;
};

struct VehicleAppearanceOptions {
    int primaryColor = 0;
    int secondaryColor = 0;
    int paintjob = -1;
    int modId = 1000;
};

struct PlayerAppearanceOptions {
    int skinModel = 0;
    int clothesTexture = 0;
    int clothesModel = 0;
    int clothesBodyPart = 0;
    int statId = 0;
    float statValue = 0.0f;
};

} // namespace XBase::Types
