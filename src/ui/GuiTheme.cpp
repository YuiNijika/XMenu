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
            6.0f, ImVec2(12.0f, 10.0f), ImVec2(10.0f, 6.0f),
            4.0f, 4.0f
        }
    };

    static int currentThemeIndex = 0;
    static int currentInteractionIndex = 0;

    int GetThemeIndex() {
        return currentThemeIndex;
    }

    void SetThemeByIndex(int index) {
        if (index >= 0 && index < ThemeCount) {
            currentThemeIndex = index;
        }
    }

    const char* GetThemeNameKey(int index) {
        if (index >= 0 && index < ThemeCount) {
            return Themes[index].nameKey;
        }
        return Themes[0].nameKey;
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

    void ApplyTheme() {
        const Theme& t = Themes[currentThemeIndex];

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, t.WindowRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, t.WindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, t.ItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, t.FrameRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, t.TabRounding);

        ImGui::PushStyleColor(ImGuiCol_Text, t.Text);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, t.TextDisabled);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, t.WindowBg);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, t.ChildBg);
        ImGui::PushStyleColor(ImGuiCol_Header, t.Header);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, t.HeaderHovered);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, t.HeaderActive);
        ImGui::PushStyleColor(ImGuiCol_Button, t.Button);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, t.ButtonHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, t.ButtonActive);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, t.FrameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, t.FrameBgHovered);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, t.FrameBgActive);
        ImGui::PushStyleColor(ImGuiCol_CheckMark, t.CheckMark);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, t.SliderGrab);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, t.SliderGrabActive);
        ImGui::PushStyleColor(ImGuiCol_TabHovered, t.TabHovered);
        ImGui::PushStyleColor(ImGuiCol_TabActive, t.TabActive);
        ImGui::PushStyleColor(ImGuiCol_Separator, t.Separator);
        ImGui::PushStyleColor(ImGuiCol_NavHighlight, t.NavHighlight);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, t.PopupBg);
    }

    void RestoreTheme() {
        ImGui::PopStyleColor(ColorPushCount);
        ImGui::PopStyleVar(StyleVarPushCount);
    }

    void ApplyInteraction() {
        ImGuiIO& io = ImGui::GetIO();

        if (currentInteractionIndex == static_cast<int>(InteractionMode::KeyboardMouse)) {
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.NavActive = true;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        }
    }
}