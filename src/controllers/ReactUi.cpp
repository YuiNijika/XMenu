#include "ReactUi.h"

#include "integration/XBaseBridge.h"
#include "ui/MenuState.h"
#include "utils/BuildInfo.h"
#include "utils/I18n.h"
#include "utils/Log.h"

#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/Platform.h>
#include <XBase/UI.h>
#include <XBase/WebBridge.h>
#include <XBase/WebView.h>

#include <algorithm>
#include <string>

namespace {

bool s_active = false;
bool s_installed = false;

// 网页消息回调里不能直接隐藏或销毁控制器，动作挂起到游戏线程执行
enum class PendingAction {
    None,
    Hide,
    Close,
    SwitchUi,
};

PendingAction s_pendingAction = PendingAction::None;
bool s_pendingUiTarget = false;

// 面板占显示区域的大部分，居中留出边距
constexpr float PanelWidthRatio = 0.80f;
constexpr float PanelHeightRatio = 0.78f;

XBase::Rect PanelRect() {
    // 面板不铺满，按显示尺寸取比例并设上限，避免默认过大
    const XBase::Vec2 display = XBase::UI::GetDisplaySize();
    const float width = std::min(display.x * PanelWidthRatio, 1280.0f);
    const float height = std::min(display.y * PanelHeightRatio, 860.0f);
    XBase::Rect rect{};
    rect.left = (display.x - width) * 0.5f;
    rect.top = (display.y - height) * 0.5f;
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    return rect;
}

std::string AppPath() {
    return XBase::Platform::CurrentModuleDirectory() + "ui.html";
}

std::string AppUrl() {
    std::string path = AppPath();
    std::replace(path.begin(), path.end(), '\\', '/');
    return "file:///" + path;
}

XBase::Json::Value InfoValue() {
    XBase::Json::Value info;
    info.Set("version", XBase::Json::Value(BuildInfo::Version));
    info.Set("author", XBase::Json::Value(BuildInfo::Author));
    info.Set("url", XBase::Json::Value(BuildInfo::Url));
    info.Set("ui", XBase::Json::Value(s_active ? "react" : "imgui"));
    return info;
}

void ReportFallback(const char* reason) {
    Log::Error(std::string("React 界面不可用，已回退到 ImGui：") + reason);
    MenuState::ReactUiFallbackReason = reason;
    XBase::Host::QueueMessage(I18n::T("react.fallback.notice"));
}

void LeaveReactUi(const char* reason) {
    ReportFallback(reason);
    s_active = false;
    MenuState::ReactUi = false;
    XBase::WebView::SetVisible(false);
}

} // namespace

namespace Controllers::ReactUi {

void Install() {
    if (s_installed) {
        return;
    }

    XBase::WebBridge::RegisterMethod("i18n.dictionary", [](const XBase::Json::Value&) {
        XBase::Json::Value entries;
        const std::unordered_map<std::string, std::string> dictionary = I18n::GetDictionary();
        for (const auto& entry : dictionary) {
            entries.Set(entry.first, XBase::Json::Value(entry.second));
        }
        XBase::Json::Value result;
        result.Set("lang", XBase::Json::Value(I18n::GetCurrentLanguageCode()));
        result.Set("entries", entries);
        Log::Info("React 界面已下发语言包，词条 "
            + std::to_string(dictionary.size())
            + " 条，语言 " + I18n::GetCurrentLanguageCode());
        return result;
    });

    XBase::WebBridge::RegisterMethod("menu.info", [](const XBase::Json::Value&) {
        return InfoValue();
    });

    XBase::WebBridge::RegisterMethod("menu.hide", [](const XBase::Json::Value&) {
        s_pendingAction = PendingAction::Hide;
        XBase::Json::Value result;
        result.Set("queued", XBase::Json::Value(true));
        return result;
    });

    XBase::WebBridge::RegisterMethod("menu.close", [](const XBase::Json::Value&) {
        s_pendingAction = PendingAction::Close;
        XBase::Json::Value result;
        result.Set("queued", XBase::Json::Value(true));
        return result;
    });

    XBase::WebBridge::RegisterMethod("menu.setUi", [](const XBase::Json::Value& params) {
        const std::string ui = params["ui"].AsString();
        s_pendingAction = PendingAction::SwitchUi;
        s_pendingUiTarget = ui == "react";
        XBase::Json::Value result;
        result.Set("queued", XBase::Json::Value(true));
        result.Set("ui", XBase::Json::Value(s_active ? "react" : "imgui"));
        return result;
    });

    s_installed = true;
}

bool IsActive() {
    return s_active;
}

bool Enable(bool enable) {
    if (enable == s_active) {
        return true;
    }

    if (!enable) {
        s_active = false;
        MenuState::ReactUi = false;
        XBase::WebView::SetVisible(false);
        Log::Info("React 界面已关闭");
        return true;
    }

    if (!XBaseBridge::HasCapability(XBase::Capability::WebView)) {
        ReportFallback(I18n::T("react.fallback.webviewUnsupported"));
        return false;
    }
    if (!XBase::WebView::IsRuntimeAvailable()) {
        ReportFallback(I18n::T("react.fallback.runtimeMissing"));
        return false;
    }
    if (XBase::WebView::UsesCaptureMode()) {
        ReportFallback(I18n::T("react.fallback.captureMode"));
        return false;
    }
    if (!XBase::Platform::FileExists(AppPath())) {
        ReportFallback(I18n::T("react.fallback.uiMissing"));
        return false;
    }

    s_active = true;
    MenuState::ReactUi = true;
    MenuState::ReactUiFallbackReason.clear();

    XBase::WebView::Init();
    XBase::WebView::SetBounds(PanelRect());
    XBase::WebView::Navigate(AppUrl());
    XBase::WebView::SetVisible(true);
    Log::Info("React 界面已启用，页面来自 XMenu/ui.html");
    return true;
}

void Process() {
    switch (s_pendingAction) {
    case PendingAction::Hide:
        s_pendingAction = PendingAction::None;
        Enable(false);
        return;
    case PendingAction::Close:
        s_pendingAction = PendingAction::None;
        Enable(false);
        XBase::WebView::Close();
        return;
    case PendingAction::SwitchUi:
        s_pendingAction = PendingAction::None;
        Enable(s_pendingUiTarget);
        return;
    case PendingAction::None:
    default:
        break;
    }

    if (!s_active) {
        return;
    }

    // 运行环境变化时退回 ImGui，避免留下一块无法交互的空白面板
    if (!XBase::WebView::IsRuntimeAvailable()) {
        LeaveReactUi(I18n::T("react.fallback.runtimeGone"));
        return;
    }
    if (XBase::WebView::UsesCaptureMode()) {
        LeaveReactUi(I18n::T("react.fallback.captureMode"));
        return;
    }

    // 页面缺失或加载失败时退回 ImGui，避免留下一块空白面板
    const XBase::WebView::State state = XBase::WebView::GetState();
    if (state.initialized && state.lastError != 0) {
        LeaveReactUi(I18n::T("react.fallback.loadFailed"));
        return;
    }

    XBase::WebView::SetBounds(PanelRect());
    if (XBase::Hooks::IsMenuVisible() && !XBase::WebView::IsVisible()) {
        XBase::WebView::SetVisible(true);
    }
}

} // namespace Controllers::ReactUi
