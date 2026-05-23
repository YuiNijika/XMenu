#include "World.h"
#include "imgui/imgui.h"
#include "CWeather.h"

namespace Controllers::World {
    void DrawWeatherButtons() {
        if (ImGui::Button((const char*)u8"晴天")) {
            CWeather::OldWeatherType = 1;
            CWeather::NewWeatherType = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"多云")) {
            CWeather::OldWeatherType = 4;
            CWeather::NewWeatherType = 4;
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"下雨")) {
            CWeather::OldWeatherType = 8;
            CWeather::NewWeatherType = 8;
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"起雾")) {
            CWeather::OldWeatherType = 9;
            CWeather::NewWeatherType = 9;
        }
    }
}