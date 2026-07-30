#pragma once
#include <windows.h>
#include <d3d9.h>
#include <functional>
#include <cstdint>

class D3DHook {
public:
    static bool Init(std::function<void()> onRender);
    static void Shutdown();
    static void SetMenuVisible(bool visible);
    static bool IsMenuVisible();
    static void SetBackgroundInputActive(bool active);
    static bool IsBackgroundInputActive();
    static void SetBackgroundRenderActive(bool active);
    static bool IsBackgroundRenderActive();
    static void ToggleMenu();
    static void ProcessMouse();
    static void MaintainInputState();
    static LPDIRECT3DDEVICE9 GetDevice();
    static bool IsInitialized();
    static bool IsReady();
    static bool HadInitFailure();
    static float ConsumeRawWheelDelta();
    static const char* GetInitStatus();
    static const char* GetStatusText();

private:
    static LRESULT __stdcall hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice);
    static HRESULT __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
    static HRESULT __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
    static void InitImGui(LPDIRECT3DDEVICE9 pDevice);

    static HWND window;
    static WNDPROC oWndProc;
    static bool menuVisible;
    static bool isInitialized;
    static bool hookInstalled;
    static bool initFailed;
    static bool backgroundInputActive;
    static bool backgroundRenderActive;
    static LPDIRECT3DDEVICE9 device;
    static const char* initStatus;
    static std::function<void()> renderCallback;
};
