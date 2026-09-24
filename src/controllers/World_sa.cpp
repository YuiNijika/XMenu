#include "World.h"

namespace {
    const Controllers::World::WeatherEntry kWeatherCatalog[] = {
        {0, "weather.extrasunny_la"},
        {1, "weather.sunny_la"},
        {2, "weather.extrasunny_smog_la"},
        {3, "weather.sunny_smog_la"},
        {4, "weather.cloudy_la"},
        {5, "weather.sunny_sf"},
        {6, "weather.extrasunny_sf"},
        {7, "weather.cloudy_sf"},
        {8, "weather.rainy_sf"},
        {9, "weather.foggy_sf"},
        {10, "weather.sunny_vegas"},
        {11, "weather.extrasunny_vegas"},
        {12, "weather.cloudy_vegas"},
        {13, "weather.extrasunny_countryside"},
        {14, "weather.sunny_countryside"},
        {15, "weather.cloudy_countryside"},
        {16, "weather.rainy_countryside"},
        {17, "weather.extrasunny_desert"},
        {18, "weather.sunny_desert"},
        {19, "weather.sandstorm_desert"},
        {20, "weather.underwater"},
        {21, "weather.extracolours_1"},
        {22, "weather.extracolours_2"},
    };
}

namespace Controllers::World {
    const WeatherEntry* GetWeatherCatalog(int& count) {
        count = static_cast<int>(sizeof(kWeatherCatalog) / sizeof(kWeatherCatalog[0]));
        return kWeatherCatalog;
    }
}
