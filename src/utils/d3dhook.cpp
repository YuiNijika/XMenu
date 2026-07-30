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
#include "ui/GuiTheme.h"
#include "ui/MenuState.h"
#include <array>
#include <cstring>
#include <string>
#include <cstdint>

#ifndef WM_INPUT
#define WM_INPUT 0x00FF
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC        (-4)
#endif

// Forward declaration for ImGui WndProc handler.
// Note: The declaration is intentionally #if 0'd in imgui_impl_win32.h (to avoid pulling <windows.h> into the header).
// We must provide our own forward declaration (as recommended by the header comments).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
typedef HRESULT(__stdcall* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(__stdcall* Present_t)(LPDIRECT3DDEVICE9, const RECT*, const RECT*, HWND, const RGNDATA*);

static EndScene_t oEndScene = NULL;
static Reset_t oReset = NULL;
static Present_t oPresent = NULL;

extern const bool XMENU_DEBUG_MODE;

namespace {
    void DebugD3D(const std::string& message) {
        if (XMENU_DEBUG_MODE) {
            Log::Info(std::string("D3D Hook 调试：") + message);
        }
    }

    std::string StatusText(int status) {
        return std::to_string(status);
    }

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

            ImWchar ranges[] = {
                0x0020, 0x00FF,
                0x0400, 0x044F,
                0x0490, 0x052F,
                0x3000, 0x30FF,
                0x31F0, 0x31FF,
                0x4E00, 0x9FFF,
                0x3400, 0x4DBF,
                0xF900, 0xFAFF,
                0xFF00, 0xFFEF,
                0,
            };

            if (io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, nullptr, ranges)) {
                Log::Info(std::string("多语言字体加载成功: ") + font.displayName + " (" + fontPath + ")");
                return true;
            }

            Log::Warn(std::string("字体文件存在但加载失败: ") + font.displayName + " (" + fontPath + ")");
        }

        Log::Warn(std::string("未找到可用字体候选: ") + fontsDir);
        return false;
    }

    void ReleaseWindowInput() {
        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().MouseDrawCursor = false;
        }
    }

    bool IsWindowUsable(HWND hwnd) {
        return hwnd && IsWindow(hwnd);
    }

    // 系统光标（窗口客户区）→ ImGui DisplaySize
    // SA 宽屏/非 1:1 时客户区像素与渲染尺寸不一致会导致点击偏移；III/VC 通常 1:1
    void SyncSystemCursorToImGui(HWND hwnd) {
        if (!hwnd || !ImGui::GetCurrentContext()) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.DisplaySize.x <= 1.0f || io.DisplaySize.y <= 1.0f) {
            return;
        }

        POINT cursor{};
        if (!GetCursorPos(&cursor) || !ScreenToClient(hwnd, &cursor)) {
            return;
        }

        RECT client{};
        if (!GetClientRect(hwnd, &client)) {
            return;
        }

        const float clientW = static_cast<float>(client.right - client.left);
        const float clientH = static_cast<float>(client.bottom - client.top);
        if (clientW <= 1.0f || clientH <= 1.0f) {
            return;
        }

        const float scaleX = io.DisplaySize.x / clientW;
        const float scaleY = io.DisplaySize.y / clientH;
        float x = static_cast<float>(cursor.x) * scaleX;
        float y = static_cast<float>(cursor.y) * scaleY;

        if (x < 0.0f) x = 0.0f;
        if (y < 0.0f) y = 0.0f;
        if (x > io.DisplaySize.x - 1.0f) x = io.DisplaySize.x - 1.0f;
        if (y > io.DisplaySize.y - 1.0f) y = io.DisplaySize.y - 1.0f;

        io.AddMousePosEvent(x, y);
    }

    void ApplyMousePatch(bool menuActive) {
        if (menuActive) {
            plugin::patch::SetUChar((std::uintptr_t)BY_GAME(0x6194A0, 0x6020A0, 0x580D20), 0xC3);
            plugin::patch::Nop((std::uintptr_t)BY_GAME(0x541DD7, 0x4AB6CA, 0x49272F), 5);
#ifdef GTASA
            plugin::patch::SetUChar(0x4EB731, 0xEB);
            plugin::patch::SetUChar(0x4EB75A, 0xEB);
#endif
            return;
        }

        plugin::patch::SetUChar((std::uintptr_t)BY_GAME(0x6194A0, 0x6020A0, 0x580D20), BY_GAME(0xE9, 0x53, 0x53));
#ifdef GTASA
        plugin::patch::SetRaw(0x541DD7, (void*)"\xE8\xE4\xD5\xFF\xFF", 5);
        plugin::patch::SetUChar(0x4EB731, 0x74);
        plugin::patch::SetUChar(0x4EB75A, 0x74);
#elif GTAVC
        plugin::patch::SetRaw(0x4AB6CA, (void*)"\xE8\x51\x21\x00\x00", 5);
#else
        plugin::patch::SetRaw(0x49272F, (void*)"\xE8\x6C\xF5\xFF\xFF", 5);
#endif
    }
}

HWND D3DHook::window = NULL;
WNDPROC D3DHook::oWndProc = NULL;
bool D3DHook::menuVisible = false;
bool D3DHook::isInitialized = false;
bool D3DHook::hookInstalled = false;
bool D3DHook::initFailed = false;
bool D3DHook::backgroundInputActive = false;
bool D3DHook::backgroundRenderActive = false;
LPDIRECT3DDEVICE9 D3DHook::device = nullptr;
const char* D3DHook::initStatus = "not initialized";
std::function<void()> D3DHook::renderCallback = nullptr;
static float g_rawWheelDelta = 0.0f;

bool D3DHook::IsMenuVisible() {
    return menuVisible;
}

void D3DHook::ToggleMenu() {
    SetMenuVisible(!menuVisible);
}

LPDIRECT3DDEVICE9 D3DHook::GetDevice() {
    return device;
}

bool D3DHook::IsInitialized() {
    return isInitialized;
}

bool D3DHook::IsReady() {
    return hookInstalled;
}

bool D3DHook::HadInitFailure() {
    return initFailed;
}

const char* D3DHook::GetInitStatus() {
    return initStatus;
}

const char* D3DHook::GetStatusText() {
    return initStatus;
}

void D3DHook::SetBackgroundInputActive(bool active) {
    if (backgroundInputActive == active) {
        return;
    }

    backgroundInputActive = active;
    if (isInitialized) {
        ProcessMouse();
    }
}

bool D3DHook::IsBackgroundInputActive() {
    return backgroundInputActive;
}

void D3DHook::SetBackgroundRenderActive(bool active) {
    backgroundRenderActive = active;
}

bool D3DHook::IsBackgroundRenderActive() {
    return backgroundRenderActive;
}

float D3DHook::ConsumeRawWheelDelta() {
    const float value = g_rawWheelDelta;
    g_rawWheelDelta = 0.0f;
    return value;
}

void D3DHook::SetMenuVisible(bool visible) {
    if (!hookInstalled) {
        menuVisible = false;
        ReleaseWindowInput();
        return;
    }

    if (menuVisible == visible) {
        return;
    }

    menuVisible = visible;

    if (isInitialized) {
        ProcessMouse();
        return;
    }

    if (!menuVisible) {
        ReleaseWindowInput();
    }
}

void D3DHook::ProcessMouse() {
    if (!ImGui::GetCurrentContext()) {
        if (!menuVisible) {
            ReleaseWindowInput();
        }
        return;
    }

    bool showCursor = false;
    if (backgroundInputActive) {
        showCursor = true;
    } else if (menuVisible) {
        showCursor = GuiTheme::WantsMouseCursor();
    }
    ImGui::GetIO().MouseDrawCursor = showCursor;

    if (menuVisible || backgroundInputActive) {
        ApplyMousePatch(true);
    } else {
        ApplyMousePatch(false);
        CPad::UpdatePads();
        CPad::NewMouseControllerState.x = 0;
        CPad::NewMouseControllerState.y = 0;
#ifndef GTA3
        CPad* pad = CPad::GetPad(0);
        if (pad) {
            pad->NewState.DPadUp = 0;
            pad->OldState.DPadUp = 0;
            pad->NewState.DPadDown = 0;
            pad->OldState.DPadDown = 0;
        }
#endif
        ReleaseWindowInput();
    }
}

void D3DHook::MaintainInputState() {
    if (hookInstalled && isInitialized && (menuVisible || backgroundInputActive)) {
        const bool showCursor = backgroundInputActive || (menuVisible && GuiTheme::WantsMouseCursor());
        ApplyMousePatch(true);
        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().MouseDrawCursor = showCursor;
            GuiTheme::Sync();
        }
    }
}

LRESULT __stdcall D3DHook::hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if ((menuVisible || backgroundInputActive) && ImGui::GetCurrentContext()) {
        // 与 III/VC 相同：Win32 绝对客户区坐标交给 ImGui
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP
            || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_MBUTTONDOWN
            || uMsg == WM_MBUTTONUP || uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEHWHEEL
            || uMsg == WM_INPUT) {
            return 1;
        }
    } else if (!(menuVisible || backgroundInputActive)) {
        if (uMsg == WM_MOUSEWHEEL) {
            g_rawWheelDelta += static_cast<float>(static_cast<short>(HIWORD(wParam))) / static_cast<float>(WHEEL_DELTA);
        }
#ifdef GTASA
        else if (uMsg == WM_INPUT) {
            RAWINPUT raw;
            UINT size = sizeof(raw);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == sizeof(raw)
                && raw.header.dwType == RIM_TYPEMOUSE
                && (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)) {
                g_rawWheelDelta += static_cast<float>(static_cast<SHORT>(raw.data.mouse.usButtonData)) / static_cast<float>(WHEEL_DELTA);
            }
        }
#endif
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

    GuiTheme::SetThemeByIndex(MenuState::GuiThemeIndex);
    GuiTheme::SetInteractionByIndex(MenuState::GuiInteractionMode);
    GuiTheme::ApplyToStyle();
    GuiTheme::ApplyInteraction();

    isInitialized = true;
    ProcessMouse();
    Log::Info("ImGui 初始化完成");
}

HRESULT __stdcall D3DHook::hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
    device = pDevice;
    if (!isInitialized) {
        D3DDEVICE_CREATION_PARAMETERS params;
        pDevice->GetCreationParameters(&params);
        window = params.hFocusWindow;
        if (!IsWindowUsable(window)) {
            initFailed = true;
            initStatus = "D3D9 Hook failed: invalid game window";
            Log::Error("D3D Hook 初始化失败：无法获取有效游戏窗口");
            menuVisible = false;
            ReleaseWindowInput();
            return oEndScene(pDevice);
        }
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
        InitImGui(pDevice);
    }

    if (isInitialized && (menuVisible || backgroundRenderActive || backgroundInputActive)) {
        if (menuVisible || backgroundInputActive) {
            ApplyMousePatch(true);
        }
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        // 用系统光标按客户区→DisplaySize 重映射，消除 SA 点击偏移
        if (menuVisible || backgroundInputActive) {
            SyncSystemCursorToImGui(window);
        }

        ImGui::NewFrame();
        GuiTheme::Sync();

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
    hookInstalled = false;
    initFailed = false;
    initStatus = "initializing";
    DebugD3D("开始初始化 kiero D3D9");
    const auto initStatusCode = kiero::init(kiero::RenderType::D3D9);
    DebugD3D(std::string("kiero::init 状态=") + StatusText(static_cast<int>(initStatusCode)));
    if (initStatusCode == kiero::Status::Success) {
        const auto endSceneStatus = kiero::bind(42, (void**)&oEndScene, (void*)hkEndScene);
        const auto resetStatus = kiero::bind(16, (void**)&oReset, (void*)hkReset);
        const auto presentStatus = kiero::bind(17, (void**)&oPresent, (void*)hkPresent);
        DebugD3D(std::string("bind EndScene(42) 状态=") + StatusText(static_cast<int>(endSceneStatus)));
        DebugD3D(std::string("bind Reset(16) 状态=") + StatusText(static_cast<int>(resetStatus)));
        DebugD3D(std::string("bind Present(17) 状态=") + StatusText(static_cast<int>(presentStatus)));

        if (endSceneStatus == kiero::Status::Success && resetStatus == kiero::Status::Success && presentStatus == kiero::Status::Success) {
            hookInstalled = true;
            initStatus = "D3D9 Hook ready";
            Log::Info("D3D9 Hook 绑定完成");
            return true;
        }

        hookInstalled = false;
        initFailed = true;
        menuVisible = false;
        ReleaseWindowInput();
        initStatus = "D3D9 Hook binding failed";
        Log::Error("D3D9 Hook 绑定失败");
        DebugD3D("绑定失败，已执行 kiero::shutdown");
        kiero::shutdown();
        return false;
    }

    hookInstalled = false;
    initFailed = true;
    menuVisible = false;
    ReleaseWindowInput();
    initStatus = "D3D9 Hook init failed: GTA III/VC need D3D8to9; SA needs D3D9 render path";
    Log::Error("kiero D3D9 初始化失败：GTA III/VC 请确认已安装 D3D8to9 wrapper；SA 请确认当前渲染路径为 D3D9。菜单渲染不可用，但脚本逻辑继续执行");
    DebugD3D("初始化失败：GTA III/VC 通常需要 D3D8to9 wrapper；SA 需要确认当前渲染路径为 D3D9");
    return false;
}

void D3DHook::Shutdown() {
    menuVisible = false;
    ReleaseWindowInput();

    if (isInitialized) {
        Log::Info("关闭 D3D Hook 与 ImGui");
        SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        isInitialized = false;
    }

    if (hookInstalled) {
        kiero::shutdown();
        hookInstalled = false;
    }

    device = nullptr;
    menuVisible = false;
}