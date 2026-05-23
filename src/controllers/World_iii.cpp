#include "World.h"
#include "imgui/imgui.h"
#include "CWeather.h"

namespace Controllers::World {
    void DrawWeatherButtons() {
        if (ImGui::Button((const char*)u8"晴天")) {
            CWeather::ForceWeatherNow(WEATHER_SUNNY);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"多云")) {
            CWeather::ForceWeatherNow(WEATHER_CLOUDY);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"下雨")) {
            CWeather::ForceWeatherNow(WEATHER_RAINY);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"起雾")) {
            CWeather::ForceWeatherNow(WEATHER_FOGGY);
        }
    }
}