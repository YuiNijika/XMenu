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

            // 合并多个语言的字符范围：中文、日文、韩文、西里尔文（俄语）、默认拉丁文
            ImWchar ranges[] = {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement (English, etc.)
                0x0400, 0x044F, // Cyrillic (Russian)
                0x0490, 0x052F, // Cyrillic Supplement
                0x3000, 0x30FF, // Japanese Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Phonetic Extensions
                0x4E00, 0x9FFF, // CJK Unified Ideographs (Chinese, Japanese Kanji)
                0x3400, 0x4DBF, // CJK Unified Ideographs Extension A
                0xF900, 0xFAFF, // CJK Compatibility Ideographs
                0xFF00, 0xFFEF, // Full-width Characters
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
        ClipCursor(nullptr);
        ReleaseCapture();
        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().MouseDrawCursor = false;
        }
    }

    void ReleaseWindowCaptureOnly() {
        ClipCursor(nullptr);
        if (GetCapture()) {
            ReleaseCapture();
        }
    }

    bool IsWindowUsable(HWND hwnd) {
        return hwnd && IsWindow(hwnd);
    }

    bool IsGameWindowActive(HWND hwnd) {
        if (!IsWindowUsable(hwnd)) {
            return false;
        }

        const HWND foreground = GetForegroundWindow();
        return foreground == hwnd || IsChild(hwnd, foreground) || IsChild(foreground, hwnd);
    }

    void ClearGameMouseState() {
        CPad::NewMouseControllerState.x = 0;
        CPad::NewMouseControllerState.y = 0;
#ifdef GTASA
        CPad::OldMouseControllerState.x = 0;
        CPad::OldMouseControllerState.y = 0;
#endif
    }

    void ApplyMousePatch(bool menuActive) {
        if (menuActive) {
            plugin::patch::SetUChar((uintptr_t)BY_GAME(0x6194A0, 0x6020A0, 0x580D20), 0xC3);
            plugin::patch::Nop((uintptr_t)BY_GAME(0x541DD7, 0x4AB6CA, 0x49272F), 5);
            return;
        }

        plugin::patch::SetUChar((uintptr_t)BY_GAME(0x6194A0, 0x6020A0, 0x580D20), BY_GAME(0xE9, 0x53, 0x53));
#ifdef GTASA
        plugin::patch::SetRaw(0x541DD7, (void*)"\xE8\xE4\xD5\xFF\xFF", 5);
#elif GTAVC
        plugin::patch::SetRaw(0x4AB6CA, (void*)"\xE8\x51\x21\x00\x00", 5);
#else
        plugin::patch::SetRaw(0x49272F, (void*)"\xE8\x6C\xF5\xFF\xFF", 5);
#endif
    }

    float ClampFloat(float value, float minValue, float maxValue) {
        if (value < minValue) {
            return minValue;
        }
        if (value > maxValue) {
            return maxValue;
        }
        return value;
    }

    void SyncImGuiMousePosition(HWND hwnd) {
        if (!ImGui::GetCurrentContext() || !IsWindowUsable(hwnd)) {
            return;
        }

        RECT clientRect = {};
        if (!GetClientRect(hwnd, &clientRect)) {
            return;
        }

#ifdef GTASA
        static bool virtualMouseReady = false;
        static ImVec2 virtualMousePos(0.0f, 0.0f);

        const float width = static_cast<float>(clientRect.right - clientRect.left);
        const float height = static_cast<float>(clientRect.bottom - clientRect.top);
        if (width <= 0.0f || height <= 0.0f) {
            return;
        }

        if (!virtualMouseReady) {
            POINT cursor = {};
            if (GetCursorPos(&cursor) && ScreenToClient(hwnd, &cursor)
                && cursor.x >= 0 && cursor.y >= 0
                && cursor.x < static_cast<int>(width)
                && cursor.y < static_cast<int>(height)) {
                virtualMousePos = ImVec2(static_cast<float>(cursor.x), static_cast<float>(cursor.y));
            } else {
                virtualMousePos = ImVec2(width * 0.5f, height * 0.5f);
            }
            virtualMouseReady = true;
        }

        const float dx = static_cast<float>(CPad::NewMouseControllerState.x);
        const float dy = static_cast<float>(CPad::NewMouseControllerState.y);
        if (dx != 0.0f || dy != 0.0f) {
            virtualMousePos.x = ClampFloat(virtualMousePos.x + dx, 0.0f, width - 1.0f);
            virtualMousePos.y = ClampFloat(virtualMousePos.y + dy, 0.0f, height - 1.0f);
        }

        ImGui::GetIO().MousePos = virtualMousePos;
#else
        POINT cursor = {};
        if (!GetCursorPos(&cursor)) {
            return;
        }

        ScreenToClient(hwnd, &cursor);
        ImGui::GetIO().MousePos = ImVec2(static_cast<float>(cursor.x), static_cast<float>(cursor.y));
#endif
    }
}

HWND D3DHook::window = NULL;
WNDPROC D3DHook::oWndProc = NULL;
bool D3DHook::menuVisible = false;
bool D3DHook::isInitialized = false;
bool D3DHook::hookInstalled = false;
bool D3DHook::initFailed = false;
LPDIRECT3DDEVICE9 D3DHook::device = nullptr;
const char* D3DHook::initStatus = "not initialized";
std::function<void()> D3DHook::renderCallback = nullptr;

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

void D3DHook::SetMenuVisible(bool visible) {
    if (!hookInstalled) {
        menuVisible = false;
        ReleaseWindowInput();
        return;
    }

    const bool changed = menuVisible != visible;
    menuVisible = visible;

    if (isInitialized) {
        ProcessMouse();
        return;
    }

    if (!menuVisible || changed) {
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

    ImGui::GetIO().MouseDrawCursor = menuVisible;

    if (menuVisible) {
        ApplyMousePatch(true);
        ClearGameMouseState();
    } else {
        ApplyMousePatch(false);
        // 清除鼠标历史记录防止视角跳变
        CPad::UpdatePads();
        ClearGameMouseState();
        ReleaseWindowInput();
#ifdef GTA3
        // CPad::GetPad(0)->ClearMouseHistory(); // Disabled to prevent camera reset
#else
        // CPad::ClearMouseHistory(); // Disabled to prevent camera reset
        CPad* pad = CPad::GetPad(0);
        if (pad) {
            pad->NewState.DPadUp = 0;
            pad->OldState.DPadUp = 0;
            pad->NewState.DPadDown = 0;
            pad->OldState.DPadDown = 0;
        }
#endif
    }
}

void D3DHook::MaintainInputState() {
    const bool active = IsGameWindowActive(window);

    if (hookInstalled && isInitialized && menuVisible && active) {
        ApplyMousePatch(true);
        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().MouseDrawCursor = true;
            SyncImGuiMousePosition(window);
        }
        ClearGameMouseState();
        return;
    }

    if (!hookInstalled || !isInitialized || !menuVisible || !active) {
        if (menuVisible && !active) {
            menuVisible = false;
            ProcessMouse();
            DebugD3D("窗口失焦，已关闭菜单并释放鼠标状态");
            return;
        }

        ReleaseWindowInput();
    }
}

LRESULT __stdcall D3DHook::hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KILLFOCUS || uMsg == WM_CANCELMODE || uMsg == WM_CAPTURECHANGED || uMsg == WM_ACTIVATE || uMsg == WM_ACTIVATEAPP) {
        const bool inactive =
            uMsg == WM_KILLFOCUS ||
            uMsg == WM_CANCELMODE ||
            uMsg == WM_CAPTURECHANGED ||
            (uMsg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) ||
            (uMsg == WM_ACTIVATEAPP && wParam == FALSE);

        if (inactive) {
            SetMenuVisible(false);
        }
    }

    if (menuVisible) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return true;
        }

        switch (uMsg) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_CHAR:
        case WM_SYSCHAR:
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
            return true;
        default:
            break;
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
            window = GetForegroundWindow();
        }
        if (!IsWindowUsable(window)) {
            initFailed = true;
            initStatus = "D3D9 Hook failed: invalid game window";
            Log::Error("D3D Hook 初始化失败：无法获取有效游戏窗口");
            menuVisible = false;
            ReleaseWindowInput();
            return oEndScene(pDevice);
        }
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)hkWndProc);
        InitImGui(pDevice);
    }
    
    if (isInitialized && menuVisible) {
        ApplyMousePatch(true);
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        SyncImGuiMousePosition(window);
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
        SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)oWndProc);
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
