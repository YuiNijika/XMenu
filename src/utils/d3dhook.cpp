#include "D3DHook.h"
#include "defines.h"
#include "plugin.h"
#include "Patch.h"
#include "common.h"
#include "CPad.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
#include "kiero/kiero.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
typedef HRESULT(__stdcall* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(__stdcall* Present_t)(LPDIRECT3DDEVICE9, const RECT*, const RECT*, HWND, const RGNDATA*);

static EndScene_t oEndScene = NULL;
static Reset_t oReset = NULL;
static Present_t oPresent = NULL;

HWND D3DHook::window = NULL;
WNDPROC D3DHook::oWndProc = NULL;
bool D3DHook::menuVisible = false;
bool D3DHook::isInitialized = false;
std::function<void()> D3DHook::renderCallback = nullptr;

bool D3DHook::IsMenuVisible() {
    return menuVisible;
}

void D3DHook::SetMenuVisible(bool visible) {
    if (menuVisible == visible) return;
    menuVisible = visible;
    ProcessMouse();
}

void D3DHook::ProcessMouse() {
    ImGui::GetIO().MouseDrawCursor = menuVisible;

    if (menuVisible) {
        plugin::patch::SetUChar((uintptr_t)BY_GAME(0x6194A0, 0x6020A0, 0x580D20), 0xC3); // psSetMousePos
        plugin::patch::Nop((uintptr_t)BY_GAME(0x541DD7, 0x4AB6CA, 0x49272F), 5); // don't call CPad::UpdateMouse()
#ifdef GTASA
        plugin::patch::SetUChar(0x4EB731, 0xEB); // jz -> jmp, skip mouse checks
        plugin::patch::SetUChar(0x4EB75A, 0xEB); // jz -> jmp, skip mouse checks
#endif
    } else {
        plugin::patch::SetUChar((uintptr_t)BY_GAME(0x6194A0, 0x6020A0, 0x580D20), BY_GAME(0xE9, 0x53, 0x53));
#ifdef GTASA
        plugin::patch::SetRaw(0x541DD7, (void*)"\xE8\xE4\xD5\xFF\xFF", 5);
        plugin::patch::SetUChar(0x4EB731, 0x74); // jz
        plugin::patch::SetUChar(0x4EB75A, 0x74); // jz
#elif GTAVC
        plugin::patch::SetRaw(0x4AB6CA, (void*)"\xE8\x51\x21\x00\x00", 5);
#else
        plugin::patch::SetRaw(0x49272F, (void*)"\xE8\x6C\xF5\xFF\xFF", 5);
#endif
        // 清除鼠标历史记录防止视角跳变
        CPad::UpdatePads();
        CPad::NewMouseControllerState.x = 0;
        CPad::NewMouseControllerState.y = 0;
#ifdef GTA3
        // CPad::GetPad(0)->ClearMouseHistory(); // Disabled to prevent camera reset
#else
        // CPad::ClearMouseHistory(); // Disabled to prevent camera reset
        CPad::GetPad(0)->NewState.DPadUp = 0;
        CPad::GetPad(0)->OldState.DPadUp = 0;
        CPad::GetPad(0)->NewState.DPadDown = 0;
        CPad::GetPad(0)->OldState.DPadDown = 0;
#endif
    }
}

LRESULT __stdcall D3DHook::hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (menuVisible) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_MOUSEWHEEL) {
            return true;
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void D3DHook::InitImGui(LPDIRECT3DDEVICE9 pDevice) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    
    // 加载中文字体
    char windowsPath[MAX_PATH];
    GetWindowsDirectoryA(windowsPath, MAX_PATH);
    std::string fontPath = std::string(windowsPath) + "\\Fonts\\msyh.ttc";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(pDevice);
    
    isInitialized = true;
}

HRESULT __stdcall D3DHook::hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
    if (!isInitialized) {
        D3DDEVICE_CREATION_PARAMETERS params;
        pDevice->GetCreationParameters(&params);
        window = params.hFocusWindow;
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)hkWndProc);
        InitImGui(pDevice);
    }
    
    if (isInitialized && menuVisible) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        if (renderCallback) {
            renderCallback();
        }
        
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
    return oEndScene(pDevice);
}

HRESULT __stdcall D3DHook::hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (isInitialized) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        HRESULT hr = oReset(pDevice, pPresentationParameters);
        ImGui_ImplDX9_CreateDeviceObjects();
        return hr;
    }
    return oReset(pDevice, pPresentationParameters);
}

HRESULT __stdcall D3DHook::hkPresent(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

bool D3DHook::Init(std::function<void()> onRender) {
    renderCallback = onRender;
    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success) {
        kiero::bind(42, (void**)&oEndScene, (void*)hkEndScene);
        kiero::bind(16, (void**)&oReset, (void*)hkReset);
        kiero::bind(17, (void**)&oPresent, (void*)hkPresent);
        return true;
    }
    return false;
}

void D3DHook::Shutdown() {
    if (isInitialized) {
        SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)oWndProc);
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        kiero::shutdown();
        isInitialized = false;
    }
}
