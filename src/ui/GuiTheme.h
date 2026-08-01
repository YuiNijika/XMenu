#pragma once
#include <XBase/ValueTypes.h>

namespace GuiTheme {
    using Color = XBase::ColorF;
    using Vec2 = XBase::Vec2;
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
        Color Text;
        Color TextDisabled;
        Color WindowBg;
        Color ChildBg;
        Color Header;
        Color HeaderHovered;
        Color HeaderActive;
        Color Button;
        Color ButtonHovered;
        Color ButtonActive;
        Color FrameBg;
        Color FrameBgHovered;
        Color FrameBgActive;
        Color CheckMark;
        Color SliderGrab;
        Color SliderGrabActive;
        Color TabHovered;
        Color TabActive;
        Color Separator;
        Color NavHighlight;
        Color PopupBg;
        Color Border;
        Color TitleBg;
        Color TitleBgActive;
        Color ScrollbarBg;
        Color ScrollbarGrab;
        float WindowRounding;
        Vec2 WindowPadding;
        Vec2 ItemSpacing;
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

    // 应用到 XBase 渲染主题（需运行时已初始化）
    void ApplyToStyle();

    // 按列表/面板 + 交互模式同步键盘导航与鼠标开关
    void ApplyInteraction();

    // 每帧轻量同步（交互标志）；主题仅在变更时重写 Style
    void Sync();

    // 列表模式且未开鼠标时：隐藏菜单光标
    bool WantsMouseCursor();

    // 兼容：等同 ApplyToStyle / 空操作
    void ApplyTheme();
    void RestoreTheme();
}