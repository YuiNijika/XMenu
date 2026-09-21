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

    void ReloadWebPage() {
        if (!MenuState::WebViewVisible) {
            OpenWebPage();
            return;
        }
        XBase::WebView::Reload();
    }

    void CloseWebPage() {
        XBase::WebView::SetVisible(false);
        MenuState::WebViewVisible = false;
    }

    // 网页区域坐标，失败提示与重试按钮都复用这一块
    XBase::Rect ComputeViewport(const XBase::Vec2& origin, const XBase::Vec2& available) {
        const XBase::Rect windowRect = XBase::UI::GetCurrentWindowRect();
        XBase::Rect viewport{};
        viewport.left = std::max(origin.x, windowRect.left);
        viewport.top = std::max(origin.y, windowRect.top);
        viewport.right = std::min(origin.x + available.x, windowRect.right);
        viewport.bottom = std::min(origin.y + available.y, windowRect.bottom);
        if (viewport.right - viewport.left < 24.0f || viewport.bottom - viewport.top < 24.0f) {
            viewport.right = viewport.left + 1.0f;
            viewport.bottom = viewport.top + 1.0f;
        }
        return viewport;
    }
}

namespace Pages::Web {
    void Process() {
        if (MenuState::WebViewVisible != XBase::WebView::IsVisible()) {
            MenuState::WebViewVisible = XBase::WebView::IsVisible();
        }

        // 切到该页时自动加载，独占全屏下提示改窗口模式才能原生渲染
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
        XBase::UI::TextDisabled(T("web.guide"));

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

        if (MenuState::UseNativeMenu) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("web.windowModeHint"));
            return;
        }

        if (XBase::WebView::UsesCaptureMode()) {
            XBase::UI::Spacing();
            XBase::UI::TextDisabled(T("web.fullscreenHint"));
        }

        const XBase::Vec2 origin = XBase::UI::GetCursorScreenPosition();
        const XBase::Vec2 available = XBase::UI::GetContentAvailable();
        if (available.x < 32.0f || available.y < 32.0f) {
            return;
        }

        XBase::UI::InvisibleButton("##WebViewport", {available.x, available.y});

        if (MenuState::WebViewVisible) {
            const XBase::Rect viewport = ComputeViewport(origin, available);
            XBase::WebView::SetBounds(viewport);

            // 独占全屏下 HWND 覆盖层不合成，改用抓帧贴图回显并转发输入
            if (XBase::WebView::UsesCaptureMode()) {
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

        const XBase::WebView::State state = XBase::WebView::GetState();
        if (!MenuState::WebViewVisible || state.lastError != 0) {
            const XBase::Vec2 center{
                origin.x + available.x * 0.5f,
                origin.y + available.y * 0.5f};
            XBase::UI::SetCursorScreenPos({center.x - 150.0f, center.y - 44.0f});
            XBase::UI::TextWrapped(state.lastError != 0 ? T("web.loadFailed") : T("web.closedHint"));
            XBase::UI::SetCursorScreenPos({center.x - 70.0f, center.y + 4.0f});
            if (XBase::UI::Button(T("web.retry"), {140.0f, 32.0f})) {
                ReloadWebPage();
            }
        }
    }
}
