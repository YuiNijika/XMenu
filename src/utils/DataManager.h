#pragma once
#include <string>
#include <vector>
#include "JsonLoader.h"

namespace DataManager {
    struct LocationData {
        std::string category;  // i18n key
        std::string name;      // i18n key
        float x, y, z;
        int interior;
    };
    
    struct WeaponData {
        std::string category;  // i18n key
        std::string name;      // i18n key
        int id;
        bool isModel;
        int modelId;
    };
    
    struct VehicleData {
        std::string category;  // i18n key
        std::string name;      // i18n key
        int id;
    };
    
    // 加载当前游戏的地图数据
    std::vector<LocationData> LoadLocations();
    
    // 加载当前游戏的武器数据
    std::vector<WeaponData> LoadWeapons();
    
    // 加载当前游戏的车辆数据
    std::vector<VehicleData> LoadVehicles();
    
    // 获取数据文件路径
    std::string GetDataFilePath(const std::string& filename);
}
