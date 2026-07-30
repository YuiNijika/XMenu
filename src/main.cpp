/**
 * --------------------------------------------------------------------------------------------
 * XMenu
 * 作者：鼠子(YuiNijika)
 * 版本：@XMENU_VERSION
 * 网站：https://gtamodx.com/mods/xmenu
 * GitHub：https://github.com/YuiNijika/XMenu
 * QQ群：https://gtamodx.com/qqun
 * 声明：永久免费禁止倒卖，禁止用于商业用途。
 * 若有任何问题, 请加入QQ群或前往GitHub发布issue反馈。
 * --------------------------------------------------------------------------------------------
 */

#include <windows.h>
#include "plugin.h"
#include "app/Startup.h"
#include "utils/D3DHook.h"
#include "ui/Menu.h"
#include "features/GameLogic.h"
#include "CPad.h"
#include "utils/Log.h"
#include "utils/I18n.h"
#include "utils/UpdateChecker.h"
#include "utils/AppConfig.h"
#include "CHud.h"

#ifdef GTASA
#include "fla/Main.h"
#endif
#include "integration/XBaseBridge.h"

extern const bool XMENU_DEBUG_MODE = false;
const char* XMENU_VERSION = "v0.1.0-alpha1";
const char* XMENU_AUTHOR = "鼠子(YuiNijika)";
const char* XMENU_AUTHOR_TEST = "枫林、狂风晨、IIScar、Happy";
const char* XMENU_URL = "https://gtamodx.com/mods/xmenu";
const char* XMENU_GITHUB = "https://github.com/YuiNijika/XMenu";
const char* XMENU_GITHUB_API = "https://api.github.com/repos/YuiNijika/XMenu/releases/latest";
const char* XMENU_QQ_GROUP = "https://gtamodx.com/qqun";
const char* XMENU_TECH_STACK = "C++ / DirectX 9 / Windows API";
const char* XMENU_OPEN_SOURCE_LIBS = "Dear ImGui, kiero, MinHook, plugin-sdk";

namespace {
    bool xMenuActive = false;

    // -1=等 initGame 0=i18n+config 1=逻辑/更新 2=D3D 3=收尾 4=就绪
    // 分帧做重活，避免 initGame 一帧内同步扫盘/装 hook 造成明显卡顿
    int bootstrapStage = -1;

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

    void AdvanceBootstrap() {
        if (bootstrapStage < 0 || bootstrapStage >= 4) {
            return;
        }

        static bool coreInited = false;
        if (bootstrapStage == 0) {
            if (!coreInited) {
                Log::Info("分帧初始化[0]: I18n + AppConfig");
                I18n::Init();
                AppConfig::Init();
                coreInited = true;
            }
            bootstrapStage = 1;
            return;
        }

        if (bootstrapStage == 1) {
            Log::Info("分帧初始化[1]: GameLogic + UpdateChecker");
            // 资源 JSON 按需懒加载，不在启动路径全量扫盘
            UpdateChecker::Start(XMENU_VERSION);
            GameLogic::Init();
            XBaseBridge::Init();
            bootstrapStage = 2;
            return;
        }

        if (bootstrapStage == 2) {
            Log::Info("分帧初始化[2]: D3D Hook");
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
            bootstrapStage = 3;
            return;
        }

        bootstrapStage = 4;
        Log::Info("分帧初始化完成");
    }

    void InitXMenu() {
        Log::Info("注册游戏初始化事件（驱动订阅延迟到initGame后，以降低CLEO/Init窗口冲突）");
        plugin::Events::initGameEvent.after += []() {
            // 仅放必须尽早、且相对轻量的工作；重 IO / D3D 放到脚本帧分摊
            Log::Info("游戏初始化事件触发（轻量阶段）");
#ifdef GTASA
            CFastman92limitAdjuster::Init();
            Log::Info("FLA 初始化完成");
#endif
            // 新游戏/读档：重置 III 就绪门闩。不要用 initScriptsEvent（与 CLEO 抢 0x48C26B 等）
            GameLogic::NotifyGameInit();
            XBaseBridge::NotifyGameInit();
            bootstrapStage = 0;

            // 延迟安装周期驱动：首次 initGame 后再挂钩，避免在首个 CTheScripts::Init 窗口打补丁
            static bool driverInstalled = false;
            if (!driverInstalled) {
                driverInstalled = true;

                // 将原 processScripts 循环体提取为可复用 lambda
                static auto s_driver = []() {
                    if (bootstrapStage < 0) {
                        return;
                    }

                    // I18n/配置不碰游戏堆；可在池未就绪时推进
                    // GameLogic::Init / Process / Menu 必须等 III 池与玩家稳定
                    if (bootstrapStage < 4) {
                        if (bootstrapStage >= 1 && !GameLogic::IsWorldReady()) {
                            return;
                        }
                        AdvanceBootstrap();
                        // 启动帧只推进一个阶段，尽快把控制权交回游戏
                        if (bootstrapStage < 4) {
                            return;
                        }
                    }

                    if (!GameLogic::IsWorldReady()) {
                        return;
                    }
                    if (!XBaseBridge::IsWorldReady()) {
                        return;
                    }

                    GameLogic::Process();
                    Menu::Process();
                    XBaseBridge::Process();
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

                // III：一律 gameProcessEvent。
                // processScripts 与任务过场/脚本装载同窗；无 CLEO 时 0x5ABD80（RW 流读空）也会在过场加载爆
#ifdef GTA3
                plugin::Events::gameProcessEvent += s_driver;
                Log::Info("III：使用 gameProcessEvent 驱动主循环（避开 processScripts，降低 Init/过场装载冲突）");
#else
                plugin::Events::processScriptsEvent += s_driver;
#endif
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
        XBaseBridge::Shutdown();
        D3DHook::Shutdown();
        Log::Shutdown();
    }
    return TRUE;
}