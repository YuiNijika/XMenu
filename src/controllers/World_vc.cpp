#include "World.h"

namespace {
    const Controllers::World::WeatherEntry kWeatherCatalog[] = {
        {0, "weather.sunny"},
        {1, "weather.cloudy"},
        {2, "weather.rainy"},
        {3, "weather.foggy"},
        {4, "weather.extrasunny"},
        {5, "weather.hurricane"},
        {6, "weather.extracolours"},
    };
}

namespace Controllers::World {
    const WeatherEntry* GetWeatherCatalog(int& count) {
        count = static_cast<int>(sizeof(kWeatherCatalog) / sizeof(kWeatherCatalog[0]));
        return kWeatherCatalog;
    }
}
