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
            CWeather::ForceWeatherNow(0);
        }
        ImGui::SameLine();
        if (ImGui::Button(T("weather.cloudy"))) {
            CWeather::ForceWeatherNow(1);
        }
        ImGui::SameLine();
        if (ImGui::Button(T("weather.rainy"))) {
            CWeather::ForceWeatherNow(2);
        }
        ImGui::SameLine();
        if (ImGui::Button(T("weather.foggy"))) {
            CWeather::ForceWeatherNow(3);
        }
    }
}