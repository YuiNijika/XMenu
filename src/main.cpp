/**
 * XMenu
 * 作者：鼠子(YuiNijika)
 * 测试：枫林、狂风晨、IIScar
 * 版本：v0.0.1-alpha2
 * 网站：https://gtamodx.com/mods/xmenu
 * GitHub：https://github.com/YuiNijika/XMenu
 * QQ群：https://gtamodx.com/qqun
 * 声明：永久免费禁止倒卖，禁止用于商业用途。
 * 若有任何问题, 请加入QQ群或前往GitHub发布issue反馈。
 */

#include <windows.h>
#include "plugin.h"
#include "app/Startup.h"
#include "utils/D3DHook.h"
#include "ui/Menu.h"
#include "features/GameLogic.h"
#include "resources/ResourceData.h"
#include "CPad.h"
#include "utils/Log.h"
#include "utils/I18n.h"
#include "utils/UpdateChecker.h"
#include "utils/AppConfig.h"
#include "CHud.h"

extern const bool XMENU_DEBUG_MODE = false;
const char* XMENU_VERSION = "v0.0.1-alpha2";
const char* XMENU_AUTHOR = "鼠子(YuiNijika)";
const char* XMENU_AUTHOR_TEST = "枫林、狂风晨、IIScar";
const char* XMENU_URL = "https://gtamodx.com/mods/xmenu";
const char* XMENU_GITHUB = "https://github.com/YuiNijika/XMenu";
const char* XMENU_GITHUB_API = "https://api.github.com/repos/YuiNijika/XMenu/releases/latest";
const char* XMENU_QQ_GROUP = "https://gtamodx.com/qqun";
const char* XMENU_TECH_STACK = "C++ / DirectX 9 / Windows API";
const char* XMENU_OPEN_SOURCE_LIBS = "Dear ImGui, kiero, MinHook, plugin-sdk";

namespace {
    bool xMenuActive = false;

    void ShowD3DHookFailedMessage() {
#ifdef GTASA
        CHud::SetHelpMessage("XMenu: D3D Hook 初始化失败, 菜单渲染不可用", true, false, false);
#elif GTAVC
        static const wchar_t message[] = L"XMenu: D3D9 Hook 初始化失败, 请安装 D3D8to9 wrapper";
        CHud::SetHelpMessage(message, true, false);
#else
        static wchar_t message[] = L"XMenu: D3D9 Hook 初始化失败, 请安装 D3D8to9 wrapper";
        CHud::SetHelpMessage(message, true);
#endif
    }

    void InitXMenu() {
        Log::Info("注册游戏初始化与脚本循环事件");
        plugin::Events::initGameEvent.after += []() {
            Log::Info("游戏初始化事件触发，开始初始化功能模块");
            I18n::Init();
            AppConfig::Init();
            Resources::InitData();  // 初始化游戏数据
            UpdateChecker::Start(XMENU_GITHUB_API, XMENU_VERSION);
            GameLogic::Init();
            const bool hookReady = D3DHook::Init([]() {
                Menu::Draw();
            });
 
            if (hookReady) {
                xMenuActive = true;
                Log::Info("D3D Hook 初始化成功");
            } else {
                xMenuActive = false;
                D3DHook::SetMenuVisible(false);
                ShowD3DHookFailedMessage();
                Log::Error("D3D9 Hook 初始化失败，GTA III/VC 请确认已安装 D3D8to9 wrapper；菜单渲染不可用，脚本逻辑继续执行");
            }
        };

        plugin::Events::processScriptsEvent += []() {
            GameLogic::Process();
            Menu::Process();
            D3DHook::MaintainInputState();

            if (!xMenuActive) {
                CPad* pad = CPad::GetPad(0);
                if (pad) {
                    pad->DisablePlayerControls = false;
                }

                if (XMENU_DEBUG_MODE && D3DHook::HadInitFailure()) {
                    static unsigned int hintCooldown = 0;
                    if (hintCooldown++ % 1800 == 0) {
                        ShowD3DHookFailedMessage();
                    }
                }
                return;
            }

            static bool lastHotkeyState = false;
            const bool currentHotkeyState = AppConfig::IsMenuHotkeyPressed();

            if (currentHotkeyState && !lastHotkeyState) {
                D3DHook::SetMenuVisible(!D3DHook::IsMenuVisible());
            }
            lastHotkeyState = currentHotkeyState;

            CPad* pad = CPad::GetPad(0);
            if (pad) {
                pad->DisablePlayerControls = D3DHook::IsInitialized() && D3DHook::IsMenuVisible();
            }
        };
    }
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved) {
    if (nReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hDllHandle);
        Log::Init();
        Log::Info("DLL 已加载，开始启动校验");
        if (Startup::Validate()) {
            Log::Info("启动校验通过");
            InitXMenu();
        } else {
            Log::Error("启动校验失败, XMenu 已停止初始化");
        }
    } else if (nReason == DLL_PROCESS_DETACH) {
        D3DHook::Shutdown();
        Log::Shutdown();
    }
    return TRUE;
}