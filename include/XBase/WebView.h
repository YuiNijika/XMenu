#pragma once

#include <functional>
#include <string>

#include "ValueTypes.h"

namespace XBase::WebView {

using WebViewId = unsigned int;
constexpr WebViewId DefaultInstance = 0;

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
bool IsRuntimeAvailable(WebViewId id);

// 创建 WebView2 实例句柄。句柄保留独立的导航状态和消息回调，
// 但同一个 XBase 运行时只允许一个实例处于可见/创建状态；
// 显示新实例时会自动关闭当前实例。
WebViewId Create();
void Destroy(WebViewId id);
WebViewId CurrentInstance();

// 在游戏窗口内创建网页视图，默认隐藏。必须由 Core 领域分发或宿主在游戏线程调用。
bool Init();
bool Init(WebViewId id);
bool IsInitialized();
bool IsInitialized(WebViewId id);
void NotifyGameInit();
void Process();
void Shutdown();
void Shutdown(WebViewId id);

bool Navigate(const std::string& url);
bool Navigate(WebViewId id, const std::string& url);
bool SetHtml(const std::string& html);
bool SetHtml(WebViewId id, const std::string& html);
void Reload();
void Reload(WebViewId id);
bool GoBack();
bool GoBack(WebViewId id);
bool GoForward();
bool GoForward(WebViewId id);
void SetZoom(float factor);
void SetZoom(WebViewId id, float factor);

void SetVisible(bool visible);
void SetVisible(WebViewId id, bool visible);
bool IsVisible();
bool IsVisible(WebViewId id);
void SetBounds(const Rect& bounds);
void SetBounds(WebViewId id, const Rect& bounds);
State GetState();
State GetState(WebViewId id);

// 隐藏只让面板不可见，浏览器保持存活，关闭会释放浏览器与宿主窗口，
// 之后再次设为可见或导航会重新创建，两者分别对应窗口的最小化与关闭
bool Close();
bool Close(WebViewId id);

// 独占全屏等无法合成 HWND 的环境下，面板改用抓帧贴图呈现。
// 此时宿主必须在占位矩形内调用 DrawPanel 并转发面板输入。
bool UsesCaptureMode();
bool UsesCaptureMode(WebViewId id);
void DrawPanel(const Rect& bounds);
void DrawPanel(WebViewId id, const Rect& bounds);
void ForwardPanelInput(const Rect& bounds, Vec2 mouse, bool mouseDown, float wheelDelta);
void ForwardPanelInput(WebViewId id, const Rect& bounds, Vec2 mouse, bool mouseDown, float wheelDelta);

// 导航、标题等状态变化时在游戏线程回调，宿主可用它刷新界面。
using StateCallback = void (*)();
void SetStateCallback(StateCallback callback);
void SetStateCallback(WebViewId id, StateCallback callback);

// 网页通过 chrome.webview.postMessage 发来的原始 JSON，回调在游戏线程执行
using MessageHandler = std::function<void(const std::string& json)>;
void SetMessageHandler(MessageHandler handler);
void SetMessageHandler(WebViewId id, MessageHandler handler);

// 原生向网页投递 JSON，网页用 chrome.webview 的 message 事件接收
bool PostJson(const std::string& json);
bool PostJson(WebViewId id, const std::string& json);

// 注入在页面脚本之前执行的代码，可重复调用，创建中的请求会排队
bool InjectScript(const std::string& script);
bool InjectScript(WebViewId id, const std::string& script);

// 把本地目录映射成虚拟 https 主机，页面就能像普通站点一样加载脚本与样式
bool MapFolder(const std::string& hostName, const std::string& folderPath);
bool MapFolder(WebViewId id, const std::string& hostName, const std::string& folderPath);

} // namespace XBase::WebView
