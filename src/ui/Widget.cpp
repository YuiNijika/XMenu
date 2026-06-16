#include "Widget.h"
#include "MenuState.h"
#include "Menu.h"
#include <windows.h>

namespace {
    float ContentWidth() {
        return ImGui::GetContentRegionAvail().x;
    }

    float RowWidth() {
        return ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - ImGui::GetCursorStartPos().x;
    }
}

namespace UI {
    int listCurrentIndex = 0;
    int listSelectedIndex = 0;
    int listTotalItems = 0;
    static int lastScrolledIndex = -1;
    static bool actionExecutedThisFrame = false;
    static int flashIndex = -1;
    static ULONGLONG flashTick = 0;

    void ResetListIndex() {
        listSelectedIndex = 0;
        lastScrolledIndex = -1;
    }

    void SetListTotalItems() {
        listTotalItems = listCurrentIndex;
    }

    void UpdateListNavigation() {
        if (!MenuState::UseNativeMenu) return;
        
        // Reset per-frame rendering counter
        listCurrentIndex = 0;
        actionExecutedThisFrame = false;

        if (listTotalItems > 0) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel > 0.0f) {
                listSelectedIndex--;
                if (listSelectedIndex < 0) listSelectedIndex = listTotalItems - 1;
            } else if (wheel < 0.0f) {
                listSelectedIndex++;
                if (listSelectedIndex >= listTotalItems) listSelectedIndex = 0;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad8, true)) {
                listSelectedIndex--;
                if (listSelectedIndex < 0) listSelectedIndex = listTotalItems - 1;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad2, true)) {
                listSelectedIndex++;
                if (listSelectedIndex >= listTotalItems) listSelectedIndex = 0;
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false) || ImGui::IsKeyPressed(ImGuiKey_Keypad0, false) || ImGui::IsMouseClicked(1)) {
            Menu::PopPage();
        } else if (listTotalItems > 0 && 
            (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, false) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, false))) {
            
            // Check if the current selected item is NOT a Slider or something that uses left/right arrows
            // We can determine this in DrawListItemBase, but for a global PopPage we can just track if outChangedLeftRight was true.
            // Since UpdateListNavigation runs before the items are drawn, we can't easily know if the CURRENT item accepts left/right.
            // So we delay PopPage to after items are drawn? No, PopPage changes activePage, doing it after drawing might cause issues.
            // Instead, we let DrawListItemBase trigger a PopPage if left arrow is pressed and it's not a value slider.
        }
    }

    bool DrawListItemBase(const char* label, const std::string& rightText, bool& outChangedLeftRight, bool canChangeValue = false) {
        bool isSelected = (listCurrentIndex == listSelectedIndex);
        outChangedLeftRight = false;
        
        if (isSelected) {
            ImVec4 headerColor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive);
            float luminance = (headerColor.x * 0.299f + headerColor.y * 0.587f + headerColor.z * 0.114f);
            if (luminance > 0.5f) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            }
            
            // Allow left/right navigation for values.
            // If canChangeValue is false, the left arrow in UpdateListNavigation will trigger PopPage.
            if (canChangeValue) {
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, true)) {
                    outChangedLeftRight = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad6, true)) {
                    outChangedLeftRight = true;
                }
            } else {
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, false) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, false)) {
                    // Schedule a pop page for next frame to avoid drawing state issues, or we can just pop it directly.
                    Menu::PopPage();
                }
            }
        }
        
        // Use ImGuiSelectableFlags_SpanAllColumns for full width, and add some left padding to text
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
        
        bool isFlashing = (flashIndex == listCurrentIndex && GetTickCount64() < flashTick);
        
        bool isHovered = false;
        if (!isSelected && !isFlashing) {
            // For non-selected items, make them light up slightly when hovered
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing());
            
            // Allow hover effect ONLY if the mouse is actively moving or has moved this frame
            // AND it's actually hovering over the item bounds
            if ((ImGui::GetIO().MouseDelta.x != 0.0f || ImGui::GetIO().MouseDelta.y != 0.0f) && 
                ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y))) {
                isHovered = true;
            }
            
            ImVec4 normalText = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            if (isHovered) {
                ImGui::PushStyleColor(ImGuiCol_Text, normalText);
            } else {
                normalText.w *= 0.6f; // Dim by reducing alpha
                ImGui::PushStyleColor(ImGuiCol_Text, normalText);
            }
        }
        
        bool clicked = ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns);

        if (!isSelected && !isFlashing) {
            ImGui::PopStyleColor();
        }

        // 如果有右侧文本（例如状态、数值），画在右边
        if (!rightText.empty()) {
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(rightText.c_str()).x - ImGui::GetStyle().WindowPadding.x);
            
            // Render right text with slightly dimmed color if not selected, otherwise full color
            if (!isSelected) {
                ImVec4 normalText = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                if (!isHovered) {
                    normalText.w *= 0.6f;
                }
                ImGui::PushStyleColor(ImGuiCol_Text, normalText);
                ImGui::Text("%s", rightText.c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::Text("%s", rightText.c_str());
            }
        }

        if (isSelected) {
            ImGui::PopStyleColor(1);
        }

        if (ImGui::IsItemHovered() && (ImGui::GetIO().MouseDelta.x != 0.0f || ImGui::GetIO().MouseDelta.y != 0.0f)) {
            // Only update selection if the mouse is actively moving.
            // This prevents the selection from snapping to the mouse pointer's location 
            // when scrolling with keys/wheel and the list moves under the stationary mouse.
            listSelectedIndex = listCurrentIndex;
        } else if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            // Also select on click just in case
            listSelectedIndex = listCurrentIndex;
        }
        
        if (isSelected && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false) || ImGui::IsKeyPressed(ImGuiKey_Keypad5, false))) {
            if (!actionExecutedThisFrame) {
                clicked = true;
                actionExecutedThisFrame = true; // Prevent multiple clicks in the same frame
            }
        }

        if (clicked) {
            flashIndex = listCurrentIndex;
            flashTick = GetTickCount64() + 150; // flash for 150ms
        }

        // Scroll to keep selected item in view only when selection changes
        if (isSelected && lastScrolledIndex != listSelectedIndex) {
            ImGui::SetScrollHereY(0.5f); // 0.5f centers the item
            lastScrolledIndex = listSelectedIndex;
        }

        listCurrentIndex++;
        return clicked;
    }
    ImVec2 CalcSize(short count, bool spacing) {
        if (count <= 1) {
            return ImVec2(ContentWidth(), ImGui::GetFrameHeight() * 1.3f);
        }

        const float rowWidth = RowWidth();
        const float spacingWidth = spacing ? ImGui::GetStyle().ItemSpacing.x * (count - 1) : 0.0f;
        const float width = (rowWidth - spacingWidth) / count;

        return ImVec2(width, ImGui::GetFrameHeight() * 1.3f);
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
        if (MenuState::UseNativeMenu) return; // 移除旧逻辑
    }

    void EndPage() {
        if (MenuState::UseNativeMenu) return; // 移除旧逻辑
    }

    bool BeginTab(const char* id, const char* label) {
        if (MenuState::UseNativeMenu) {
            // Draw a separator/header to indicate a new section
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "[ %s ]", label);
            ImGui::Separator();
            
            ImGui::PushID(id);
            return true;
        } else {
            return ImGui::BeginTabItem(label);
        }
    }

    void EndTab() {
        if (MenuState::UseNativeMenu) {
            ImGui::PopID();
            ImGui::Spacing();
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
            bool dummyLR;
            if (DrawListItemBase(label, isOpen ? "[-]" : "[+]", dummyLR, false)) {
                isOpen = !isOpen;
            }
            return isOpen;
        } else {
            // ImGui::CollapsingHeader internally manages state, but we want to bind it to isOpen
            ImGui::SetNextItemOpen(isOpen, ImGuiCond_Always);
            bool clicked = ImGui::CollapsingHeader(label);
            if (ImGui::IsItemClicked()) {
                isOpen = !isOpen;
            }
            return isOpen;
        }
    }

    bool Checkbox(const char* label, bool* v) {
        if (MenuState::UseNativeMenu) {
            bool dummyLR;
            if (DrawListItemBase(label, *v ? "[ ON ]" : "[ OFF ]", dummyLR, false)) {
                *v = !*v;
                return true;
            }
            return false;
        }
        return ImGui::Checkbox(label, v);
    }

    bool Button(const char* label, short columns) {
        if (MenuState::UseNativeMenu) {
            bool dummyLR;
            return DrawListItemBase(label, "", dummyLR, false);
        }
        return ImGui::Button(label, CalcSize(columns));
    }

    bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
        if (MenuState::UseNativeMenu) {
            char numText[32];
            snprintf(numText, sizeof(numText), format, *v);
            char valText[64];
            snprintf(valText, sizeof(valText), "< %s >", numText);

            bool changedLR;
            bool activated = DrawListItemBase(label, valText, changedLR, true);

            if (changedLR) {
                float step = 0.1f;
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, true)) *v -= step;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad6, true)) *v += step;
                if (*v < v_min) *v = v_min;
                if (*v > v_max) *v = v_max;
                return true;
            }
            return activated;
        }
        return ImGui::SliderFloat(label, v, v_min, v_max, format);
    }

    bool SliderInt(const char* label, int* v, int v_min, int v_max) {
        if (MenuState::UseNativeMenu) {
            char valText[32];
            snprintf(valText, sizeof(valText), "< %d >", *v);

            bool changedLR;
            bool activated = DrawListItemBase(label, valText, changedLR, true);

            if (changedLR) {
                int step = 1;
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, true)) *v -= step;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad6, true)) *v += step;
                if (*v < v_min) *v = v_min;
                if (*v > v_max) *v = v_max;
                return true;
            }
            return activated;
        }
        return ImGui::SliderInt(label, v, v_min, v_max);
    }

    bool InputFloat(const char* label, float* v, float step, float step_fast, const char* format) {
        if (MenuState::UseNativeMenu) {
            return SliderFloat(label, v, -10000.0f, 10000.0f, format);
        }
        return ImGui::InputFloat(label, v, step, step_fast, format);
    }

    bool InputInt(const char* label, int* v, int step, int step_fast) {
        if (MenuState::UseNativeMenu) {
            char valText[32];
            snprintf(valText, sizeof(valText), "< %d >", *v);

            bool changedLR;
            bool activated = DrawListItemBase(label, valText, changedLR, true);

            if (changedLR) {
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, true)) *v -= step;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) || ImGui::IsKeyPressed(ImGuiKey_Keypad6, true)) *v += step;
                return true;
            }
            
            // Allow direct input via popup when activated
            if (activated) {
                ImGui::OpenPopup("InputIntPopup");
            }
            
            if (ImGui::BeginPopup("InputIntPopup")) {
                ImGui::SetKeyboardFocusHere();
                ImGui::InputInt("##val", v, step, step_fast);
                if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            return activated;
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