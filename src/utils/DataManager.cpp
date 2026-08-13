#include "DataManager.h"
#include "resources/I18nResources.h"
#include "utils/Log.h"
#include <XBase/Platform.h>

#ifdef GTASA
    #define GAME_DIR "sa"
    #define MAPS_RESOURCE_ID IDR_DATA_SA_MAPS
    #define WEAPONS_RESOURCE_ID IDR_DATA_SA_WEAPONS
    #define VEHICLES_RESOURCE_ID IDR_DATA_SA_VEHICLES
    #define PEDS_RESOURCE_ID IDR_DATA_SA_PEDS
#elif GTAVC
    #define GAME_DIR "vc"
    #define MAPS_RESOURCE_ID IDR_DATA_VC_MAPS
    #define WEAPONS_RESOURCE_ID IDR_DATA_VC_WEAPONS
    #define VEHICLES_RESOURCE_ID IDR_DATA_VC_VEHICLES
    #define PEDS_RESOURCE_ID IDR_DATA_VC_PEDS
#else
    #define GAME_DIR "iii"
    #define MAPS_RESOURCE_ID IDR_DATA_III_MAPS
    #define WEAPONS_RESOURCE_ID IDR_DATA_III_WEAPONS
    #define VEHICLES_RESOURCE_ID IDR_DATA_III_VEHICLES
    #define PEDS_RESOURCE_ID IDR_DATA_III_PEDS
#endif

namespace DataManager {

std::string GetDataFilePath(const std::string& filename) {
    const std::string directory = XBase::Platform::ModuleDirectory("XMenu.asi");
    if (!directory.empty()) {
        return directory + "XMenu/data/" GAME_DIR "/" + filename;
    }

    return std::string("XMenu/data/") + GAME_DIR + "/" + filename;
}

JsonLoader::JsonValue LoadDataJson(const std::string& filename, int resourceId, const char* label) {
    const std::string filepath = GetDataFilePath(filename);
    if (XBase::Platform::FileExists(filepath)) {
        Log::Info(std::string("尝试加载") + label + "数据: " + filepath);
        JsonLoader::JsonValue data = JsonLoader::LoadFromFile(filepath);
        if (data.type == JsonLoader::JsonValue::OBJECT) {
            return data;
        }
        Log::Warn(std::string("从文件系统加载") + label + "数据失败，尝试从DLL资源加载");
    } else {
        Log::Warn(std::string("未找到") + label + "数据文件: " + filepath + "，尝试从DLL资源加载");
    }

    if (resourceId != 0) {
        Log::Info(std::string("尝试从DLL资源加载") + label + "数据，资源ID: " + std::to_string(resourceId));
        JsonLoader::JsonValue data = JsonLoader::LoadFromResource(resourceId);
        if (data.type == JsonLoader::JsonValue::OBJECT) {
            Log::Info(std::string("从DLL资源成功加载") + label + "数据");
            return data;
        }
        Log::Warn(std::string("从DLL资源加载") + label + "数据失败");
    }

    return JsonLoader::JsonValue();
}

std::vector<LocationData> LoadLocations() {
    std::vector<LocationData> locations;
    
    JsonLoader::JsonValue data = LoadDataJson("maps.json", MAPS_RESOURCE_ID, "地图");
    if (data.type != JsonLoader::JsonValue::OBJECT) {
        Log::Warn("地图数据加载失败，使用空数据");
        return locations;
    }
    
    const auto& locationCategories = JsonLoader::GetArray(data, "locations");
    
    for (const auto& category : locationCategories) {
        if (category.type != JsonLoader::JsonValue::OBJECT) {
            continue;
        }
        
        std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
        const auto& entries = JsonLoader::GetArray(category, "entries");
        
        for (const auto& entry : entries) {
            if (entry.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }
            
            LocationData loc;
            loc.category = categoryName;
            loc.name = JsonLoader::GetString(entry, "name", "unknown");
            loc.x = static_cast<float>(JsonLoader::GetNumber(entry, "x", 0.0));
            loc.y = static_cast<float>(JsonLoader::GetNumber(entry, "y", 0.0));
            loc.z = static_cast<float>(JsonLoader::GetNumber(entry, "z", 0.0));
            loc.interior = static_cast<int>(JsonLoader::GetNumber(entry, "interior", 0));
            
            locations.push_back(loc);
        }
    }
    
    Log::Info(std::string("成功加载 ") + std::to_string(locations.size()) + " 个地点");
    return locations;
}

std::vector<WeaponData> LoadWeapons() {
    std::vector<WeaponData> weapons;
    
    JsonLoader::JsonValue data = LoadDataJson("weapons.json", WEAPONS_RESOURCE_ID, "武器");
    if (data.type != JsonLoader::JsonValue::OBJECT) {
        Log::Warn("武器数据加载失败，使用空数据");
        return weapons;
    }
    
    const auto& weaponCategories = JsonLoader::GetArray(data, "weapons");
    
    for (const auto& category : weaponCategories) {
        if (category.type != JsonLoader::JsonValue::OBJECT) {
            continue;
        }
        
        std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
        const auto& entries = JsonLoader::GetArray(category, "entries");
        
        for (const auto& entry : entries) {
            if (entry.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }
            
            WeaponData weapon;
            weapon.category = categoryName;
            weapon.name = JsonLoader::GetString(entry, "name", "unknown");
            weapon.id = static_cast<int>(JsonLoader::GetNumber(entry, "id", 0));
            weapon.isModel = JsonLoader::GetBool(entry, "isModel", false);
            weapon.modelId = static_cast<int>(JsonLoader::GetNumber(entry, "modelId", 0));
            
            weapons.push_back(weapon);
        }
    }
    
    Log::Info(std::string("成功加载 ") + std::to_string(weapons.size()) + " 种武器");
    return weapons;
}

std::vector<VehicleData> LoadVehicles() {
    std::vector<VehicleData> vehicles;
    
    JsonLoader::JsonValue data = LoadDataJson("vehicles.json", VEHICLES_RESOURCE_ID, "车辆");
    if (data.type != JsonLoader::JsonValue::OBJECT) {
        Log::Warn("车辆数据加载失败，使用空数据");
        return vehicles;
    }
    
    const auto& vehicleCategories = JsonLoader::GetArray(data, "vehicles");
    
    for (const auto& category : vehicleCategories) {
        if (category.type != JsonLoader::JsonValue::OBJECT) {
            continue;
        }
        
        std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
        const auto& entries = JsonLoader::GetArray(category, "entries");
        
        for (const auto& entry : entries) {
            if (entry.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }
            
            VehicleData vehicle;
            vehicle.category = categoryName;
            vehicle.name = JsonLoader::GetString(entry, "name", "unknown");
            vehicle.id = static_cast<int>(JsonLoader::GetNumber(entry, "id", 0));
            
            vehicles.push_back(vehicle);
        }
    }
    
    Log::Info(std::string("成功加载 ") + std::to_string(vehicles.size()) + " 辆车辆");
    return vehicles;
}

std::vector<PedData> LoadPeds() {
    std::vector<PedData> peds;

    JsonLoader::JsonValue data = LoadDataJson("peds.json", PEDS_RESOURCE_ID, "行人");
    if (data.type != JsonLoader::JsonValue::OBJECT) {
        Log::Warn("行人数据加载失败，使用空数据");
        return peds;
    }

    const auto& pedCategories = JsonLoader::GetArray(data, "peds");

    for (const auto& category : pedCategories) {
        if (category.type != JsonLoader::JsonValue::OBJECT) {
            continue;
        }

        std::string categoryName = JsonLoader::GetString(category, "category", "unknown");
        const auto& entries = JsonLoader::GetArray(category, "entries");

        for (const auto& entry : entries) {
            if (entry.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }

            PedData ped;
            ped.category = categoryName;
            ped.name = JsonLoader::GetString(entry, "name", "unknown");
            ped.id = static_cast<int>(JsonLoader::GetNumber(entry, "id", 0));

            peds.push_back(ped);
        }
    }

    Log::Info(std::string("成功加载 ") + std::to_string(peds.size()) + " 个行人");
    return peds;
}

} // namespace DataManager
