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
        ImVec4 Border;
        ImVec4 TitleBg;
        ImVec4 TitleBgActive;
        ImVec4 ScrollbarBg;
        ImVec4 ScrollbarGrab;
        float WindowRounding;
        ImVec2 WindowPadding;
        ImVec2 ItemSpacing;
        float FrameRounding;
        float TabRounding;
    };

    constexpr int ThemeCount = 4;
    constexpr int InteractionCount = 2;

    // 兼容旧调用：不再依赖 Push/Pop 配对
    constexpr int ColorPushCount = 0;
    constexpr int StyleVarPushCount = 0;

    extern const Theme Themes[ThemeCount];

    int GetThemeIndex();
    void SetThemeByIndex(int index);
    const char* GetThemeNameKey(int index);
    const Theme& GetCurrentTheme();

    int GetInteractionIndex();
    void SetInteractionByIndex(int index);
    const char* GetInteractionNameKey(int index);

    // 将主题写入全局 ImGuiStyle（需已有 Context）
    void ApplyToStyle();

    // 按列表/面板 + 交互模式同步键盘导航与鼠标开关
    void ApplyInteraction();

    // 每帧轻量同步（交互标志）；主题仅在变更时重写 Style
    void Sync();

    // 列表模式且未开鼠标时：隐藏系统/ImGui 光标
    bool WantsMouseCursor();

    // 兼容：等同 ApplyToStyle / 空操作
    void ApplyTheme();
    void RestoreTheme();
}