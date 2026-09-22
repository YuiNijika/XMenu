#include "ReactUi.h"

#include "integration/XBaseBridge.h"
#include "ui/MenuState.h"
#include "utils/BuildInfo.h"
#include "utils/I18n.h"
#include "utils/AppConfig.h"
#include "utils/DataManager.h"
#include "utils/UpdateChecker.h"
#include "utils/Log.h"

#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/Log.h>
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
    Resize,
};

PendingAction s_pendingAction = PendingAction::None;
bool s_pendingUiTarget = false;
float s_pendingWidth = 0.0f;
float s_pendingHeight = 0.0f;

// 面板默认占显示区域的八成，网页可拖动右下角改尺寸
constexpr float PanelWidthRatio = 0.80f;
constexpr float PanelHeightRatio = 0.78f;
constexpr float PanelMinWidth = 480.0f;
constexpr float PanelMinHeight = 360.0f;

float s_panelWidth = 0.0f;
float s_panelHeight = 0.0f;

XBase::Vec2 ClampPanelSize(float width, float height) {
    const XBase::Vec2 display = XBase::UI::GetDisplaySize();
    const float maxWidth = display.x > 0.0f ? display.x - 40.0f : width;
    const float maxHeight = display.y > 0.0f ? display.y - 40.0f : height;
    XBase::Vec2 size{};
    size.x = std::min(std::max(width, PanelMinWidth), std::max(maxWidth, PanelMinWidth));
    size.y = std::min(std::max(height, PanelMinHeight), std::max(maxHeight, PanelMinHeight));
    return size;
}

XBase::Rect PanelRect() {
    const XBase::Vec2 display = XBase::UI::GetDisplaySize();
    const XBase::Vec2 fallback{display.x * PanelWidthRatio, display.y * PanelHeightRatio};
    const float width = s_panelWidth > 0.0f ? s_panelWidth : std::min(fallback.x, 1280.0f);
    const float height = s_panelHeight > 0.0f ? s_panelHeight : std::min(fallback.y, 860.0f);
    XBase::Rect rect{};
    rect.left = (display.x - width) * 0.5f;
    rect.top = (display.y - height) * 0.5f;
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    return rect;
}

void ApplyPanelSize(float width, float height) {
    const XBase::Vec2 size = ClampPanelSize(width, height);
    s_panelWidth = size.x;
    s_panelHeight = size.y;
    XBase::WebView::SetBounds(PanelRect());
}

constexpr const char* PanelHost = "xmenu.local";

std::string AppPath() {
    return XBase::Platform::CurrentModuleDirectory() + "ui.html";
}

// file 协议把每个页面当独立源，子资源会被拦下，这里用虚拟主机映射成本地站点
std::string AppUrl() {
    return std::string("https://") + PanelHost + "/ui.html";
}

XBase::Json::Value InfoValue() {
    XBase::Json::Value info;
    info.Set("version", XBase::Json::Value(BuildInfo::Version));
    info.Set("author", XBase::Json::Value(BuildInfo::Author));
    info.Set("url", XBase::Json::Value(BuildInfo::Url));
    info.Set("ui", XBase::Json::Value(s_active ? "react" : "imgui"));
    info.Set("lang", XBase::Json::Value(I18n::GetCurrentLanguageCode()));
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

    // 显示模式与 ImGui 设置页同源，写入后提示重启生效
    XBase::WebBridge::RegisterMethod("settings.windowModeGet", [](const XBase::Json::Value&) {
        XBase::Json::Value result;
        result.Set("current", XBase::Json::Value(static_cast<int>(XBase::Hooks::GetWindowMode())));
        result.Set("pending", XBase::Json::Value(AppConfig::GetWindowModeSetting()));
        result.Set("restartRequired",
            XBase::Json::Value(AppConfig::GetWindowModeSetting() != static_cast<int>(XBase::Hooks::GetWindowMode())));
        return result;
    });

    XBase::WebBridge::RegisterMethod("settings.windowMode", [](const XBase::Json::Value& params) {
        XBase::Json::Value result;
        if (!params["value"].IsNull()) {
            const int mode = params["value"].AsInt(0);
            result.Set("applied", XBase::Json::Value(AppConfig::SetWindowModeSetting(mode)));
        }
        result.Set("current", XBase::Json::Value(static_cast<int>(XBase::Hooks::GetWindowMode())));
        result.Set("pending", XBase::Json::Value(AppConfig::GetWindowModeSetting()));
        result.Set("restartRequired",
            XBase::Json::Value(AppConfig::GetWindowModeSetting() != static_cast<int>(XBase::Hooks::GetWindowMode())));
        return result;
    });

    XBase::WebBridge::RegisterMethod("update.status", [](const XBase::Json::Value&) {
        const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
        XBase::Json::Value result;
        result.Set("checking", XBase::Json::Value(UpdateChecker::IsChecking()));
        result.Set("available", XBase::Json::Value(info.available));
        result.Set("currentVersion", XBase::Json::Value(info.currentVersion));
        result.Set("latestVersion", XBase::Json::Value(info.latestVersion));
        result.Set("releaseUrl", XBase::Json::Value(info.releaseUrl));
        result.Set("source", XBase::Json::Value(UpdateChecker::SourceDisplayName(info.source)));
        switch (info.status) {
        case UpdateChecker::VersionStatus::Equal: result.Set("status", XBase::Json::Value("equal")); break;
        case UpdateChecker::VersionStatus::LocalNewer: result.Set("status", XBase::Json::Value("localNewer")); break;
        case UpdateChecker::VersionStatus::RemoteNewer: result.Set("status", XBase::Json::Value("remoteNewer")); break;
        default: result.Set("status", XBase::Json::Value("unknown")); break;
        }
        return result;
    });

    XBase::WebBridge::RegisterMethod("update.check", [](const XBase::Json::Value&) {
        UpdateChecker::Refresh();
        XBase::Json::Value result;
        result.Set("started", XBase::Json::Value(true));
        return result;
    });

    XBase::WebBridge::RegisterMethod("update.open", [](const XBase::Json::Value& params) {
        std::string target = params["url"].AsString();
        if (target.empty()) {
            const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
            target = info.releaseUrl.empty() ? XMENU_GITHUB : info.releaseUrl;
        }
        XBase::Json::Value result;
        result.Set("opened", XBase::Json::Value(XBase::Platform::OpenExternal(target.c_str())));
        return result;
    });

    XBase::WebBridge::RegisterMethod("update.skip", [](const XBase::Json::Value&) {
        UpdateChecker::SkipCurrentVersion();
        XBase::Json::Value result;
        result.Set("skipped", XBase::Json::Value(true));
        return result;
    });

    XBase::WebBridge::RegisterMethod("update.later", [](const XBase::Json::Value& params) {
        UpdateChecker::SnoozeHours(params["hours"].AsInt(24));
        XBase::Json::Value result;
        result.Set("snoozed", XBase::Json::Value(true));
        return result;
    });

    XBase::WebBridge::RegisterMethod("log.recent", [](const XBase::Json::Value& params) {
        const int limit = params["count"].AsInt(80);
        const std::vector<Log::Entry> entries = Log::GetEntries();
        XBase::Json::Value items;
        const std::size_t start = entries.size() > static_cast<std::size_t>(limit)
            ? entries.size() - static_cast<std::size_t>(limit)
            : 0;
        for (std::size_t index = start; index < entries.size(); ++index) {
            XBase::Json::Value item;
            item.Set("level", XBase::Json::Value(entries[index].level));
            item.Set("line", XBase::Json::Value(entries[index].line));
            items.Push(item);
        }
        XBase::Json::Value result;
        result.Set("total", XBase::Json::Value(static_cast<int>(Log::GetTotalCount())));
        result.Set("items", items);
        return result;
    });

    XBase::WebBridge::RegisterMethod("log.copy", [](const XBase::Json::Value&) {
        XBase::Json::Value result;
        result.Set("copied", XBase::Json::Value(XBase::Platform::SetClipboardText(Log::GetText().c_str())));
        return result;
    });

    XBase::WebBridge::RegisterMethod("config.export", [](const XBase::Json::Value&) {
        const std::string text = AppConfig::ExportToText(AppConfig::TransferScope::All);
        XBase::Json::Value result;
        result.Set("length", XBase::Json::Value(static_cast<int>(text.size())));
        result.Set("copied", XBase::Json::Value(XBase::Platform::SetClipboardText(text.c_str())));
        result.Set("text", XBase::Json::Value(text));
        return result;
    });

    XBase::WebBridge::RegisterMethod("config.import", [](const XBase::Json::Value& params) {
        const std::string text = params["text"].AsString();
        XBase::Json::Value result;
        result.Set("imported", XBase::Json::Value(!text.empty() && AppConfig::ImportFromText(text, AppConfig::TransferScope::All)));
        return result;
    });

    XBase::WebBridge::RegisterMethod("app.info", [](const XBase::Json::Value&) {
        XBase::Json::Value result;
        result.Set("name", XBase::Json::Value(BuildInfo::Name));
        result.Set("version", XBase::Json::Value(BuildInfo::Version));
        result.Set("author", XBase::Json::Value(BuildInfo::Author));
        result.Set("url", XBase::Json::Value(BuildInfo::Url));
        result.Set("github", XBase::Json::Value(XMENU_GITHUB));
        return result;
    });

    // 数据包列表按需下发，网页用它做带搜索的浏览界面，名称与分类仍是词条键
    XBase::WebBridge::RegisterMethod("data.locations", [](const XBase::Json::Value&) {
        XBase::Json::Value items;
        for (const DataManager::LocationData& location : DataManager::LoadLocations()) {
            XBase::Json::Value item;
            item.Set("category", XBase::Json::Value(location.category));
            item.Set("name", XBase::Json::Value(location.name));
            item.Set("x", XBase::Json::Value(static_cast<double>(location.x)));
            item.Set("y", XBase::Json::Value(static_cast<double>(location.y)));
            item.Set("z", XBase::Json::Value(static_cast<double>(location.z)));
            item.Set("interior", XBase::Json::Value(location.interior));
            items.Push(item);
        }
        XBase::Json::Value result;
        result.Set("items", items);
        return result;
    });

    XBase::WebBridge::RegisterMethod("data.vehicles", [](const XBase::Json::Value&) {
        XBase::Json::Value items;
        for (const DataManager::VehicleData& vehicle : DataManager::LoadVehicles()) {
            XBase::Json::Value item;
            item.Set("category", XBase::Json::Value(vehicle.category));
            item.Set("name", XBase::Json::Value(vehicle.name));
            item.Set("id", XBase::Json::Value(vehicle.id));
            items.Push(item);
        }
        XBase::Json::Value result;
        result.Set("items", items);
        return result;
    });

    XBase::WebBridge::RegisterMethod("data.weapons", [](const XBase::Json::Value&) {
        XBase::Json::Value items;
        for (const DataManager::WeaponData& weapon : DataManager::LoadWeapons()) {
            XBase::Json::Value item;
            item.Set("category", XBase::Json::Value(weapon.category));
            item.Set("name", XBase::Json::Value(weapon.name));
            item.Set("id", XBase::Json::Value(weapon.id));
            item.Set("isModel", XBase::Json::Value(weapon.isModel));
            item.Set("modelId", XBase::Json::Value(weapon.modelId));
            items.Push(item);
        }
        XBase::Json::Value result;
        result.Set("items", items);
        return result;
    });

    XBase::WebBridge::RegisterMethod("data.peds", [](const XBase::Json::Value&) {
        XBase::Json::Value items;
        for (const DataManager::PedData& ped : DataManager::LoadPeds()) {
            XBase::Json::Value item;
            item.Set("category", XBase::Json::Value(ped.category));
            item.Set("name", XBase::Json::Value(ped.name));
            item.Set("id", XBase::Json::Value(ped.id));
            items.Push(item);
        }
        XBase::Json::Value result;
        result.Set("items", items);
        return result;
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

    // 面板尺寸由网页拖拽决定，动作同样挂起到游戏线程执行
    XBase::WebBridge::RegisterMethod("menu.setPanelSize", [](const XBase::Json::Value& params) {
        const float width = static_cast<float>(params["width"].AsNumber(0.0));
        const float height = static_cast<float>(params["height"].AsNumber(0.0));
        if (width >= 320.0f && height >= 240.0f) {
            s_pendingWidth = width;
            s_pendingHeight = height;
            s_pendingAction = PendingAction::Resize;
        }
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
    XBase::WebView::MapFolder(PanelHost, XBase::Platform::CurrentModuleDirectory());
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
        // 隐藏只收起菜单与面板，浏览器留着，再次打开菜单时复用
        XBase::Hooks::SetMenuVisible(false);
        XBase::WebView::SetVisible(false);
        return;
    case PendingAction::Close:
        s_pendingAction = PendingAction::None;
        XBase::Hooks::SetMenuVisible(false);
        Enable(false);
        XBase::WebView::Close();
        return;
    case PendingAction::SwitchUi:
        s_pendingAction = PendingAction::None;
        Enable(s_pendingUiTarget);
        return;
    case PendingAction::Resize:
        s_pendingAction = PendingAction::None;
        ApplyPanelSize(s_pendingWidth, s_pendingHeight);
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
    if (!XBase::Hooks::IsMenuVisible() && XBase::WebView::IsVisible()) {
        XBase::WebView::SetVisible(false);
    }
}

} // namespace Controllers::ReactUi
