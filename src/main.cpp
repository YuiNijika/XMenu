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
#include "controllers/BulletAssist.h"
#include "integration/XBaseBridge.h"
#include "ui/Menu.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"
#include "utils/Log.h"
#include "controllers/ReactUi.h"
#include "utils/BuildInfo.h"
#include "utils/UpdateChecker.h"

#include <XBase/Hooks.h>
#include <XBase/WebBridge.h>
#include <XBase/Host.h>
#include <XBase/Input.h>

extern const bool XMENU_DEBUG_MODE = false;
const char* XMENU_VERSION = BuildInfo::Version;
const char* XMENU_AUTHOR = BuildInfo::Author;
const char* XMENU_AUTHOR_TEST = "枫林、狂风晨、IIScar、Happy";
const char* XMENU_URL = BuildInfo::Url;
const char* XMENU_GITHUB = "https://github.com/YuiNijika/XMenu";
const char* XMENU_GITHUB_API = "https://api.github.com/repos/YuiNijika/XMenu/releases/latest";
const char* XMENU_QQ_GROUP = "https://gtamodx.com/qqun";
const char* XMENU_TECH_STACK = "C++ / XBase Runtime API";
const char* XMENU_OPEN_SOURCE_LIBS = "XBase";

namespace {

bool xMenuActive = false;
XBase::Hooks::DrawCallbackId xMenuDrawCallbackId;

// -1=等待游戏初始化，0=配置，1=产品逻辑，2=渲染 Hook，3=收尾，4=就绪。
int bootstrapStage = -1;

void ShowRenderBackendFailedMessage() {
    XBase::Host::ShowMessage("XMenu: XBase 渲染后端初始化失败，菜单渲染不可用");
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
        Log::Info("分帧初始化[1]: XBase + UpdateChecker");
        UpdateChecker::Start(XMENU_VERSION);
        XBaseBridge::Init();
        Controllers::BulletAssist::Init();
        bootstrapStage = 2;
        return;
    }

    if (bootstrapStage == 2) {
        Log::Info("分帧初始化[2]: XBase 渲染后端");
        xMenuDrawCallbackId = XBase::Hooks::RegisterDrawCallback([]() {
            Menu::Draw();
        });
        const bool hookReady = static_cast<bool>(xMenuDrawCallbackId) && XBase::Hooks::Init();
        if (hookReady) {
            // 网页页可以调用 XBase API，前端框架因此可以替代 ImGui 写界面
            XBase::WebBridge::Install();
            Controllers::ReactUi::Install();
            Controllers::ReactUi::Enable(AppConfig::GetUiMode() == "react");
        }

        if (hookReady) {
            xMenuActive = true;
            Log::Info("XBase 渲染后端初始化成功");
        } else {
            if (xMenuDrawCallbackId) {
                XBase::Hooks::UnregisterDrawCallback(xMenuDrawCallbackId);
                xMenuDrawCallbackId = {};
            }
            xMenuActive = false;
            XBase::Hooks::SetMenuVisible(false);
            ShowRenderBackendFailedMessage();
            Log::Error("XBase 渲染后端初始化失败；菜单渲染不可用，脚本逻辑继续执行");
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
            if (hintCooldown++ % 1800 == 0) ShowRenderBackendFailedMessage();
        }
        return;
    }

    Controllers::ReactUi::Process();

    if (XBase::Input::WasPressed(AppConfig::GetMenuHotkey())) {
        XBase::Hooks::SetMenuVisible(!XBase::Hooks::IsMenuVisible());
    }
}

bool InitXMenu() {
    Log::Info("注册 XBase Host 生命周期（帧驱动延迟到首次游戏初始化后）");
    return XBase::Host::Install({OnGameInit, OnProcess});
}

} // namespace

extern "C" void XBasePayloadAttach() {
    Log::Init();
    Log::Info("DLL 已加载，开始启动校验");
    if (!Startup::Validate()) {
        Log::Error("启动校验失败，XMenu 已停止初始化");
        return;
    }

    // 显示模式必须在游戏创建设备之前决定，窗口期交换链只能在这一刻建立
    const int startupWindowMode = AppConfig::PeekStoredWindowMode();
    if (startupWindowMode == 1) {
        XBase::Hooks::PrepareStartupWindowMode(XBase::Hooks::WindowMode::Windowed);
    } else if (startupWindowMode == 2) {
        XBase::Hooks::PrepareStartupWindowMode(XBase::Hooks::WindowMode::Borderless);
    }

    if (InitXMenu()) {
        Log::Info("启动校验与 Host 注册通过");
    } else {
        Log::Error("Host 注册失败，XMenu 已停止初始化");
    }
}

extern "C" void XBasePayloadDetach() {
    Controllers::BulletAssist::Shutdown();
    XBase::Host::Shutdown();
    XBaseBridge::Shutdown();
    if (xMenuDrawCallbackId) {
        XBase::Hooks::UnregisterDrawCallback(xMenuDrawCallbackId);
        xMenuDrawCallbackId = {};
    }
    XBase::WebBridge::Shutdown();
    XBase::Hooks::Shutdown();
    Log::Shutdown();
}