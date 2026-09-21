#include "Web.h"
#include "integration/XBaseBridge.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"
#include <XBase/Capabilities.h>
#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/UI.h>
#include <XBase/WebView.h>
#include <algorithm>
#include <cstdio>

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    bool IsWebPageAvailable() {
        return XBaseBridge::HasCapability(XBase::Capability::WebView);
    }

    // 检测结果进程内缓存，避免在渲染钩子里反复查询运行时
    bool IsWebRuntimeReady() {
        static int state = -1;
        if (state < 0) {
            state = XBase::WebView::IsRuntimeAvailable() ? 1 : 0;
        }
        return state == 1;
    }

    const char* WebStateText(const XBase::WebView::State& state) {
        if (state.lastError != 0) {
            return T("web.stateError");
        }
        if (state.loading) {
            return T("web.stateLoading");
        }
        if (state.initialized) {
            return T("web.stateReady");
        }
        return T("web.stateIdle");
    }

    void OpenWebPage() {
        if (!IsWebPageAvailable() || !IsWebRuntimeReady()) {
            MenuState::ShowNotice(T("web.runtimeMissing"), 2.5);
            return;
        }
        XBase::WebView::Init();
        if (MenuState::WebViewUrl[0] != '\0') {
            XBase::WebView::Navigate(MenuState::WebViewUrl);
        }
        XBase::WebView::SetZoom(MenuState::WebViewZoom);
        XBase::WebView::SetVisible(true);
        MenuState::WebViewVisible = XBase::WebView::IsVisible();
    }

    void CloseWebPage() {
        XBase::WebView::SetVisible(false);
        MenuState::WebViewVisible = false;
    }
}

namespace Pages::Web {
    void Process() {
        if (MenuState::WebViewVisible != XBase::WebView::IsVisible()) {
            MenuState::WebViewVisible = XBase::WebView::IsVisible();
        }

        // 切到网页页时自动加载，独占全屏下提示改窗口模式才能原生渲染
        if (MenuState::WebTabEntered && !MenuState::WebViewVisible) {
            OpenWebPage();
        }
        static bool fullscreenNoticeShown = false;
        if (MenuState::WebViewVisible && XBase::WebView::UsesCaptureMode()) {
            if (!fullscreenNoticeShown) {
                fullscreenNoticeShown = true;
                MenuState::ShowNotice(T("web.fullscreenNotice"), 6.0, true);
            }
        } else if (!XBase::WebView::UsesCaptureMode()) {
            fullscreenNoticeShown = false;
        }

        if (!MenuState::WebViewVisible) {
            return;
        }
        if (!XBase::Hooks::IsMenuVisible()) {
            CloseWebPage();
        }
    }

    void Draw() {
        XBase::UI::Text(T("web.title"));
        XBase::UI::TextWrapped(T("web.hint"));

        if (!IsWebPageAvailable()) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("web.unavailable"));
            return;
        }
        if (!IsWebRuntimeReady()) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("web.runtimeMissing"));
            return;
        }

        XBase::UI::Spacing();
        XBase::UI::PushItemWidth(360.0f);
        XBase::UI::InputText(T("web.url"), MenuState::WebViewUrl, sizeof(MenuState::WebViewUrl));
        XBase::UI::PopItemWidth();

        if (UI::Button(T("web.open"), 2)) {
            OpenWebPage();
        }
        XBase::UI::SameLine();
        if (UI::Button(T("web.close"), 2)) {
            CloseWebPage();
        }
        XBase::UI::SameLine();
        if (UI::Button(T("web.reload"))) {
            XBase::WebView::Reload();
        }
        XBase::UI::SameLine();
        if (UI::Button(T("web.back"))) {
            XBase::WebView::GoBack();
        }
        XBase::UI::SameLine();
        if (UI::Button(T("web.forward"))) {
            XBase::WebView::GoForward();
        }

        const XBase::WebView::State state = XBase::WebView::GetState();
        XBase::UI::PushItemWidth(220.0f);
        if (XBase::UI::Slider(T("web.zoom"), MenuState::WebViewZoom, 0.5f, 2.0f, "%.2fx")) {
            XBase::WebView::SetZoom(MenuState::WebViewZoom);
        }
        XBase::UI::PopItemWidth();
        XBase::UI::SameLine();
        XBase::UI::Text(T("web.state"), WebStateText(state));
        if (!state.url.empty()) {
            XBase::UI::TextWrapped(state.url.c_str());
        }

        if (MenuState::UseNativeMenu) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("web.windowModeHint"));
            return;
        }

        if (XBase::Hooks::IsWindowModeSupported()) {
            bool borderless = AppConfig::GetWindowModeSetting() == 2;
            if (XBase::UI::Checkbox(T("web.borderlessMode"), borderless)) {
                if (AppConfig::SetWindowModeSetting(borderless ? 2 : 0)) {
                    MenuState::ShowNotice(T("settings.displayModeRestartNotice"), 3.0, true);
                    XBase::Host::QueueMessage(T("settings.displayModeRestartMessage"));
                }
            }
            XBase::UI::SameLine();
            XBase::UI::TextDisabled(T("web.borderlessModeHint"));
            if (AppConfig::GetWindowModeSetting() != static_cast<int>(XBase::Hooks::GetWindowMode())) {
                UI::TextWarning(T("settings.displayModePending"));
            }
        }

        if (XBase::WebView::UsesCaptureMode()) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("web.fullscreenHint"));
        }

        // 视口占用菜单内容区剩余空间，网页渲染在菜单窗口内部
        const XBase::Vec2 origin = XBase::UI::GetCursorScreenPosition();
        const XBase::Vec2 available = XBase::UI::GetContentAvailable();
        if (available.x < 32.0f || available.y < 32.0f) {
            return;
        }
        const float viewportHeight = std::max(120.0f, available.y);
        const XBase::Vec2 display = XBase::UI::GetDisplaySize();
        char geometry[192]{};
        std::snprintf(
            geometry,
            sizeof(geometry),
            "display %.0fx%.0f  viewport %.0f,%.0f %.0fx%.0f  visible %d initialized %d",
            display.x, display.y, origin.x, origin.y, available.x, viewportHeight,
            MenuState::WebViewVisible ? 1 : 0, state.initialized ? 1 : 0);
        XBase::UI::TextDisabled(geometry);

        // 视口占用菜单内容区剩余空间，网页渲染在菜单窗口内部
        const XBase::Rect windowRect = XBase::UI::GetCurrentWindowRect();
        XBase::Rect viewport{};
        viewport.left = std::max(origin.x, windowRect.left);
        viewport.top = std::max(origin.y, windowRect.top);
        viewport.right = std::min(origin.x + available.x, windowRect.right);
        viewport.bottom = std::min(origin.y + viewportHeight, windowRect.bottom);
        if (viewport.right - viewport.left < 24.0f || viewport.bottom - viewport.top < 24.0f) {
            viewport.right = viewport.left + 1.0f;
            viewport.bottom = viewport.top + 1.0f;
        }

        if (MenuState::WebViewVisible) {
            XBase::WebView::SetBounds(viewport);
        } else {
            XBase::UI::TextDisabled(T("web.closedHint"));
        }

        // 占位并标出网页区域，便于确认面板位置
        XBase::UI::Canvas::Rect(
            {origin.x, origin.y},
            {origin.x + available.x, origin.y + viewportHeight},
            {41, 98, 255, 255},
            1.0f);
        XBase::UI::InvisibleButton("##WebViewport", {available.x, viewportHeight});

        // 独占全屏下 HWND 覆盖层不合成，改用抓帧贴图回显并转发输入
        if (MenuState::WebViewVisible && XBase::WebView::UsesCaptureMode()) {
            XBase::WebView::DrawPanel(viewport);
            if (XBase::UI::IsLastItemHovered()) {
                const float wheelDelta = XBase::Hooks::ConsumeWheelDelta();
                XBase::WebView::ForwardPanelInput(
                    viewport,
                    XBase::UI::GetMousePosition(),
                    XBase::UI::IsMouseDown(XBase::UI::MouseButton::Left),
                    wheelDelta);
            }
        }
    }
}
