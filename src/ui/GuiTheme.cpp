#include "GuiTheme.h"
#include "MenuState.h"
#include "imgui/imgui.h"

namespace GuiTheme {
    const Theme Themes[ThemeCount] = {
        {
            ThemeId::Default, "settings.theme.default",
            ImVec4(0.92f, 0.92f, 0.92f, 1.00f), ImVec4(0.50f, 0.50f, 0.50f, 1.00f),
            ImVec4(0.055f, 0.060f, 0.075f, 0.98f), ImVec4(0.075f, 0.082f, 0.100f, 1.00f),
            ImVec4(0.180f, 0.280f, 0.420f, 0.85f), ImVec4(0.220f, 0.340f, 0.520f, 0.95f),
            ImVec4(0.260f, 0.420f, 0.650f, 1.00f),
            ImVec4(0.170f, 0.270f, 0.400f, 0.90f), ImVec4(0.220f, 0.350f, 0.540f, 1.00f),
            ImVec4(0.260f, 0.420f, 0.650f, 1.00f),
            ImVec4(0.120f, 0.160f, 0.220f, 1.00f), ImVec4(0.180f, 0.240f, 0.340f, 1.00f),
            ImVec4(0.220f, 0.300f, 0.440f, 1.00f),
            ImVec4(0.260f, 0.580f, 0.980f, 1.00f),
            ImVec4(0.260f, 0.580f, 0.980f, 1.00f), ImVec4(0.360f, 0.680f, 1.00f, 1.00f),
            ImVec4(0.220f, 0.340f, 0.520f, 0.95f), ImVec4(0.260f, 0.420f, 0.650f, 1.00f),
            ImVec4(0.120f, 0.160f, 0.220f, 0.50f), ImVec4(0.260f, 0.580f, 0.980f, 1.00f),
            ImVec4(0.055f, 0.060f, 0.075f, 0.94f),
            ImVec4(0.180f, 0.240f, 0.340f, 0.55f),
            ImVec4(0.100f, 0.140f, 0.200f, 1.00f), ImVec4(0.160f, 0.240f, 0.360f, 1.00f),
            ImVec4(0.040f, 0.045f, 0.055f, 0.90f), ImVec4(0.220f, 0.340f, 0.520f, 0.85f),
            8.0f, ImVec2(12.0f, 12.0f), ImVec2(10.0f, 8.0f),
            4.0f, 4.0f
        },
        {
            ThemeId::DarkDeep, "settings.theme.darkDeep",
            ImVec4(0.88f, 0.88f, 0.88f, 1.00f), ImVec4(0.40f, 0.40f, 0.40f, 1.00f),
            ImVec4(0.020f, 0.020f, 0.030f, 0.98f), ImVec4(0.035f, 0.035f, 0.050f, 1.00f),
            ImVec4(0.140f, 0.200f, 0.320f, 0.85f), ImVec4(0.180f, 0.260f, 0.420f, 0.95f),
            ImVec4(0.220f, 0.340f, 0.520f, 1.00f),
            ImVec4(0.130f, 0.190f, 0.300f, 0.90f), ImVec4(0.180f, 0.260f, 0.420f, 1.00f),
            ImVec4(0.220f, 0.340f, 0.520f, 1.00f),
            ImVec4(0.080f, 0.100f, 0.160f, 1.00f), ImVec4(0.130f, 0.170f, 0.260f, 1.00f),
            ImVec4(0.170f, 0.230f, 0.350f, 1.00f),
            ImVec4(0.200f, 0.520f, 0.920f, 1.00f),
            ImVec4(0.200f, 0.520f, 0.920f, 1.00f), ImVec4(0.300f, 0.620f, 1.00f, 1.00f),
            ImVec4(0.180f, 0.260f, 0.420f, 0.95f), ImVec4(0.220f, 0.340f, 0.520f, 1.00f),
            ImVec4(0.080f, 0.100f, 0.160f, 0.50f), ImVec4(0.200f, 0.520f, 0.920f, 1.00f),
            ImVec4(0.020f, 0.020f, 0.030f, 0.94f),
            ImVec4(0.100f, 0.140f, 0.220f, 0.50f),
            ImVec4(0.050f, 0.060f, 0.090f, 1.00f), ImVec4(0.100f, 0.150f, 0.240f, 1.00f),
            ImVec4(0.015f, 0.015f, 0.020f, 0.90f), ImVec4(0.180f, 0.260f, 0.420f, 0.80f),
            4.0f, ImVec2(10.0f, 10.0f), ImVec2(8.0f, 6.0f),
            2.0f, 2.0f
        },
        {
            ThemeId::Classic, "settings.theme.classic",
            ImVec4(0.90f, 0.90f, 0.88f, 1.00f), ImVec4(0.55f, 0.55f, 0.52f, 1.00f),
            ImVec4(0.085f, 0.085f, 0.080f, 0.97f), ImVec4(0.110f, 0.110f, 0.105f, 1.00f),
            ImVec4(0.320f, 0.280f, 0.160f, 0.80f), ImVec4(0.400f, 0.350f, 0.200f, 0.90f),
            ImVec4(0.480f, 0.420f, 0.240f, 1.00f),
            ImVec4(0.300f, 0.260f, 0.140f, 0.85f), ImVec4(0.380f, 0.330f, 0.180f, 0.95f),
            ImVec4(0.460f, 0.400f, 0.220f, 1.00f),
            ImVec4(0.200f, 0.180f, 0.100f, 1.00f), ImVec4(0.280f, 0.250f, 0.140f, 1.00f),
            ImVec4(0.340f, 0.300f, 0.160f, 1.00f),
            ImVec4(0.880f, 0.740f, 0.280f, 1.00f),
            ImVec4(0.780f, 0.640f, 0.220f, 1.00f), ImVec4(0.920f, 0.780f, 0.320f, 1.00f),
            ImVec4(0.400f, 0.350f, 0.200f, 0.90f), ImVec4(0.480f, 0.420f, 0.240f, 1.00f),
            ImVec4(0.200f, 0.180f, 0.100f, 0.50f), ImVec4(0.880f, 0.740f, 0.280f, 1.00f),
            ImVec4(0.085f, 0.085f, 0.080f, 0.94f),
            ImVec4(0.280f, 0.240f, 0.120f, 0.55f),
            ImVec4(0.140f, 0.120f, 0.070f, 1.00f), ImVec4(0.220f, 0.190f, 0.100f, 1.00f),
            ImVec4(0.050f, 0.050f, 0.045f, 0.90f), ImVec4(0.400f, 0.350f, 0.200f, 0.85f),
            2.0f, ImVec2(8.0f, 8.0f), ImVec2(8.0f, 6.0f),
            0.0f, 0.0f
        },
        {
            ThemeId::Light, "settings.theme.light",
            ImVec4(0.08f, 0.08f, 0.10f, 1.00f), ImVec4(0.45f, 0.45f, 0.48f, 1.00f),
            ImVec4(0.940f, 0.940f, 0.950f, 0.98f), ImVec4(0.970f, 0.970f, 0.980f, 1.00f),
            ImVec4(0.560f, 0.600f, 0.700f, 0.70f), ImVec4(0.620f, 0.660f, 0.760f, 0.85f),
            ImVec4(0.500f, 0.540f, 0.640f, 1.00f),
            ImVec4(0.500f, 0.540f, 0.640f, 0.80f), ImVec4(0.560f, 0.600f, 0.700f, 0.90f),
            ImVec4(0.440f, 0.480f, 0.580f, 1.00f),
            ImVec4(0.860f, 0.880f, 0.920f, 1.00f), ImVec4(0.800f, 0.820f, 0.880f, 1.00f),
            ImVec4(0.740f, 0.760f, 0.840f, 1.00f),
            ImVec4(0.200f, 0.360f, 0.720f, 1.00f),
            ImVec4(0.200f, 0.360f, 0.720f, 1.00f), ImVec4(0.280f, 0.440f, 0.800f, 1.00f),
            ImVec4(0.620f, 0.660f, 0.760f, 0.85f), ImVec4(0.500f, 0.540f, 0.640f, 1.00f),
            ImVec4(0.680f, 0.700f, 0.760f, 0.50f), ImVec4(0.200f, 0.360f, 0.720f, 1.00f),
            ImVec4(0.940f, 0.940f, 0.950f, 0.94f),
            ImVec4(0.620f, 0.640f, 0.700f, 0.55f),
            ImVec4(0.860f, 0.870f, 0.900f, 1.00f), ImVec4(0.780f, 0.800f, 0.860f, 1.00f),
            ImVec4(0.900f, 0.900f, 0.920f, 0.90f), ImVec4(0.560f, 0.600f, 0.700f, 0.80f),
            6.0f, ImVec2(12.0f, 10.0f), ImVec2(10.0f, 6.0f),
            4.0f, 4.0f
        }
    };

    static int currentThemeIndex = 0;
    static int currentInteractionIndex = 0;
    static int appliedThemeIndex = -1;

    int GetThemeIndex() {
        return currentThemeIndex;
    }

    void SetThemeByIndex(int index) {
        if (index >= 0 && index < ThemeCount) {
            if (currentThemeIndex != index) {
                currentThemeIndex = index;
                appliedThemeIndex = -1; // 强制下次 Sync 重写 Style
            }
        }
    }

    const char* GetThemeNameKey(int index) {
        if (index >= 0 && index < ThemeCount) {
            return Themes[index].nameKey;
        }
        return Themes[0].nameKey;
    }

    const Theme& GetCurrentTheme() {
        return Themes[currentThemeIndex];
    }

    int GetInteractionIndex() {
        return currentInteractionIndex;
    }

    void SetInteractionByIndex(int index) {
        if (index >= 0 && index < InteractionCount) {
            currentInteractionIndex = index;
        }
    }

    const char* GetInteractionNameKey(int index) {
        static const char* keys[InteractionCount] = {
            "settings.interaction.mouse",
            "settings.interaction.keyboardMouse"
        };
        if (index >= 0 && index < InteractionCount) {
            return keys[index];
        }
        return keys[0];
    }

    void ApplyToStyle() {
        if (!ImGui::GetCurrentContext()) {
            return;
        }

        const Theme& t = Themes[currentThemeIndex];
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = t.WindowRounding;
        style.WindowPadding = t.WindowPadding;
        style.ItemSpacing = t.ItemSpacing;
        style.FrameRounding = t.FrameRounding;
        style.TabRounding = t.TabRounding;
        style.ChildRounding = t.FrameRounding;
        style.PopupRounding = t.FrameRounding;
        style.ScrollbarRounding = t.FrameRounding;
        style.GrabRounding = t.FrameRounding;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = t.Text;
        colors[ImGuiCol_TextDisabled] = t.TextDisabled;
        colors[ImGuiCol_WindowBg] = t.WindowBg;
        colors[ImGuiCol_ChildBg] = t.ChildBg;
        colors[ImGuiCol_PopupBg] = t.PopupBg;
        colors[ImGuiCol_Border] = t.Border;
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg] = t.FrameBg;
        colors[ImGuiCol_FrameBgHovered] = t.FrameBgHovered;
        colors[ImGuiCol_FrameBgActive] = t.FrameBgActive;
        colors[ImGuiCol_TitleBg] = t.TitleBg;
        colors[ImGuiCol_TitleBgActive] = t.TitleBgActive;
        colors[ImGuiCol_TitleBgCollapsed] = t.TitleBg;
        colors[ImGuiCol_MenuBarBg] = t.TitleBg;
        colors[ImGuiCol_ScrollbarBg] = t.ScrollbarBg;
        colors[ImGuiCol_ScrollbarGrab] = t.ScrollbarGrab;
        colors[ImGuiCol_ScrollbarGrabHovered] = t.HeaderHovered;
        colors[ImGuiCol_ScrollbarGrabActive] = t.HeaderActive;
        colors[ImGuiCol_CheckMark] = t.CheckMark;
        colors[ImGuiCol_SliderGrab] = t.SliderGrab;
        colors[ImGuiCol_SliderGrabActive] = t.SliderGrabActive;
        colors[ImGuiCol_Button] = t.Button;
        colors[ImGuiCol_ButtonHovered] = t.ButtonHovered;
        colors[ImGuiCol_ButtonActive] = t.ButtonActive;
        colors[ImGuiCol_Header] = t.Header;
        colors[ImGuiCol_HeaderHovered] = t.HeaderHovered;
        colors[ImGuiCol_HeaderActive] = t.HeaderActive;
        colors[ImGuiCol_Separator] = t.Separator;
        colors[ImGuiCol_SeparatorHovered] = t.HeaderHovered;
        colors[ImGuiCol_SeparatorActive] = t.HeaderActive;
        colors[ImGuiCol_ResizeGrip] = t.Header;
        colors[ImGuiCol_ResizeGripHovered] = t.HeaderHovered;
        colors[ImGuiCol_ResizeGripActive] = t.HeaderActive;
        colors[ImGuiCol_Tab] = t.Button;
        colors[ImGuiCol_TabHovered] = t.TabHovered;
        colors[ImGuiCol_TabActive] = t.TabActive;
        colors[ImGuiCol_TabUnfocused] = t.FrameBg;
        colors[ImGuiCol_TabUnfocusedActive] = t.Header;
        colors[ImGuiCol_PlotLines] = t.CheckMark;
        colors[ImGuiCol_PlotLinesHovered] = t.SliderGrabActive;
        colors[ImGuiCol_PlotHistogram] = t.CheckMark;
        colors[ImGuiCol_PlotHistogramHovered] = t.SliderGrabActive;
        colors[ImGuiCol_TableHeaderBg] = t.Header;
        colors[ImGuiCol_TableBorderStrong] = t.Border;
        colors[ImGuiCol_TableBorderLight] = t.Separator;
        colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(t.Text.x, t.Text.y, t.Text.z, 0.04f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(t.HeaderActive.x, t.HeaderActive.y, t.HeaderActive.z, 0.40f);
        colors[ImGuiCol_DragDropTarget] = t.CheckMark;
        colors[ImGuiCol_NavHighlight] = t.NavHighlight;
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1, 1, 1, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.45f);

        appliedThemeIndex = currentThemeIndex;
    }

    void ApplyInteraction() {
        if (!ImGui::GetCurrentContext()) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        const bool listMode = MenuState::UseNativeMenu;
        const bool panelKeyboard =
            !listMode &&
            currentInteractionIndex == static_cast<int>(InteractionMode::KeyboardMouse);

        // 列表用自有导航；面板仅 KeyboardMouse 开 ImGui Nav
        if (panelKeyboard) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        }

        // 列表默认吃鼠标；仅用户关闭 ListMenuMouseInput 时 NoMouse
        if (listMode && !MenuState::ListMenuMouseInput) {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }
    }

    bool WantsMouseCursor() {
        if (MenuState::UseNativeMenu) {
            return MenuState::ListMenuMouseInput;
        }
        return true;
    }

    void Sync() {
        if (!ImGui::GetCurrentContext()) {
            return;
        }
        if (appliedThemeIndex != currentThemeIndex) {
            ApplyToStyle();
        }
        ApplyInteraction();
    }

    void ApplyTheme() {
        ApplyToStyle();
    }

    void RestoreTheme() {
        // Style 已全局写入，无需 Pop
    }
}