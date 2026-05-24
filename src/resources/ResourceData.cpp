#include "ResourceData.h"
#include "utils/DataManager.h"
#include <mutex>

namespace Resources {
namespace {
    std::vector<DataManager::LocationData> cachedLocations;
    std::vector<DataManager::WeaponData> cachedWeapons;
    std::vector<DataManager::VehicleData> cachedVehicles;
    bool dataInitialized = false;
    std::mutex dataMutex;
}

void InitData() {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (dataInitialized) return;
    
    cachedLocations = DataManager::LoadLocations();
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
