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
#include "utils/Log.h"
#include <array>
#include <string>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
typedef HRESULT(__stdcall* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(__stdcall* Present_t)(LPDIRECT3DDEVICE9, const RECT*, const RECT*, HWND, const RGNDATA*);

static EndScene_t oEndScene = NULL;
static Reset_t oReset = NULL;
static Present_t oPresent = NULL;

namespace {
    bool FileExists(const std::string& path) {
        const DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool LoadChineseFont(ImGuiIO& io) {
        char windowsPath[MAX_PATH] = {};
        const UINT pathSize = GetWindowsDirectoryA(windowsPath, MAX_PATH);
        if (pathSize == 0 || pathSize >= MAX_PATH) {
            Log::Warn("获取 Windows 字体目录失败，将使用 ImGui 默认字体");
            return false;
        }

        const std::string fontsDir = std::string(windowsPath) + "\\Fonts\\";

        struct FontCandidate {
            const char* fileName;
            const char* displayName;
        };

        // 从常见中文字体一路兜底，尽量保证精简系统或定制系统也能正常显示中文。
        const std::array<FontCandidate, 16> fontCandidates = {{
            {"msyh.ttc", "微软雅黑"},
            {"msyh.ttf", "微软雅黑"},
            {"msyhbd.ttc", "微软雅黑 Bold"},
            {"simhei.ttf", "黑体"},
            {"simsun.ttc", "宋体"},
            {"simfang.ttf", "仿宋"},
            {"simkai.ttf", "楷体"},
            {"Deng.ttf", "等线"},
            {"Dengb.ttf", "等线 Bold"},
            {"STZHONGS.TTF", "华文中宋"},
            {"STSONG.TTF", "华文宋体"},
            {"STXIHEI.TTF", "华文细黑"},
            {"NotoSansCJK-Regular.ttc", "Noto Sans CJK"},
            {"SourceHanSansCN-Regular.otf", "思源黑体"},
            {"SourceHanSerifCN-Regular.otf", "思源宋体"},
            {"arialuni.ttf", "Arial Unicode MS"},
        }};

        for (const auto& font : fontCandidates) {
            const std::string fontPath = fontsDir + font.fileName;
            if (!FileExists(fontPath)) {
                continue;
            }

            if (io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull())) {
                Log::Info(std::string("中文字体加载成功: ") + font.displayName + " (" + fontPath + ")");
                return true;
            }

            Log::Warn(std::string("中文字体文件存在但加载失败: ") + font.displayName + " (" + fontPath + ")");
        }

        Log::Warn(std::string("未找到可用中文字体目录候选: ") + fontsDir);
        return false;
    }
}

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
    
    if (!LoadChineseFont(io)) {
        Log::Warn("没有找到可用中文字体，将使用 ImGui 默认字体");
    }
    
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(pDevice);
    
    isInitialized = true;
    Log::Info("ImGui 初始化完成");
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
    const auto initStatus = kiero::init(kiero::RenderType::D3D9);
    if (initStatus == kiero::Status::Success) {
        const auto endSceneStatus = kiero::bind(42, (void**)&oEndScene, (void*)hkEndScene);
        const auto resetStatus = kiero::bind(16, (void**)&oReset, (void*)hkReset);
        const auto presentStatus = kiero::bind(17, (void**)&oPresent, (void*)hkPresent);

        if (endSceneStatus == kiero::Status::Success && resetStatus == kiero::Status::Success && presentStatus == kiero::Status::Success) {
            Log::Info("D3D9 Hook 绑定完成");
            return true;
        }

        Log::Error("D3D9 Hook 绑定失败");
        kiero::shutdown();
        return false;
    }

    Log::Error("kiero D3D9 初始化失败");
    return false;
}

void D3DHook::Shutdown() {
    if (isInitialized) {
        Log::Info("关闭 D3D Hook 与 ImGui");
        SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)oWndProc);
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        kiero::shutdown();
        isInitialized = false;
    }
}
