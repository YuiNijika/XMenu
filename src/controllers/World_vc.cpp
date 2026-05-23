#include "World.h"
#include "imgui/imgui.h"
#include "CWeather.h"

namespace Controllers::World {
    void DrawWeatherButtons() {
        if (ImGui::Button((const char*)u8"晴天")) {
            CWeather::ForceWeatherNow(0);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"多云")) {
            CWeather::ForceWeatherNow(1);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"下雨")) {
            CWeather::ForceWeatherNow(2);
        }
        ImGui::SameLine();
        if (ImGui::Button((const char*)u8"起雾")) {
            CWeather::ForceWeatherNow(3);
        }
    }
}