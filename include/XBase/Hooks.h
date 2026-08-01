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

} // namespace XBase::Hooks
