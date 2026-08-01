#pragma once

#include "ValueTypes.h"

namespace XBase::Theme {

enum class Preset {
    Dark,
    Light,
    ClassicBlue,
    Custom
};

struct ColorSet {
    ColorF primary{0.26f, 0.59f, 0.98f, 1.00f};
    ColorF accent{0.95f, 0.37f, 0.22f, 1.00f};
    ColorF background{0.08f, 0.08f, 0.10f, 1.00f};
    ColorF surface{0.14f, 0.14f, 0.17f, 1.00f};
    ColorF text{0.92f, 0.92f, 0.94f, 1.00f};
    ColorF textDisabled{0.45f, 0.45f, 0.50f, 1.00f};
    ColorF border{0.22f, 0.22f, 0.27f, 1.00f};
    ColorF highlight{0.20f, 0.42f, 0.75f, 1.00f};
};

struct Style {
    int id = 0;
    const char* name = "";
    ColorF text;
    ColorF textDisabled;
    ColorF windowBackground;
    ColorF childBackground;
    ColorF header;
    ColorF headerHovered;
    ColorF headerActive;
    ColorF button;
    ColorF buttonHovered;
    ColorF buttonActive;
    ColorF frameBackground;
    ColorF frameBackgroundHovered;
    ColorF frameBackgroundActive;
    ColorF checkMark;
    ColorF sliderGrab;
    ColorF sliderGrabActive;
    ColorF tabHovered;
    ColorF tabActive;
    ColorF separator;
    ColorF navigationHighlight;
    ColorF popupBackground;
    ColorF border;
    ColorF titleBackground;
    ColorF titleBackgroundActive;
    ColorF scrollbarBackground;
    ColorF scrollbarGrab;
    float windowRounding = 0.0f;
    Vec2 windowPadding;
    Vec2 itemSpacing;
    float frameRounding = 0.0f;
    float tabRounding = 0.0f;
};

void Init();
void Shutdown();
void ApplyPreset(Preset preset);
void ApplyCustom(const ColorSet& colors);
void ApplyStyle(const Style& style);
void ConfigureInteraction(bool keyboardNavigation, bool mouseEnabled);
ColorSet GetColors();

FontId LoadFont(const char* path, float size);
bool SetDefaultFont(FontId font);

} // namespace XBase::Theme