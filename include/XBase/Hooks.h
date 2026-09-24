#pragma once

#include <cstdint>
#include <functional>

namespace XBase::Hooks {

struct DrawCallbackId {
    std::uint64_t value = 0;

    constexpr explicit operator bool() const {
        return value != 0;
    }
};

enum class RuntimeState {
    Uninitialized,
    Hooked,
    RenderReady,
    ShuttingDown,
    Failed,
};

bool Init();
void Shutdown();
RuntimeState GetState();
bool IsInitialized();
bool IsReady();
bool HadInitFailure();
const char* GetStatusText();

DrawCallbackId RegisterDrawCallback(std::function<void()> callback);
bool UnregisterDrawCallback(DrawCallbackId callbackId);
void SetMenuVisible(bool visible);
bool IsMenuVisible();
void ToggleMenu();

void SetBackgroundInputActive(bool active);
bool IsBackgroundInputActive();
void SetBackgroundRenderActive(bool active);
bool IsBackgroundRenderActive();
void MaintainInputState();
float GetFrameDeltaSeconds();
bool IsKeyboardCaptureActive();
float ConsumeWheelDelta();

// While suppressed the wheel delta is consumed by XBase and the raw message is
// not forwarded to the game window, so the host can own wheel driven actions.
void SetWheelInputSuppressed(bool suppressed);
bool IsWheelInputSuppressed();

// True when the game window has no caption and covers its whole monitor, which
// means DWM composition is bypassed and HWND overlays cannot appear on screen.
bool IsGameWindowFullscreen();

// Window modes: fullscreen keeps the game swap chain exclusive, windowed and
// borderless present in a window so DWM composes the window and HWND overlays
// stay visible at native speed. Borderless fills the monitor without a frame.
enum class WindowMode {
    Fullscreen,
    Windowed,
    Borderless,
};

bool IsWindowModeSupported();
WindowMode GetWindowMode();
bool SetWindowMode(WindowMode mode);

// Call before the game creates its D3D device at ASI load time to make the
// game build a windowed swap chain from the start, like the reference
// III.VC.SA.WindowedMode plugin: windowed presentation is what lets DWM
// compose the game window and HWND overlays. Exclusive fullscreen can only
// be left by recreating the device, which is why the mode applies at launch.
bool PrepareStartupWindowMode(WindowMode mode);

} // namespace XBase::Hooks
