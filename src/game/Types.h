#pragma once

namespace GameTypes {
    struct Vec3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct ProofState {
        bool bullet = false;
        bool collision = false;
        bool explosion = false;
        bool fire = false;
        bool melee = false;
        bool nonPlayer = false;
    };

    struct PlayerState {
        bool available = false;
        bool dead = false;
        float health = 0.0f;
        float armour = 0.0f;
        int wantedLevel = 0;
        int money = 0;
        Vec3 position;
        ProofState proofs;
    };

    struct VehicleState {
        bool available = false;
        float health = 0.0f;
        bool engine = false;
        bool locked = false;
        bool lights = false;
        bool visible = true;
    };

    struct SpawnVehicleOptions {
        bool asDriver = true;
        bool aircraftInAir = true;
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
}