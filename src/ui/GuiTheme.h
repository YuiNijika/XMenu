#pragma once
#include "imgui/imgui.h"

namespace GuiTheme {
    enum class ThemeId : int {
        Default = 0,
        DarkDeep = 1,
        Classic = 2,
        Light = 3
    };

    enum class InteractionMode : int {
        Mouse = 0,
        KeyboardMouse = 1
    };

    struct Theme {
        ThemeId id;
        const char* nameKey;
        ImVec4 Text;
        ImVec4 TextDisabled;
        ImVec4 WindowBg;
        ImVec4 ChildBg;
        ImVec4 Header;
        ImVec4 HeaderHovered;
        ImVec4 HeaderActive;
        ImVec4 Button;
        ImVec4 ButtonHovered;
        ImVec4 ButtonActive;
        ImVec4 FrameBg;
        ImVec4 FrameBgHovered;
        ImVec4 FrameBgActive;
        ImVec4 CheckMark;
        ImVec4 SliderGrab;
        ImVec4 SliderGrabActive;
        ImVec4 TabHovered;
        ImVec4 TabActive;
        ImVec4 Separator;
        ImVec4 NavHighlight;
        ImVec4 PopupBg;
        float WindowRounding;
        ImVec2 WindowPadding;
        ImVec2 ItemSpacing;
        float FrameRounding;
        float TabRounding;
    };

    constexpr int ThemeCount = 4;
    constexpr int ColorPushCount = 21;
    constexpr int StyleVarPushCount = 5;
    constexpr int InteractionCount = 2;

    extern const Theme Themes[ThemeCount];

    int GetThemeIndex();
    void SetThemeByIndex(int index);
    const char* GetThemeNameKey(int index);

    int GetInteractionIndex();
    void SetInteractionByIndex(int index);
    const char* GetInteractionNameKey(int index);

    void ApplyTheme();
    void RestoreTheme();
    void ApplyInteraction();
}