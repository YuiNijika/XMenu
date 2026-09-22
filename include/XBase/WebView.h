#pragma once

#include <functional>
#include <string>

#include "ValueTypes.h"

namespace XBase::WebView {

struct State {
    bool initialized = false;  // 环境与控制器已就绪
    bool visible = false;
    bool loading = false;
    bool canGoBack = false;
    bool canGoForward = false;
    int lastError = 0;  // 最近一次导航失败码，0 表示成功
    std::string url;
    std::string title;
};

// WebView2 运行时与加载器是否可用。调用 Init 前先查询。
bool IsRuntimeAvailable();

// 在游戏窗口内创建网页视图，默认隐藏。必须由 Core 领域分发或宿主在游戏线程调用。
bool Init();
bool IsInitialized();
void NotifyGameInit();
void Process();
void Shutdown();

bool Navigate(const std::string& url);
bool SetHtml(const std::string& html);
void Reload();
bool GoBack();
bool GoForward();
void SetZoom(float factor);

void SetVisible(bool visible);
bool IsVisible();
void SetBounds(const Rect& bounds);
State GetState();

// 隐藏只让面板不可见，浏览器保持存活；关闭会释放浏览器与宿主窗口，
// 之后再次 SetVisible(true) 或 Navigate 会重新创建，两者对应窗口的最小化与关闭
bool Close();

// 独占全屏等无法合成 HWND 的环境下，面板改用抓帧贴图呈现。
// 此时宿主必须在占位矩形内调用 DrawPanel 并转发面板输入。
bool UsesCaptureMode();
void DrawPanel(const Rect& bounds);
void ForwardPanelInput(const Rect& bounds, Vec2 mouse, bool mouseDown, float wheelDelta);

// 导航、标题等状态变化时在游戏线程回调，宿主可用它刷新界面。
using StateCallback = void (*)();
void SetStateCallback(StateCallback callback);

// 网页通过 chrome.webview.postMessage 发来的原始 JSON，回调在游戏线程执行
using MessageHandler = std::function<void(const std::string& json)>;
void SetMessageHandler(MessageHandler handler);

// 原生向网页投递 JSON，网页用 chrome.webview 的 message 事件接收
bool PostJson(const std::string& json);

// 注入在页面脚本之前执行的代码，可重复调用，创建中的请求会排队
bool InjectScript(const std::string& script);

} // namespace XBase::WebView
