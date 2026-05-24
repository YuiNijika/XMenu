#include "ResourceData.h"
#include "utils/DataManager.h"
#include "utils/AppConfig.h"
#include <cctype>
#include <mutex>

namespace Resources {
namespace {
    std::vector<DataManager::LocationData> cachedLocations;
    std::vector<DataManager::WeaponData> cachedWeapons;
    std::vector<DataManager::VehicleData> cachedVehicles;
    bool dataInitialized = false;
    std::mutex dataMutex;

    std::string TrimLocationName(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        return value;
    }
}

void InitData() {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (dataInitialized) return;
    
    cachedLocations = DataManager::LoadLocations();
    const std::vector<DataManager::LocationData>& customLocations = AppConfig::GetCustomLocations();
    cachedLocations.insert(cachedLocations.end(), customLocations.begin(), customLocations.end());
    cachedWeapons = DataManager::LoadWeapons();
    cachedVehicles = DataManager::LoadVehicles();
    dataInitialized = true;
}

WeaponTable GetWeapons() {
    if (!dataInitialized) InitData();
    return {&cachedWeapons, cachedWeapons.size()};
}

LocationTable GetLocations() {
    if (!dataInitialized) InitData();
    return {&cachedLocations, cachedLocations.size()};
}

void ReloadLocations() {
    std::lock_guard<std::mutex> lock(dataMutex);
    cachedLocations = DataManager::LoadLocations();
    const std::vector<DataManager::LocationData>& customLocations = AppConfig::GetCustomLocations();
    cachedLocations.insert(cachedLocations.end(), customLocations.begin(), customLocations.end());
    dataInitialized = true;
}

bool AddCustomLocation(const std::string& name, float x, float y, float z, int interior) {
    const std::string trimmedName = TrimLocationName(name);
    if (!AppConfig::AddCustomLocation(trimmedName, x, y, z, interior)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(dataMutex);
    DataManager::LocationData location;
    location.category = "custom";
    location.name = trimmedName;
    location.x = x;
    location.y = y;
    location.z = z;
    location.interior = interior;
    cachedLocations.push_back(location);
    return true;
}

VehicleTable GetVehicles() {
    if (!dataInitialized) InitData();
    return {&cachedVehicles, cachedVehicles.size()};
}

bool IsKnownVehicleModel(unsigned int model) {
    if (!dataInitialized) InitData();
    for (const auto& v : cachedVehicles) {
        if (static_cast<unsigned int>(v.id) == model) return true;
    }
    return false;
}

} // namespace Resources
