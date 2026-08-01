#include "World.h"
#include "utils/I18n.h"
#include <XBase/UI.h>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }
}

namespace Controllers::World {
    void DrawWeatherButtons() {
        if (XBase::UI::Button(T("weather.sunny"))) {
            ForceWeatherNow(0);
        }
        XBase::UI::SameLine();
        if (XBase::UI::Button(T("weather.cloudy"))) {
            ForceWeatherNow(1);
        }
        XBase::UI::SameLine();
        if (XBase::UI::Button(T("weather.rainy"))) {
            ForceWeatherNow(2);
        }
        XBase::UI::SameLine();
        if (XBase::UI::Button(T("weather.foggy"))) {
                        ForceWeatherNow(3);
        }
    }
}