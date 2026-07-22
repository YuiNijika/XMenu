#include "Widget.h"
#include "MenuState.h"
#include "Menu.h"
#include "GuiTheme.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

namespace {
    // GTA V Interactive Menu 尺寸
    constexpr float kListWidth = 432.0f;
    constexpr float kHeaderH = 52.0f;
    constexpr float kFooterH = 30.0f;
    constexpr float kRowH = 38.0f;
    constexpr float kPadX = 14.0f;
    constexpr float kAccentBarW = 4.0f;

    float ContentWidth() {
        return ImGui::GetContentRegionAvail().x;
    }

    float RowWidth() {
        return ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - ImGui::GetCursorStartPos().x;
    }

    ImU32 ColU32(const ImVec4& c) {
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    ImU32 AccentColor() {
        return ColU32(GuiTheme::GetCurrentTheme().CheckMark);
    }

    bool MouseAllowed() {
        return MenuState::ListMenuMouseInput;
    }

    bool KeyDownRepeat(ImGuiKey key) {
        return ImGui::IsKeyPressed(key, true);
    }

    bool KeyDownOnce(ImGuiKey key) {
        return ImGui::IsKeyPressed(key, false);
    }
}

namespace UI {
    int listSelectedIndex = 0;
    int listTotalItems = 0;

    namespace {
        int listCurrentIndex = 0;
        int lastScrolledIndex = -1;
        bool actionConsumed = false;
        bool backConsumed = false;
        bool selectionCanAdjust = false;
        int flashIndex = -1;
        ULONGLONG flashUntil = 0;
        bool shellOpen = false;

        void ClampSelection() {
            if (listTotalItems <= 0) {
                listSelectedIndex = 0;
                return;
            }
            if (listSelectedIndex < 0) {
                listSelectedIndex = listTotalItems - 1;
            }
            if (listSelectedIndex >= listTotalItems) {
                listSelectedIndex = 0;
            }
        }

        void MoveSelection(int delta) {
            if (listTotalItems <= 0) {
                return;
            }
            listSelectedIndex += delta;
            while (listSelectedIndex < 0) {
                listSelectedIndex += listTotalItems;
            }
            while (listSelectedIndex >= listTotalItems) {
                listSelectedIndex -= listTotalItems;
            }
            lastScrolledIndex = -1;
        }

        struct RowResult {
            bool activated = false;
            bool stepLeft = false;
            bool stepRight = false;
        };

        RowResult DrawGtaRow(const char* label, const char* rightText, bool canAdjust) {
            RowResult result{};
            const int index = listCurrentIndex++;
            bool selected = (index == listSelectedIndex);
            const bool flashing = (index == flashIndex && GetTickCount64() < flashUntil);

            if (selected && canAdjust) {
                selectionCanAdjust = true;
            }

            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            const float width = ImGui::GetContentRegionAvail().x;
            const ImVec2 rowMax(rowMin.x + width, rowMin.y + kRowH);

            ImGui::PushID(index);
            ImGui::InvisibleButton("##row", ImVec2(width, kRowH));
            const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            const bool mouseOk = MouseAllowed();
            const bool leftClick = mouseOk && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            // 悬停跟随（仅鼠标移动时，避免键盘滚动被静止光标抢走）
            if (mouseOk && hovered) {
                if (ImGui::GetIO().MouseDelta.x != 0.0f || ImGui::GetIO().MouseDelta.y != 0.0f) {
                    listSelectedIndex = index;
                    selected = true;
                }
            }

            // 右侧状态区域
            ImVec2 rightPos(0, 0);
            ImVec2 rightSize(0, 0);
            if (rightText && rightText[0]) {
                rightSize = ImGui::CalcTextSize(rightText);
                rightPos = ImVec2(rowMax.x - kPadX - rightSize.x, rowMin.y + (kRowH - rightSize.y) * 0.5f);
            }

            if (leftClick) {
                listSelectedIndex = index;
                selected = true;

                if (canAdjust && rightText && rightText[0]) {
                    const ImVec2 mp = ImGui::GetIO().MousePos;
                    const float zoneLeft = rightPos.x - 10.0f;
                    if (mp.x >= zoneLeft) {
                        const float mid = rightPos.x + rightSize.x * 0.5f;
                        if (mp.x < mid) {
                            result.stepLeft = true;
                        } else {
                            result.stepRight = true;
                        }
                    } else {
                        result.activated = true;
                    }
                } else {
                    result.activated = true;
                }
            }

            // 背景：选中白条 / 悬停淡白
            if (selected || flashing) {
                dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, flashing ? 220 : 255));
                dl->AddRectFilled(rowMin, ImVec2(rowMin.x + kAccentBarW, rowMax.y), AccentColor());
            } else if (mouseOk && hovered) {
                dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 28));
            }

            const ImU32 textCol = (selected || flashing)
                ? IM_COL32(0, 0, 0, 255)
                : IM_COL32(255, 255, 255, 230);
            ImU32 rightCol = (selected || flashing)
                ? IM_COL32(0, 0, 0, 255)
                : IM_COL32(255, 255, 255, 160);

            if (!selected && !flashing && rightText) {
                if (std::strcmp(rightText, "ON") == 0) {
                    rightCol = IM_COL32(114, 204, 114, 255);
                } else if (std::strcmp(rightText, "OFF") == 0) {
                    rightCol = IM_COL32(255, 255, 255, 110);
                }
            }

            const ImVec2 labelPos(
                rowMin.x + kPadX + ((selected || flashing) ? 2.0f : 0.0f),
                rowMin.y + (kRowH - ImGui::GetTextLineHeight()) * 0.5f
            );
            dl->AddText(labelPos, textCol, label);

            if (rightText && rightText[0]) {
                dl->AddText(rightPos, rightCol, rightText);
            }

            // 键盘：仅当前选中项
            if (selected && !actionConsumed) {
                if (canAdjust) {
                    if (KeyDownRepeat(ImGuiKey_LeftArrow) || KeyDownRepeat(ImGuiKey_GamepadDpadLeft) || KeyDownRepeat(ImGuiKey_Keypad4)) {
                        result.stepLeft = true;
                        actionConsumed = true;
                    }
                    if (KeyDownRepeat(ImGuiKey_RightArrow) || KeyDownRepeat(ImGuiKey_GamepadDpadRight) || KeyDownRepeat(ImGuiKey_Keypad6)) {
                        result.stepRight = true;
                        actionConsumed = true;
                    }
                } else if (KeyDownOnce(ImGuiKey_LeftArrow) || KeyDownOnce(ImGuiKey_GamepadDpadLeft) || KeyDownOnce(ImGuiKey_Keypad4)) {
                    if (!backConsumed) {
                        Menu::PopPage();
                        backConsumed = true;
                        actionConsumed = true;
                    }
                }

                if (KeyDownOnce(ImGuiKey_Enter)
                    || KeyDownOnce(ImGuiKey_KeypadEnter)
                    || KeyDownOnce(ImGuiKey_GamepadFaceDown)
                    || KeyDownOnce(ImGuiKey_Keypad5)
                    || KeyDownOnce(ImGuiKey_Space)) {
                    result.activated = true;
                    actionConsumed = true;
                }
            }

            if (result.activated || result.stepLeft || result.stepRight) {
                flashIndex = index;
                flashUntil = GetTickCount64() + 120;
            }

            if (selected && lastScrolledIndex != listSelectedIndex) {
                ImGui::SetScrollHereY(0.35f);
                lastScrolledIndex = listSelectedIndex;
            }

            ImGui::PopID();
            return result;
        }
    }

    void ResetListIndex() {
        listSelectedIndex = 0;
        lastScrolledIndex = -1;
        flashIndex = -1;
    }

    void SetListTotalItems() {
        listTotalItems = listCurrentIndex;
        ClampSelection();
    }

    bool ListSelectionCanAdjust() {
        return selectionCanAdjust;
    }

    void UpdateListNavigation() {
        if (!MenuState::UseNativeMenu) {
            return;
        }

        listCurrentIndex = 0;
        actionConsumed = false;
        backConsumed = false;
        selectionCanAdjust = false;

        if (listTotalItems > 0) {
            if (KeyDownRepeat(ImGuiKey_UpArrow) || KeyDownRepeat(ImGuiKey_GamepadDpadUp) || KeyDownRepeat(ImGuiKey_Keypad8)) {
                MoveSelection(-1);
            }
            if (KeyDownRepeat(ImGuiKey_DownArrow) || KeyDownRepeat(ImGuiKey_GamepadDpadDown) || KeyDownRepeat(ImGuiKey_Keypad2)) {
                MoveSelection(1);
            }

            if (MouseAllowed()) {
                const float wheel = ImGui::GetIO().MouseWheel;
                if (wheel > 0.0f) {
                    MoveSelection(-1);
                } else if (wheel < 0.0f) {
                    MoveSelection(1);
                }
            }
        }

        const bool wantBack =
            KeyDownOnce(ImGuiKey_Backspace)
            || KeyDownOnce(ImGuiKey_Escape)
            || KeyDownOnce(ImGuiKey_GamepadFaceRight)
            || KeyDownOnce(ImGuiKey_Keypad0)
            || (MouseAllowed() && ImGui::IsMouseClicked(ImGuiMouseButton_Right));

        if (wantBack && !backConsumed) {
            Menu::PopPage();
            backConsumed = true;
        }
    }

    void BeginListShell(const char* title, const char* subtitle, bool showBackHint) {
        (void)showBackHint;
        shellOpen = false;
        if (!MenuState::UseNativeMenu) {
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(40.0f, 80.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kListWidth, 560.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(kListWidth, 320.0f), ImVec2(kListWidth, 900.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.82f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse;

        bool open = true;
        if (!ImGui::Begin("XMenu_ListGTA", &open, flags)) {
            ImGui::End();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(5);
            return;
        }
        shellOpen = true;

        // 顶栏拖拽移动（无原生标题栏）
        {
            const ImVec2 wp = ImGui::GetWindowPos();
            const float ww = ImGui::GetWindowWidth();
            if (MouseAllowed()
                && ImGui::IsMouseHoveringRect(wp, ImVec2(wp.x + ww, wp.y + kHeaderH))
                && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                const ImVec2 delta = ImGui::GetIO().MouseDelta;
                ImGui::SetWindowPos(ImVec2(wp.x + delta.x, wp.y + delta.y));
            }
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        const float ww = ImGui::GetWindowWidth();
        const float wh = ImGui::GetWindowHeight();

        dl->AddRectFilled(wp, ImVec2(wp.x + ww, wp.y + kHeaderH), IM_COL32(0, 0, 0, 245));
        dl->AddRectFilled(ImVec2(wp.x, wp.y + kHeaderH - 3.0f), ImVec2(wp.x + ww, wp.y + kHeaderH), AccentColor());

        const float titleY = wp.y + 10.0f;
        dl->AddText(ImVec2(wp.x + kPadX, titleY), IM_COL32(255, 255, 255, 255), title ? title : "XMenu");
        if (subtitle && subtitle[0]) {
            dl->AddText(
                ImVec2(wp.x + kPadX, titleY + ImGui::GetTextLineHeight() + 2.0f),
                IM_COL32(255, 255, 255, 150),
                subtitle
            );
        }

        ImGui::SetCursorPos(ImVec2(0.0f, kHeaderH));
        const float contentH = wh - kHeaderH - kFooterH;
        ImGui::BeginChild(
            "##ListBody",
            ImVec2(ww, contentH),
            false,
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysVerticalScrollbar
        );

        {
            const ImVec2 c0 = ImGui::GetWindowPos();
            const ImVec2 c1(c0.x + ImGui::GetWindowWidth(), c0.y + ImGui::GetWindowHeight());
            ImGui::GetWindowDrawList()->AddRectFilled(c0, c1, IM_COL32(10, 10, 12, 180));
        }

        UpdateListNavigation();
    }

    void EndListShell() {
        if (!MenuState::UseNativeMenu || !shellOpen) {
            return;
        }

        SetListTotalItems();
        ImGui::EndChild();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        const float ww = ImGui::GetWindowWidth();
        const float wh = ImGui::GetWindowHeight();
        const float footerTop = wp.y + wh - kFooterH;

        char pageBuf[32];
        std::snprintf(
            pageBuf,
            sizeof(pageBuf),
            "%d / %d",
            listTotalItems > 0 ? listSelectedIndex + 1 : 0,
            listTotalItems > 0 ? listTotalItems : 0
        );
        const ImVec2 psz = ImGui::CalcTextSize(pageBuf);
        dl->AddRectFilled(
            ImVec2(wp.x + ww - kPadX - psz.x - 6.0f, wp.y + 10.0f),
            ImVec2(wp.x + ww - 4.0f, wp.y + 10.0f + psz.y + 6.0f),
            IM_COL32(0, 0, 0, 245)
        );
        dl->AddText(ImVec2(wp.x + ww - kPadX - psz.x, wp.y + 14.0f), IM_COL32(255, 255, 255, 180), pageBuf);

        dl->AddRectFilled(ImVec2(wp.x, footerTop), ImVec2(wp.x + ww, wp.y + wh), IM_COL32(0, 0, 0, 240));
        dl->AddRectFilled(ImVec2(wp.x, footerTop), ImVec2(wp.x + ww, footerTop + 2.0f), AccentColor());

        const char* hint = MouseAllowed()
            ? (selectionCanAdjust
                ? "LMB confirm/adjust   Wheel move   RMB back"
                : "LMB select/confirm   Wheel move   RMB back")
            : (selectionCanAdjust
                ? "Enter confirm  Left/Right adjust  Back return"
                : "Enter confirm  Up/Down select  Back return");
        dl->AddText(
            ImVec2(wp.x + kPadX, footerTop + (kFooterH - ImGui::GetTextLineHeight()) * 0.5f),
            IM_COL32(255, 255, 255, 140),
            hint
        );

        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(5);
        shellOpen = false;
    }

    ImVec2 CalcSize(short count, bool spacing) {
        if (count <= 1) {
            return ImVec2(ContentWidth(), ImGui::GetFrameHeight() * 1.3f);
        }
        const float rowWidth = RowWidth();
        const float spacingWidth = spacing ? ImGui::GetStyle().ItemSpacing.x * (count - 1) : 0.0f;
        return ImVec2((rowWidth - spacingWidth) / count, ImGui::GetFrameHeight() * 1.3f);
    }

    void SameLineEvery(int index, int columns) {
        if (MenuState::UseNativeMenu) return;
        if (columns > 1 && (index + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }

    void SpacingSeparator() {
        if (MenuState::UseNativeMenu) return;
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void TextCentered(const char* text) {
        if (MenuState::UseNativeMenu) return;
        const ImVec2 size = ImGui::CalcTextSize(text);
        ImGui::NewLine();
        ImGui::SameLine((ContentWidth() - size.x) / 2.0f);
        ImGui::TextUnformatted(text);
    }

    void BeginPage(const char* id, const char* title, const char* subtitle) {
        (void)id;
        (void)title;
        (void)subtitle;
    }

    void EndPage() {}

    bool BeginTab(const char* id, const char* label) {
        if (MenuState::UseNativeMenu) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const float w = ImGui::GetContentRegionAvail().x;
            const float h = 26.0f;
            dl->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + h), IM_COL32(0, 0, 0, 90));
            dl->AddText(
                ImVec2(p0.x + kPadX, p0.y + (h - ImGui::GetTextLineHeight()) * 0.5f),
                AccentColor(),
                label
            );
            ImGui::Dummy(ImVec2(w, h));
            ImGui::PushID(id);
            return true;
        }
        return ImGui::BeginTabItem(label);
    }

    void EndTab() {
        if (MenuState::UseNativeMenu) {
            ImGui::PopID();
        } else {
            ImGui::EndTabItem();
        }
    }

    bool BeginTabBar(const char* id) {
        if (MenuState::UseNativeMenu) {
            ImGui::PushID(id);
            return true;
        }
        return ImGui::BeginTabBar(id, ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll);
    }

    void EndTabBar() {
        if (MenuState::UseNativeMenu) {
            ImGui::PopID();
        } else {
            ImGui::EndTabBar();
        }
    }

    bool CollapsingHeader(const char* label, bool& isOpen) {
        if (MenuState::UseNativeMenu) {
            const RowResult r = DrawGtaRow(label, isOpen ? "v" : ">", false);
            if (r.activated) {
                isOpen = !isOpen;
            }
            return isOpen;
        }
        ImGui::SetNextItemOpen(isOpen, ImGuiCond_Always);
        ImGui::CollapsingHeader(label);
        if (ImGui::IsItemClicked()) {
            isOpen = !isOpen;
        }
        return isOpen;
    }

    bool Checkbox(const char* label, bool* v) {
        if (MenuState::UseNativeMenu) {
            const RowResult r = DrawGtaRow(label, *v ? "ON" : "OFF", false);
            if (r.activated) {
                *v = !*v;
                return true;
            }
            return false;
        }
        return ImGui::Checkbox(label, v);
    }

    bool Button(const char* label, short columns) {
        if (MenuState::UseNativeMenu) {
            (void)columns;
            return DrawGtaRow(label, "", false).activated;
        }
        return ImGui::Button(label, CalcSize(columns));
    }

    bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
        if (MenuState::UseNativeMenu) {
            char num[32];
            std::snprintf(num, sizeof(num), format, *v);
            char right[48];
            std::snprintf(right, sizeof(right), "< %s >", num);

            const RowResult r = DrawGtaRow(label, right, true);
            bool changed = false;
            const float step = (v_max - v_min) > 100.0f ? 1.0f : 0.1f;
            if (r.stepLeft) {
                *v -= step;
                changed = true;
            }
            if (r.stepRight) {
                *v += step;
                changed = true;
            }
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            return changed || r.activated;
        }
        return ImGui::SliderFloat(label, v, v_min, v_max, format);
    }

    bool SliderInt(const char* label, int* v, int v_min, int v_max) {
        if (MenuState::UseNativeMenu) {
            char right[32];
            std::snprintf(right, sizeof(right), "< %d >", *v);
            const RowResult r = DrawGtaRow(label, right, true);
            bool changed = false;
            if (r.stepLeft) {
                *v -= 1;
                changed = true;
            }
            if (r.stepRight) {
                *v += 1;
                changed = true;
            }
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            return changed || r.activated;
        }
        return ImGui::SliderInt(label, v, v_min, v_max);
    }

    bool InputFloat(const char* label, float* v, float step, float step_fast, const char* format) {
        if (MenuState::UseNativeMenu) {
            (void)step;
            (void)step_fast;
            return SliderFloat(label, v, -10000.0f, 10000.0f, format);
        }
        return ImGui::InputFloat(label, v, step, step_fast, format);
    }

    bool InputInt(const char* label, int* v, int step, int step_fast) {
        if (MenuState::UseNativeMenu) {
            char right[32];
            std::snprintf(right, sizeof(right), "< %d >", *v);
            const RowResult r = DrawGtaRow(label, right, true);
            bool changed = false;
            if (r.stepLeft) {
                *v -= step;
                changed = true;
            }
            if (r.stepRight) {
                *v += step;
                changed = true;
            }
            if (r.activated) {
                ImGui::OpenPopup("##ListInputInt");
            }
            if (ImGui::BeginPopup("##ListInputInt")) {
                ImGui::SetKeyboardFocusHere();
                ImGui::InputInt("##v", v, step, step_fast);
                if (KeyDownOnce(ImGuiKey_Enter) || KeyDownOnce(ImGuiKey_KeypadEnter)) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            return changed || r.activated;
        }
        return ImGui::InputInt(label, v, step, step_fast);
    }

    void SameLine() { if (!MenuState::UseNativeMenu) ImGui::SameLine(); }
    void Columns(int count, const char* id, bool border) { if (!MenuState::UseNativeMenu) ImGui::Columns(count, id, border); }
    void NextColumn() { if (!MenuState::UseNativeMenu) ImGui::NextColumn(); }
    void BeginDisabled(bool disabled) { ImGui::BeginDisabled(disabled); }
    void EndDisabled() { ImGui::EndDisabled(); }
    void PushItemWidth(float item_width) { if (!MenuState::UseNativeMenu) ImGui::PushItemWidth(item_width); }
    void PopItemWidth() { if (!MenuState::UseNativeMenu) ImGui::PopItemWidth(); }
}