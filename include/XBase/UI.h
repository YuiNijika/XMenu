#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <string>

#include "ValueTypes.h"

namespace XBase::UI {

using DrawFn = std::function<void()>;

enum class WindowFlag : unsigned int {
    None = 0,
    NoTitleBar = 1u << 0,
    NoResize = 1u << 1,
    NoMove = 1u << 2,
    NoCollapse = 1u << 3,
    NoScrollbar = 1u << 4,
    NoBackground = 1u << 5,
    AlwaysAutoResize = 1u << 6,
    NoSavedSettings = 1u << 7,
    NoFocusOnAppearing = 1u << 8,
    NoNavigation = 1u << 9,
};

enum class MouseButton {
    Left,
    Right,
    Middle,
};

using WindowFlags = unsigned int;

constexpr WindowFlags Flag(WindowFlag flag) {
    return static_cast<WindowFlags>(flag);
}

void Init(const std::string& title = "XBase");
void Process();
void Shutdown();

void AddWindow(const std::string& name, DrawFn drawFn, bool defaultOpen = false, WindowFlags flags = 0);
void RemoveWindow(const std::string& name);
void SetWindowVisible(const std::string& name, bool visible);
bool IsWindowVisible(const std::string& name);
void ToggleWindow(const std::string& name);

void SetNextWindowPosition(Vec2 position, bool always = false);
void SetNextWindowSize(Vec2 size, bool firstUseOnly = false);
void SetNextWindowBackgroundAlpha(float alpha);

void Window(const char* id, const char* title, const DrawFn& drawFn, bool* open = nullptr, WindowFlags flags = 0);
void Child(const char* id, const DrawFn& drawFn, Vec2 size = {}, bool border = false);
void Disabled(bool disabled, const DrawFn& drawFn);
void Indented(const DrawFn& drawFn, float width = 0.0f);
void Group(const char* label, const DrawFn& drawFn, Vec2 size = {});
void Tree(const char* label, const DrawFn& drawFn, bool defaultOpen = false);
void OpenModal(const char* id);
void Modal(const char* id, const DrawFn& drawFn, Vec2 size = {}, bool autoResize = false);
void CloseModal();

struct TableColumn {
    const char* label = "";
    float weight = 1.0f;
};

void Table(const char* id, const TableColumn* columns, std::size_t columnCount, const DrawFn& drawFn, Vec2 size = {});
void TableNextRow();
void TableNextCell();

struct MenuSurfaceState {
    int selectedIndex = 0;
    int itemCount = 0;
};

struct MenuSurfaceResult {
    bool backRequested = false;
    bool selectionCanAdjust = false;
};

// Draws a complete controller-friendly list menu. Navigation, scrolling,
// pointer hit-testing and visual feedback remain owned by XBase. The host owns
// only the durable selection state and decides how to handle a back request.
MenuSurfaceResult MenuSurface(
    const char* id,
    const char* title,
    const char* subtitle,
    MenuSurfaceState& state,
    bool allowMouse,
    bool showBackHint,
    const DrawFn& drawFn);
bool MenuSurfaceButton(const char* label);
bool MenuSurfaceCheckbox(const char* label, bool& value);
bool MenuSurfaceCollapsingSection(const char* label, bool& open);
bool MenuSurfaceSlider(const char* label, float& value, float minValue, float maxValue, const char* format = "%.1f");
bool MenuSurfaceSlider(const char* label, int& value, int minValue, int maxValue);
bool MenuSurfaceInput(const char* label, float& value, float step = 1.0f, float fastStep = 10.0f, const char* format = "%.1f");
bool MenuSurfaceInput(const char* label, int& value, int step = 1, int fastStep = 10);
void MenuSurfaceSection(const char* label);

void Tabs(const char* id, const DrawFn& drawFn);
void Tab(const char* id, const char* label, const DrawFn& drawFn);

Vec2 GridItemSize(int columns = 1, bool includeSpacing = true);
bool CollapsingSection(const char* label, bool& open);

void Text(const char* text);
void Text(const char* format, float value);
void Text(const char* format, int value);
void Text(const char* format, int first, int second);
void Text(const char* format, const char* value);
void Text(const char* format, const char* first, const char* second);
void Text(const char* format, float first, float second, float third);
void Text(const char* format, float first, float second, int third, int fourth);
void TextWrapped(const char* text);
void TextWrapped(const char* format, const char* value);
void TextWrapped(const char* format, const char* first, const char* second);
void TextDisabled(const char* text);
void TextDisabled(const char* format, int value);
void TextDisabled(const char* format, int first, int second);
void TextDisabled(const char* format, const char* value);
void TextDisabled(const char* format, const char* first, const char* second);
void TextDisabled(const char* format, int first, int second, int third);
void Spacing();
float GetFrameRate();
void CenterText(const char* text);
void Separator();
void SeparatorText(const char* label);
void Tooltip(const char* text);
void HelpMarker(const char* desc, bool* hold = nullptr);
bool Button(const char* label, Vec2 size = {});
bool StyledButton(const char* label, Vec2 size = {});
void BeginGroupBox(const char* label, Vec2 size = {});
void EndGroupBox();
bool Checkbox(const char* label, bool& value);
bool Choice(const char* label, int& selectedValue, int value);
bool Slider(const char* label, float& value, float minValue, float maxValue, const char* format = "%.1f");
bool Slider(const char* label, int& value, int minValue, int maxValue);
bool Input(const char* label, float& value, float step = 1.0f, float fastStep = 10.0f, const char* format = "%.1f");
bool Input(const char* label, int& value, int step = 1, int fastStep = 10);
bool InputText(const char* label, char* value, std::size_t capacity, const char* hint = nullptr, bool readOnly = false, bool submitOnEnter = false);
bool InputTextMultiline(const char* label, char* value, std::size_t capacity, Vec2 size = {}, bool readOnly = false);
void SetClipboardText(const char* text);
bool Selectable(const char* label, bool selected = false, Vec2 size = {});
void Combo(const char* label, const char* preview, const DrawFn& drawFn);
void FocusLastItemByDefault();
bool MenuItem(const char* label, bool selected = false, bool enabled = true);
bool CollapsingHeader(const char* label, bool defaultOpen = false);
bool InvisibleButton(const char* id, Vec2 size);
bool IsLastItemHovered();
bool IsMouseDown(MouseButton button);
Vec2 GetMousePosition();
Vec2 GetCursorScreenPosition();
Vec2 GetContentAvailable();

namespace Canvas {
void Line(Vec2 from, Vec2 to, Color color, float thickness = 1.0f);
void Rect(Vec2 min, Vec2 max, Color color, float thickness = 1.0f);
void RectFilled(Vec2 min, Vec2 max, Color color);
void Circle(Vec2 center, float radius, Color color, float thickness = 1.0f);
void CircleFilled(Vec2 center, float radius, Color color);
void Text(Vec2 position, Color color, const char* text);

void Arc(Vec2 center, float radius, float startAngle, float endAngle, Color color, float thickness = 1.0f);
void Polyline(const Vec2* points, std::size_t count, Color color, float thickness = 1.0f);
}

struct NotificationSpec {
    const char* message = "";
    float duration = 3.0f;
    Color color = {};
};

void Notify(NotificationSpec spec);
void RenderNotifications(Vec2 screenPosition = {});
void SameLine();
void Columns(int count, const char* id = nullptr, bool border = false);
void NextColumn();
void PushItemWidth(float width);
void PopItemWidth();

void BeginTabBar(const std::string& name);
void AddTab(const std::string& label, DrawFn drawFn);
bool RenderTabBar(float height = 0.0f);
void EndTabBar();

} // namespace XBase::UI