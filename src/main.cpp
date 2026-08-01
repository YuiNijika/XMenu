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

#include <string>

#include "app/Startup.h"
#include "features/GameLogic.h"
#include "integration/XBaseBridge.h"
#include "ui/Menu.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"
#include "utils/Log.h"
#include "utils/UpdateChecker.h"

#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/Input.h>

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
XBase::Hooks::DrawCallbackId xMenuDrawCallbackId;

// -1=等待游戏初始化，0=配置，1=产品逻辑，2=渲染 Hook，3=收尾，4=就绪。
int bootstrapStage = -1;

void ShowD3DHookFailedMessage() {
    XBase::Host::ShowMessage(
        "XMenu: D3D9 Hook 初始化失败，菜单渲染不可用；GTA III/VC 请确认已安装 D3D8to9 wrapper");
}

void AdvanceBootstrap() {
    if (bootstrapStage < 0 || bootstrapStage >= 4) return;

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
        UpdateChecker::Start(XMENU_VERSION);
        GameLogic::Init();
        XBaseBridge::Init();
        bootstrapStage = 2;
        return;
    }

    if (bootstrapStage == 2) {
        Log::Info("分帧初始化[2]: D3D Hook");
        xMenuDrawCallbackId = XBase::Hooks::RegisterDrawCallback([]() {
            Menu::Draw();
        });
        const bool hookReady = static_cast<bool>(xMenuDrawCallbackId) && XBase::Hooks::Init();

        if (hookReady) {
            xMenuActive = true;
            Log::Info("D3D Hook 初始化成功");
        } else {
            if (xMenuDrawCallbackId) {
                XBase::Hooks::UnregisterDrawCallback(xMenuDrawCallbackId);
                xMenuDrawCallbackId = {};
            }
            xMenuActive = false;
            XBase::Hooks::SetMenuVisible(false);
            ShowD3DHookFailedMessage();
            Log::Error("D3D9 Hook 初始化失败；菜单渲染不可用，脚本逻辑继续执行");
        }
        bootstrapStage = 3;
        return;
    }

    bootstrapStage = 4;
    Log::Info("分帧初始化完成");
}

void OnGameInit() {
    Log::Info("游戏初始化事件触发（轻量阶段）");
    XBaseBridge::NotifyGameInit();
    bootstrapStage = 0;
}

void OnProcess() {
    if (bootstrapStage < 0) return;

    if (bootstrapStage < 4) {
        if (bootstrapStage >= 1 && !XBaseBridge::IsWorldReady()) return;
        AdvanceBootstrap();
        if (bootstrapStage < 4) return;
    }

    if (!XBaseBridge::IsWorldReady()) return;

    Menu::Process();
    XBaseBridge::Process();
    XBase::Hooks::MaintainInputState();

    if (!xMenuActive) {
        if (XMENU_DEBUG_MODE && XBase::Hooks::HadInitFailure()) {
            static unsigned int hintCooldown = 0;
            if (hintCooldown++ % 1800 == 0) ShowD3DHookFailedMessage();
        }
        return;
    }

    if (XBase::Input::WasPressed(AppConfig::GetMenuHotkey())) {
        XBase::Hooks::SetMenuVisible(!XBase::Hooks::IsMenuVisible());
    }
}

bool InitXMenu() {
    Log::Info("注册 XBase Host 生命周期（帧驱动延迟到首次游戏初始化后）");
    return XBase::Host::Install({OnGameInit, OnProcess});
}

} // namespace

extern "C" void XMenuPayloadAttach() {
    Log::Init();
    Log::Info("DLL 已加载，开始启动校验");
    if (Startup::Validate() && InitXMenu()) {
        Log::Info("启动校验与 Host 注册通过");
    } else {
        Log::Error("启动校验或 Host 注册失败，XMenu 已停止初始化");
    }
}

extern "C" void XMenuPayloadDetach() {
    XBase::Host::Shutdown();
    XBaseBridge::Shutdown();
    if (xMenuDrawCallbackId) {
        XBase::Hooks::UnregisterDrawCallback(xMenuDrawCallbackId);
        xMenuDrawCallbackId = {};
    }
    XBase::Hooks::Shutdown();
    Log::Shutdown();
}