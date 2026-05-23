/**
 * XMenu
 * 作者：鼠子(YuiNijika)
 * 版本：v0.0.1-alpha1
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
#include "CPad.h"
#include "utils/Log.h"
#include "CHud.h"

const char* XMENU_VERSION = "v0.0.1-alpha1";
const char* XMENU_AUTHOR = "鼠子(YuiNijika)";
const char* XMENU_URL = "https://gtamodx.com/mods/xmenu";
const char* XMENU_GITHUB = "https://github.com/YuiNijika/XMenu";
const char* XMENU_QQ_GROUP = "https://gtamodx.com/qqun";

namespace {
    bool xMenuActive = false;

    void ShowD3DHookFailedMessage() {
#ifdef GTASA
        CHud::SetHelpMessage("XMenu: D3D Hook 初始化失败, 菜单已禁用", true, false, false);
#elif GTAVC
        static const wchar_t message[] = L"XMenu: D3D Hook 初始化失败, 菜单已禁用";
        CHud::SetHelpMessage(message, true, false);
#else
        static wchar_t message[] = L"XMenu: D3D Hook 初始化失败, 菜单已禁用";
        CHud::SetHelpMessage(message, true);
#endif
    }

    void InitXMenu() {
        Log::Info("注册游戏初始化与脚本循环事件");
        plugin::Events::initGameEvent.after += []() {
            Log::Info("游戏初始化事件触发，开始初始化功能模块");
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
                Log::Error("D3D Hook 初始化失败, XMenu 已停止初始化");
                return;
            }
        };

        plugin::Events::processScriptsEvent += []() {
            if (!xMenuActive) {
                return;
            }

            GameLogic::Process();
            Menu::Process();

            static bool lastKeyState = false;
            const bool currentKeyState = (GetKeyState('M') & 0x8000) != 0;

            if (currentKeyState && !lastKeyState) {
                D3DHook::SetMenuVisible(!D3DHook::IsMenuVisible());
            }
            lastKeyState = currentKeyState;

            CPad* pad = CPad::GetPad(0);
            if (pad) {
                pad->DisablePlayerControls = D3DHook::IsMenuVisible();
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