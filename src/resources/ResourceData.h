#pragma once
#include <cstddef>
#include <vector>
#include "utils/DataManager.h"
#include <string>

namespace Resources {
    // 使用DataManager的结构
    using LocationEntry = DataManager::LocationData;
    using WeaponEntry = DataManager::WeaponData;
    using VehicleEntry = DataManager::VehicleData;
    
    struct WeaponTable {
        const std::vector<WeaponEntry>* entries;
        std::size_t count;
    };

    struct LocationTable {
        const std::vector<LocationEntry>* entries;
        std::size_t count;
    };

    struct VehicleTable {
        const std::vector<VehicleEntry>* entries;
        std::size_t count;
    };

    WeaponTable GetWeapons();
    LocationTable GetLocations();
    void ReloadLocations();
    bool AddCustomLocation(const std::string& name, float x, float y, float z, int interior);
    VehicleTable GetVehicles();
    bool IsKnownVehicleModel(unsigned int model);
    
    // 初始化数据
    void InitData();
}