#include "GuiTheme.h"
#include "MenuState.h"
#include <XBase/Theme.h>

namespace GuiTheme {

    const Theme Themes[ThemeCount] = {
        {
            ThemeId::Default, "settings.theme.default",
            Color(0.92f, 0.92f, 0.92f, 1.00f), Color(0.50f, 0.50f, 0.50f, 1.00f),
            Color(0.055f, 0.060f, 0.075f, 0.98f), Color(0.075f, 0.082f, 0.100f, 1.00f),
            Color(0.180f, 0.280f, 0.420f, 0.85f), Color(0.220f, 0.340f, 0.520f, 0.95f),
            Color(0.260f, 0.420f, 0.650f, 1.00f),
            Color(0.170f, 0.270f, 0.400f, 0.90f), Color(0.220f, 0.350f, 0.540f, 1.00f),
            Color(0.260f, 0.420f, 0.650f, 1.00f),
            Color(0.120f, 0.160f, 0.220f, 1.00f), Color(0.180f, 0.240f, 0.340f, 1.00f),
            Color(0.220f, 0.300f, 0.440f, 1.00f),
            Color(0.260f, 0.580f, 0.980f, 1.00f),
            Color(0.260f, 0.580f, 0.980f, 1.00f), Color(0.360f, 0.680f, 1.00f, 1.00f),
            Color(0.220f, 0.340f, 0.520f, 0.95f), Color(0.260f, 0.420f, 0.650f, 1.00f),
            Color(0.120f, 0.160f, 0.220f, 0.50f), Color(0.260f, 0.580f, 0.980f, 1.00f),
            Color(0.055f, 0.060f, 0.075f, 0.94f),
            Color(0.180f, 0.240f, 0.340f, 0.55f),
            Color(0.100f, 0.140f, 0.200f, 1.00f), Color(0.160f, 0.240f, 0.360f, 1.00f),
            Color(0.040f, 0.045f, 0.055f, 0.90f), Color(0.220f, 0.340f, 0.520f, 0.85f),
            8.0f, Vec2(12.0f, 12.0f), Vec2(10.0f, 8.0f),
            4.0f, 4.0f
        },
        {
            ThemeId::DarkDeep, "settings.theme.darkDeep",
            Color(0.88f, 0.88f, 0.88f, 1.00f), Color(0.40f, 0.40f, 0.40f, 1.00f),
            Color(0.020f, 0.020f, 0.030f, 0.98f), Color(0.035f, 0.035f, 0.050f, 1.00f),
            Color(0.140f, 0.200f, 0.320f, 0.85f), Color(0.180f, 0.260f, 0.420f, 0.95f),
            Color(0.220f, 0.340f, 0.520f, 1.00f),
            Color(0.130f, 0.190f, 0.300f, 0.90f), Color(0.180f, 0.260f, 0.420f, 1.00f),
            Color(0.220f, 0.340f, 0.520f, 1.00f),
            Color(0.080f, 0.100f, 0.160f, 1.00f), Color(0.130f, 0.170f, 0.260f, 1.00f),
            Color(0.170f, 0.230f, 0.350f, 1.00f),
            Color(0.200f, 0.520f, 0.920f, 1.00f),
            Color(0.200f, 0.520f, 0.920f, 1.00f), Color(0.300f, 0.620f, 1.00f, 1.00f),
            Color(0.180f, 0.260f, 0.420f, 0.95f), Color(0.220f, 0.340f, 0.520f, 1.00f),
            Color(0.080f, 0.100f, 0.160f, 0.50f), Color(0.200f, 0.520f, 0.920f, 1.00f),
            Color(0.020f, 0.020f, 0.030f, 0.94f),
            Color(0.100f, 0.140f, 0.220f, 0.50f),
            Color(0.050f, 0.060f, 0.090f, 1.00f), Color(0.100f, 0.150f, 0.240f, 1.00f),
            Color(0.015f, 0.015f, 0.020f, 0.90f), Color(0.180f, 0.260f, 0.420f, 0.80f),
            4.0f, Vec2(10.0f, 10.0f), Vec2(8.0f, 6.0f),
            2.0f, 2.0f
        },
        {
            ThemeId::Classic, "settings.theme.classic",
            Color(0.90f, 0.90f, 0.88f, 1.00f), Color(0.55f, 0.55f, 0.52f, 1.00f),
            Color(0.085f, 0.085f, 0.080f, 0.97f), Color(0.110f, 0.110f, 0.105f, 1.00f),
            Color(0.320f, 0.280f, 0.160f, 0.80f), Color(0.400f, 0.350f, 0.200f, 0.90f),
            Color(0.480f, 0.420f, 0.240f, 1.00f),
            Color(0.300f, 0.260f, 0.140f, 0.85f), Color(0.380f, 0.330f, 0.180f, 0.95f),
            Color(0.460f, 0.400f, 0.220f, 1.00f),
            Color(0.200f, 0.180f, 0.100f, 1.00f), Color(0.280f, 0.250f, 0.140f, 1.00f),
            Color(0.340f, 0.300f, 0.160f, 1.00f),
            Color(0.880f, 0.740f, 0.280f, 1.00f),
            Color(0.780f, 0.640f, 0.220f, 1.00f), Color(0.920f, 0.780f, 0.320f, 1.00f),
            Color(0.400f, 0.350f, 0.200f, 0.90f), Color(0.480f, 0.420f, 0.240f, 1.00f),
            Color(0.200f, 0.180f, 0.100f, 0.50f), Color(0.880f, 0.740f, 0.280f, 1.00f),
            Color(0.085f, 0.085f, 0.080f, 0.94f),
            Color(0.280f, 0.240f, 0.120f, 0.55f),
            Color(0.140f, 0.120f, 0.070f, 1.00f), Color(0.220f, 0.190f, 0.100f, 1.00f),
            Color(0.050f, 0.050f, 0.045f, 0.90f), Color(0.400f, 0.350f, 0.200f, 0.85f),
            2.0f, Vec2(8.0f, 8.0f), Vec2(8.0f, 6.0f),
            0.0f, 0.0f
        },
        {
            ThemeId::Light, "settings.theme.light",
            Color(0.08f, 0.08f, 0.10f, 1.00f), Color(0.45f, 0.45f, 0.48f, 1.00f),
            Color(0.940f, 0.940f, 0.950f, 0.98f), Color(0.970f, 0.970f, 0.980f, 1.00f),
            Color(0.560f, 0.600f, 0.700f, 0.70f), Color(0.620f, 0.660f, 0.760f, 0.85f),
            Color(0.500f, 0.540f, 0.640f, 1.00f),
            Color(0.500f, 0.540f, 0.640f, 0.80f), Color(0.560f, 0.600f, 0.700f, 0.90f),
            Color(0.440f, 0.480f, 0.580f, 1.00f),
            Color(0.860f, 0.880f, 0.920f, 1.00f), Color(0.800f, 0.820f, 0.880f, 1.00f),
            Color(0.740f, 0.760f, 0.840f, 1.00f),
            Color(0.200f, 0.360f, 0.720f, 1.00f),
            Color(0.200f, 0.360f, 0.720f, 1.00f), Color(0.280f, 0.440f, 0.800f, 1.00f),
            Color(0.620f, 0.660f, 0.760f, 0.85f), Color(0.500f, 0.540f, 0.640f, 1.00f),
            Color(0.680f, 0.700f, 0.760f, 0.50f), Color(0.200f, 0.360f, 0.720f, 1.00f),
            Color(0.940f, 0.940f, 0.950f, 0.94f),
            Color(0.620f, 0.640f, 0.700f, 0.55f),
            Color(0.860f, 0.870f, 0.900f, 1.00f), Color(0.780f, 0.800f, 0.860f, 1.00f),
            Color(0.900f, 0.900f, 0.920f, 0.90f), Color(0.560f, 0.600f, 0.700f, 0.80f),
            6.0f, Vec2(12.0f, 10.0f), Vec2(10.0f, 6.0f),
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
        const Theme& t = Themes[currentThemeIndex];
        XBase::Theme::ApplyStyle({
            static_cast<int>(t.id), t.nameKey,
            t.Text, t.TextDisabled, t.WindowBg, t.ChildBg,
            t.Header, t.HeaderHovered, t.HeaderActive,
            t.Button, t.ButtonHovered, t.ButtonActive,
            t.FrameBg, t.FrameBgHovered, t.FrameBgActive,
            t.CheckMark, t.SliderGrab, t.SliderGrabActive,
            t.TabHovered, t.TabActive, t.Separator, t.NavHighlight,
            t.PopupBg, t.Border, t.TitleBg, t.TitleBgActive,
            t.ScrollbarBg, t.ScrollbarGrab,
            t.WindowRounding, t.WindowPadding, t.ItemSpacing,
            t.FrameRounding, t.TabRounding
        });
        appliedThemeIndex = currentThemeIndex;
    }

    void ApplyInteraction() {
        const bool listMode = MenuState::UseNativeMenu;
        const bool keyboardNavigation =
            !listMode && currentInteractionIndex == static_cast<int>(InteractionMode::KeyboardMouse);
        const bool mouseEnabled = !listMode || MenuState::ListMenuMouseInput;
        XBase::Theme::ConfigureInteraction(keyboardNavigation, mouseEnabled);
    }

    bool WantsMouseCursor() {
        return !MenuState::UseNativeMenu || MenuState::ListMenuMouseInput;
    }

    void Sync() {
        if (appliedThemeIndex != currentThemeIndex) {
            ApplyToStyle();
        }
        ApplyInteraction();
    }

    void ApplyTheme() {
        ApplyToStyle();
    }

    void RestoreTheme() {
    }
}