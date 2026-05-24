#include "World.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include "CWeather.h"

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }
}

namespace Controllers::World {
    void DrawWeatherButtons() {
        if (ImGui::Button(T("weather.sunny"))) {
            CWeather::OldWeatherType = 1;
            CWeather::NewWeatherType = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("weather.cloudy"))) {
            CWeather::OldWeatherType = 4;
            CWeather::NewWeatherType = 4;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("weather.rainy"))) {
            CWeather::OldWeatherType = 8;
            CWeather::NewWeatherType = 8;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("weather.foggy"))) {
            CWeather::OldWeatherType = 9;
            CWeather::NewWeatherType = 9;
        }
    }
}