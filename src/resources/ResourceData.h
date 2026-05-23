#pragma once
#include <cstddef>

namespace Resources {
    struct WeaponEntry {
        const char* category;
        const char* label;
        unsigned int value;
        bool isModel;
    };

    struct LocationEntry {
        const char* category;
        const char* label;
        int interior;
        float x;
        float y;
        float z;
    };

    struct VehicleEntry {
        const char* category;
        const char* label;
        unsigned int model;
    };

    struct WeaponTable {
        const WeaponEntry* entries;
        std::size_t count;
    };

    struct LocationTable {
        const LocationEntry* entries;
        std::size_t count;
    };

    struct VehicleTable {
        const VehicleEntry* entries;
        std::size_t count;
    };

    WeaponTable GetWeapons();
    LocationTable GetLocations();
    VehicleTable GetVehicles();
    bool IsKnownVehicleModel(unsigned int model);
}