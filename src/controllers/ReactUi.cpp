#include "ReactUi.h"

#include "controllers/Ped.h"
#include "controllers/Player.h"
#include "controllers/Scene.h"
#include "controllers/Vehicle.h"
#include "controllers/Weapon.h"
#include "controllers/World.h"
#include "integration/XBaseBridge.h"
#include "ui/GuiTheme.h"
#include "ui/Menu.h"
#include "ui/MenuState.h"
#include "ui/UiSchema.h"
#include "../Identity.h"
#include "utils/I18n.h"
#include "utils/AppConfig.h"
#include "utils/DataManager.h"
#include "utils/UpdateChecker.h"
#include "utils/Log.h"

#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/Log.h>
#include <XBase/Platform.h>
#include <XBase/Player.h>
#include <XBase/UI.h>
#include <XBase/Version.h>
#include <XBase/Visual.h>
#include <XBase/WebBridge.h>
#include <XBase/WebView.h>

#include <algorithm>
#include <cstdlib>
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
    Move,
    SaveGame,
};

PendingAction s_pendingAction = PendingAction::None;
bool s_pendingUiTarget = false;
float s_pendingWidth = 0.0f;
float s_pendingHeight = 0.0f;
float s_pendingX = 0.0f;
float s_pendingY = 0.0f;

// 创建浏览器会碰 D3D 与窗口，必须等这一帧画完再在游戏线程里做
bool s_enablePending = false;

// 默认位置与大小对齐 XMenu 0.0.4-rc 的主窗口，网页可拖动标题栏移动、拖右下角改尺寸
constexpr float PanelDefaultX = 60.0f;
constexpr float PanelDefaultY = 60.0f;
constexpr float PanelDefaultWidth = 880.0f;
constexpr float PanelDefaultHeight = 600.0f;
constexpr float PanelMinWidth = 480.0f;
constexpr float PanelMinHeight = 360.0f;

float s_panelX = PanelDefaultX;
float s_panelY = PanelDefaultY;
float s_panelWidth = PanelDefaultWidth;
float s_panelHeight = PanelDefaultHeight;

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
    const XBase::Vec2 size = ClampPanelSize(s_panelWidth, s_panelHeight);
    const float maxX = display.x > 0.0f ? std::max(display.x - size.x, 0.0f) : s_panelX;
    const float maxY = display.y > 0.0f ? std::max(display.y - size.y, 0.0f) : s_panelY;

    XBase::Rect rect{};
    rect.left = std::min(std::max(s_panelX, 0.0f), maxX);
    rect.top = std::min(std::max(s_panelY, 0.0f), maxY);
    rect.right = rect.left + size.x;
    rect.bottom = rect.top + size.y;
    return rect;
}

void ApplyPanelSize(float width, float height) {
    const XBase::Vec2 size = ClampPanelSize(width, height);
    s_panelWidth = size.x;
    s_panelHeight = size.y;
    XBase::WebView::SetBounds(PanelRect());
}

void ApplyPanelPosition(float x, float y) {
    s_panelX = x;
    s_panelY = y;
    XBase::WebView::SetBounds(PanelRect());
}

constexpr const char* PanelHost = "xmenu.local";

std::string AppPath() {
    // 前端包与载荷同在 XBase 目录下以 XMenu 命名的子目录
    return XBase::Platform::ModDirectory("XMenu") + "ui.html";
}

// file 协议把每个页面当独立源，子资源会被拦下，这里用虚拟主机映射成本地站点
std::string AppUrl() {
    return std::string("https://") + PanelHost + "/ui.html";
}

XBase::Json::Value InfoValue() {
    XBase::Json::Value info;
    info.Set("version", XBase::Json::Value(ModIdentity::Version));
    info.Set("xbaseVersion", XBase::Json::Value(XBase::kVersionString));
    info.Set("author", XBase::Json::Value(ModIdentity::Author));
    info.Set("url", XBase::Json::Value(ModIdentity::Url));
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

extern const char* XMENU_GITHUB;

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

    // 注册表里的控件不再各自占一个桥接方法，统一走这三个入口：
    // 具体落到哪个 MenuState、哪个实时状态、哪个动作，全部由注册表决定。
    // 这样加一个控件只改 ui-schema.json，桥接表面固定就这三个
    XBase::WebBridge::RegisterMethod("ui.get", [](const XBase::Json::Value& params) {
        XBase::Json::Value result;
        result.Set("value", UiSchema::ControlValue(params["id"].AsString().c_str()));
        return result;
    });

    XBase::WebBridge::RegisterMethod("ui.set", [](const XBase::Json::Value& params) {
        const std::string id = params["id"].AsString();
        XBase::Json::Value result;
        result.Set("ok", XBase::Json::Value(UiSchema::SetControlValue(id.c_str(), params["value"])));
        result.Set("value", UiSchema::ControlValue(id.c_str()));
        return result;
    });

    XBase::WebBridge::RegisterMethod("ui.run", [](const XBase::Json::Value& params) {
        XBase::Json::Value result;
        result.Set("ok", XBase::Json::Value(UiSchema::RunControl(params["id"].AsString().c_str())));
        return result;
    });

    XBase::WebBridge::RegisterMethod("ui.options", [](const XBase::Json::Value& params) {
        XBase::Json::Value result = UiSchema::ControlOptions(params["id"].AsString().c_str());
        if (result.IsNull()) {
            result = XBase::Json::Value();
            result.Set("items", XBase::Json::Value());
        }
        return result;
    });

    // 语言、回退语言、主题与交互模式是下拉选择，选项与读写各登记一处，
    // ImGui 与网页端从此读同一份列表、改同一份配置
    UiSchema::RegisterSelectSource("language",
        [] {
            std::vector<UiSchema::SelectOption> options;
            for (const I18n::LanguageInfo& language : I18n::GetAvailableLanguages()) {
                options.push_back({language.code, language.name, false});
            }
            return options;
        },
        [] { return I18n::GetCurrentLanguageCode(); },
        [](const std::string& value) {
            I18n::SetLanguage(value);
            AppConfig::Save();
        });

    UiSchema::RegisterSelectSource("fallbackLanguage",
        [] {
            std::vector<UiSchema::SelectOption> options;
            for (const I18n::LanguageInfo& language : I18n::GetAvailableLanguages()) {
                options.push_back({language.code, language.name, false});
            }
            return options;
        },
        [] { return AppConfig::GetFallbackLanguageCode(); },
        [](const std::string& value) { AppConfig::SetFallbackLanguageCode(value); });

    UiSchema::RegisterSelectSource("theme",
        [] {
            std::vector<UiSchema::SelectOption> options;
            for (int index = 0; index < GuiTheme::ThemeCount; ++index) {
                options.push_back({std::to_string(index), GuiTheme::GetThemeNameKey(index), true});
            }
            return options;
        },
        [] { return std::to_string(GuiTheme::GetThemeIndex()); },
        [](const std::string& value) {
            // 与 ImGui 的设置页同一步骤：先落配置再换皮，少一步下次启动就回到旧主题
            const int index = std::atoi(value.c_str());
            AppConfig::SetGuiThemeIndex(index);
            GuiTheme::SetThemeByIndex(index);
            GuiTheme::Sync();
            AppConfig::Save();
        });

    UiSchema::RegisterSelectSource("interaction",
        [] {
            std::vector<UiSchema::SelectOption> options;
            for (int index = 0; index < GuiTheme::InteractionCount; ++index) {
                options.push_back({std::to_string(index), GuiTheme::GetInteractionNameKey(index), true});
            }
            return options;
        },
        [] { return std::to_string(GuiTheme::GetInteractionIndex()); },
        [](const std::string& value) {
            const int index = std::atoi(value.c_str());
            AppConfig::SetInteractionMode(index);
            GuiTheme::SetInteractionByIndex(index);
            GuiTheme::Sync();
            AppConfig::Save();
        });

    // 一个方法带多个布尔参数时按参数名落到各自的 MenuState 字段
    struct StateFlagKey {
        const char* method;
        const char* key;
        bool* flag;
    };

    static const StateFlagKey kStateFlagKeys[] = {

        {"weapon.statOverrides", "hugeDamage", &MenuState::HugeWeaponDamage},
        {"weapon.statOverrides", "longRange", &MenuState::LongWeaponRange},
        {"weapon.statOverrides", "rapidFire", &MenuState::RapidFire},
        {"weapon.statOverrides", "dualWield", &MenuState::DualWield},
        {"weapon.statOverrides", "moveAim", &MenuState::MoveAim},
        {"weapon.statOverrides", "moveFire", &MenuState::MoveFire},
        {"weapon.statOverrides", "noSpread", &MenuState::NoSpread},
        {"weapon.statOverrides", "autoAim", &MenuState::WeaponAutoAim},
        {"weapon.statOverrides", "customFireRate", &MenuState::WeaponFireRateEnabled},
        {"ped.noFireOptions", "civilians", &MenuState::PedsNoFireCivilians},
        {"ped.noFireOptions", "gangs", &MenuState::PedsNoFireGangs},
        {"ped.noFireOptions", "cops", &MenuState::PedsNoFirePolice},
        {"ped.noFireOptions", "mission", &MenuState::PedsNoFireMission},
        {"vehicle.autoDrive", "enable", &MenuState::VehicleAutoDrive},

    };

    // 数值参数同样按字段落位，避免下一帧被宿主推送覆盖
    struct StateNumberKey {
        const char* method;
        const char* key;
        float* floatTarget;
        int* intTarget;
    };

    static const StateNumberKey kStateNumberKeys[] = {
        {"vehicle.colors", "primary", nullptr, &MenuState::VehicleColorPrimary},
        {"vehicle.colors", "secondary", nullptr, &MenuState::VehicleColorSecondary},
        {"vehicle.colors4", "primary", nullptr, &MenuState::VehicleColorPrimary},
        {"vehicle.colors4", "secondary", nullptr, &MenuState::VehicleColorSecondary},
        {"vehicle.colors4", "tertiary", nullptr, &MenuState::VehicleColorTertiary},
        {"vehicle.colors4", "quaternary", nullptr, &MenuState::VehicleColorQuaternary},
        {"vehicle.trafficDensity", "value", &MenuState::VehicleTrafficDensity, nullptr},
        {"vehicle.autoDrive", "speed", &MenuState::VehicleAutoDriveSpeed, nullptr},
    };

    // 只接管宿主每帧推送的字段，其余方法交回 XBase 通用表，调用后立即生效
    static const char* const kKeyedMethods[] = {
        "weapon.statOverrides",
        "ped.noFireOptions",
        "vehicle.colors",
        "vehicle.colors4",
        "vehicle.trafficDensity",
        "vehicle.autoDrive",
    };

    for (const char* method : kKeyedMethods) {
        const std::string methodName = method;
        XBase::WebBridge::RegisterMethod(method, [methodName](const XBase::Json::Value& params) {
            for (const StateFlagKey& entry : kStateFlagKeys) {
                if (methodName != entry.method) continue;
                if (!params[entry.key].IsNull()) {
                    *entry.flag = params[entry.key].AsBool();
                }
            }
            for (const StateNumberKey& entry : kStateNumberKeys) {
                if (methodName != entry.method) continue;
                if (params[entry.key].IsNull()) continue;
                if (entry.floatTarget != nullptr) {
                    *entry.floatTarget = static_cast<float>(params[entry.key].AsNumber());
                } else if (entry.intTarget != nullptr) {
                    *entry.intTarget = params[entry.key].AsInt();
                }
            }
            XBase::Json::Value result;
            result.Set("ok", XBase::Json::Value(true));
            return result;
        });
    }

    XBase::WebBridge::RegisterMethod("weapon.statOverridesRate", [](const XBase::Json::Value& params) {
        MenuState::WeaponFireRate = static_cast<float>(params["fireRate"].AsNumber(1.0));
        XBase::Json::Value result;
        result.Set("fireRate", XBase::Json::Value(static_cast<double>(MenuState::WeaponFireRate)));
        return result;
    });

    XBase::WebBridge::RegisterMethod("player.setHealth", [](const XBase::Json::Value& params) {
        const float value = static_cast<float>(params["value"].AsNumber(100.0));
        MenuState::PlayerHealth = value;
        Controllers::Player::SetHealth(value);
        XBase::Json::Value result;
        result.Set("health", XBase::Json::Value(static_cast<double>(value)));
        return result;
    });

    XBase::WebBridge::RegisterMethod("player.wanted", [](const XBase::Json::Value& params) {
        const int level = params["level"].AsInt(0);
        MenuState::WantedLevel = level;
        XBase::Player::SetWantedLevel(level);
        XBase::Json::Value result;
        result.Set("level", XBase::Json::Value(level));
        return result;
    });

    // 外观与语言和 ImGui 设置页共用同一份状态，主题与交互模式走界面主题，语言走多语言
    XBase::WebBridge::RegisterMethod("settings.appearance", [](const XBase::Json::Value&) {
        XBase::Json::Value themes;
        for (int index = 0; index < GuiTheme::ThemeCount; ++index) {
            XBase::Json::Value item;
            item.Set("index", XBase::Json::Value(index));
            item.Set("name", XBase::Json::Value(GuiTheme::GetThemeNameKey(index)));
            themes.Push(item);
        }

        XBase::Json::Value interactions;
        for (int index = 0; index < GuiTheme::InteractionCount; ++index) {
            XBase::Json::Value item;
            item.Set("index", XBase::Json::Value(index));
            item.Set("name", XBase::Json::Value(GuiTheme::GetInteractionNameKey(index)));
            interactions.Push(item);
        }

        XBase::Json::Value languages;
        for (const I18n::LanguageInfo& language : I18n::GetAvailableLanguages()) {
            XBase::Json::Value item;
            item.Set("code", XBase::Json::Value(language.code));
            item.Set("name", XBase::Json::Value(language.name));
            languages.Push(item);
        }

        XBase::Json::Value result;
        result.Set("themeIndex", XBase::Json::Value(GuiTheme::GetThemeIndex()));
        result.Set("themes", themes);
        result.Set("interactionIndex", XBase::Json::Value(GuiTheme::GetInteractionIndex()));
        result.Set("interactions", interactions);
        result.Set("language", XBase::Json::Value(I18n::GetCurrentLanguageCode()));
        result.Set("languages", languages);
        return result;
    });

    XBase::WebBridge::RegisterMethod("settings.setTheme", [](const XBase::Json::Value& params) {
        GuiTheme::SetThemeByIndex(params["index"].AsInt(0));
        XBase::Json::Value result;
        result.Set("themeIndex", XBase::Json::Value(GuiTheme::GetThemeIndex()));
        return result;
    });

    XBase::WebBridge::RegisterMethod("settings.setInteraction", [](const XBase::Json::Value& params) {
        GuiTheme::SetInteractionByIndex(params["index"].AsInt(0));
        XBase::Json::Value result;
        result.Set("interactionIndex", XBase::Json::Value(GuiTheme::GetInteractionIndex()));
        return result;
    });

    XBase::WebBridge::RegisterMethod("settings.setLanguage", [](const XBase::Json::Value& params) {
        const std::string code = params["code"].AsString();
        if (!code.empty()) {
            I18n::SetLanguage(code);
        }
        XBase::Json::Value result;
        result.Set("language", XBase::Json::Value(I18n::GetCurrentLanguageCode()));
        return result;
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

    // 注册表里这些开关的状态存在 MenuState，由 Controllers::Vehicle 每帧下发给 XBase，
    // 桥接只负责读写状态，网页界面与 ImGui 因此共用同一个真实来源
    // 以下这一组是「读实时状态」的控件：状态在游戏对象上，没有 MenuState 可挂。
    // 宿主在这里登记一对读写函数，ImGui 每帧直接调用，网页端走 ui.get / ui.set 同名入口，
    // 两边看到的是同一份实时状态
    const auto registerLiveToggle = [](const char* id, bool (*read)(), void (*write)(bool)) {
        UiSchema::RegisterLiveToggle(id, read, write);
    };

    registerLiveToggle("vehicle.lights", [] { return Controllers::Vehicle::GetLights(); },
        [](bool enable) { Controllers::Vehicle::SetLights(enable); });
    registerLiveToggle("vehicle.locked", [] { return Controllers::Vehicle::GetLocked(); },
        [](bool enable) { Controllers::Vehicle::SetLocked(enable); });
    registerLiveToggle("vehicle.invisible", [] { return !Controllers::Vehicle::GetVisible(); },
        [](bool enable) { Controllers::Vehicle::SetVisible(!enable); });

    // 防护五项共用同一个状态结构，读写时各复制一次，改动只作用在自己那一项上
    const auto registerProofFlag = [](const char* id, bool XBase::Types::ProofState::* field) {
        UiSchema::RegisterLiveToggle(id,
            [field] {
                const XBase::Types::ProofState proofs = Controllers::Vehicle::GetProofState();
                return proofs.*field;
            },
            [field](bool enable) {
                XBase::Types::ProofState proofs = Controllers::Vehicle::GetProofState();
                proofs.*field = enable;
                Controllers::Vehicle::SetProofState(proofs);
            });
    };

    registerProofFlag("proof.bullet", &XBase::Types::ProofState::bullet);
    registerProofFlag("proof.collision", &XBase::Types::ProofState::collision);
    registerProofFlag("proof.explosion", &XBase::Types::ProofState::explosion);
    registerProofFlag("proof.fire", &XBase::Types::ProofState::fire);
    registerProofFlag("proof.melee", &XBase::Types::ProofState::melee);

    // 血量是实时值，下限夹到 1，免得把车写成炸掉
    UiSchema::RegisterLiveNumber("vehicle.health",
        [] { return Controllers::Vehicle::GetHealth(); },
        [](float value) { Controllers::Vehicle::SetHealth(value < 1.0f ? 1.0f : value); });

    // 七项载具属性走 读取成功与否 两个返回值，读不到时保持原值不动
    const auto registerVehicleAttribute = [](const char* id, bool (*read)(bool&), bool (*write)(bool)) {
        UiSchema::RegisterLiveToggle(id,
            [read] {
                bool value = false;
                read(value);
                return value;
            },
            [write](bool enable) { write(enable); });
    };

    registerVehicleAttribute("vehicle.alwaysSkidMarks",
        [](bool& value) { return Controllers::Vehicle::TryGetAlwaysSkidMarks(value); },
        [](bool enable) { return Controllers::Vehicle::SetAlwaysSkidMarks(enable); });
    registerVehicleAttribute("vehicle.disableParticles",
        [](bool& value) { return Controllers::Vehicle::TryGetDisableParticles(value); },
        [](bool enable) { return Controllers::Vehicle::SetDisableParticles(enable); });
    registerVehicleAttribute("vehicle.driverTargetable",
        [](bool& value) { return Controllers::Vehicle::TryGetDriverTargetable(value); },
        [](bool enable) { return Controllers::Vehicle::SetDriverTargetable(enable); });
    registerVehicleAttribute("vehicle.heatSeekingTargetable",
        [](bool& value) { return Controllers::Vehicle::TryGetHeatSeekingTargetable(value); },
        [](bool enable) { return Controllers::Vehicle::SetHeatSeekingTargetable(enable); });
    registerVehicleAttribute("vehicle.petrolTankWeak",
        [](bool& value) { return Controllers::Vehicle::TryGetPetrolTankWeakPoint(value); },
        [](bool enable) { return Controllers::Vehicle::SetPetrolTankWeakPoint(enable); });
    registerVehicleAttribute("vehicle.siren",
        [](bool& value) { return Controllers::Vehicle::TryGetSirenOrAlarm(value); },
        [](bool enable) { return Controllers::Vehicle::SetSirenOrAlarm(enable); });
    registerVehicleAttribute("vehicle.takeLessDamage",
        [](bool& value) { return Controllers::Vehicle::TryGetTakeLessDamage(value); },
        [](bool enable) { return Controllers::Vehicle::SetTakeLessDamage(enable); });

    // 下面几个是页面级的领域接口，不对应注册表里的某个控件，保持独立方法
    XBase::WebBridge::RegisterMethod("vehicle.spawn", [](const XBase::Json::Value& params) {
        const int model = params["model"].AsInt(MenuState::VehicleSpawnModel);
        if (model < 0) {
            XBase::Json::Value result;
            result.Set("ok", XBase::Json::Value(false));
            return result;
        }
        MenuState::VehicleSpawnModel = model;
        XBase::Json::Value result;
        result.Set("ok", XBase::Json::Value(Controllers::Vehicle::Spawn(static_cast<unsigned int>(model))));
        return result;
    });

    XBase::WebBridge::RegisterMethod("scene.animation", [](const XBase::Json::Value& params) {
        if (params["stop"].AsBool(false)) {
            Controllers::Scene::StopPlayerAnimation();
            XBase::Json::Value result;
            result.Set("ok", XBase::Json::Value(true));
            return result;
        }
        const std::string group = params["group"].AsString();
        const std::string name = params["name"].AsString();
        if (!group.empty()) {
            std::snprintf(MenuState::SceneAnimGroup, sizeof(MenuState::SceneAnimGroup), "%s", group.c_str());
        }
        if (!name.empty()) {
            std::snprintf(MenuState::SceneAnimName, sizeof(MenuState::SceneAnimName), "%s", name.c_str());
        }
        XBase::Json::Value result;
        result.Set("ok", XBase::Json::Value(Controllers::Scene::PlayPlayerAnimation()));
        return result;
    });

    XBase::WebBridge::RegisterMethod("weapon.aimPart", [](const XBase::Json::Value& params) {
        bool changed = false;
        if (!params["value"].IsNull()) {
            MenuState::WeaponBulletAimPart = params["value"].AsInt(MenuState::WeaponBulletAimPart);
            changed = true;
        } else if (!params["enable"].IsNull()) {
            MenuState::WeaponBulletAimPart = params["enable"].AsInt(MenuState::WeaponBulletAimPart);
            changed = true;
        }
        if (changed) {
            MenuState::WeaponBulletAimPart = std::min(std::max(MenuState::WeaponBulletAimPart, 0), 3);
        }
        XBase::Json::Value result;
        result.Set("value", XBase::Json::Value(MenuState::WeaponBulletAimPart));
        return result;
    });

    XBase::WebBridge::RegisterMethod("ui.schema", [](const XBase::Json::Value&) {
        return UiSchema::SchemaPayload();
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
        result.Set("name", XBase::Json::Value(ModIdentity::Name));
        result.Set("version", XBase::Json::Value(ModIdentity::Version));
        result.Set("author", XBase::Json::Value(ModIdentity::Author));
        result.Set("url", XBase::Json::Value(ModIdentity::Url));
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

    XBase::WebBridge::RegisterMethod("data.missions", [](const XBase::Json::Value&) {
        XBase::Json::Value items;
        for (const DataManager::MissionData& mission : DataManager::LoadMissions()) {
            XBase::Json::Value item;
            item.Set("category", XBase::Json::Value(mission.category));
            item.Set("name", XBase::Json::Value(mission.name));
            item.Set("id", XBase::Json::Value(mission.id));
            items.Push(item);
        }
        XBase::Json::Value result;
        result.Set("items", items);
        return result;
    });

    XBase::WebBridge::RegisterMethod("world.weatherCatalog", [](const XBase::Json::Value&) {
        int count = 0;
        const Controllers::World::WeatherEntry* entries = Controllers::World::GetWeatherCatalog(count);
        XBase::Json::Value items;
        for (int i = 0; i < count; ++i) {
            XBase::Json::Value item;
            item.Set("id", XBase::Json::Value(entries[i].id));
            item.Set("key", XBase::Json::Value(entries[i].key));
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

    // 面板位置与尺寸由网页拖拽决定，动作同样挂起到游戏线程执行
    XBase::WebBridge::RegisterMethod("menu.setPanelPos", [](const XBase::Json::Value& params) {
        const float x = static_cast<float>(params["x"].AsNumber(0.0));
        const float y = static_cast<float>(params["y"].AsNumber(0.0));
        s_pendingX = x;
        s_pendingY = y;
        s_pendingAction = PendingAction::Move;
        XBase::Json::Value result;
        result.Set("queued", XBase::Json::Value(true));
        return result;
    });

    XBase::WebBridge::RegisterMethod("menu.panelRect", [](const XBase::Json::Value&) {
        const XBase::Rect rect = PanelRect();
        XBase::Json::Value result;
        result.Set("x", XBase::Json::Value(static_cast<double>(rect.left)));
        result.Set("y", XBase::Json::Value(static_cast<double>(rect.top)));
        result.Set("width", XBase::Json::Value(static_cast<double>(rect.right - rect.left)));
        result.Set("height", XBase::Json::Value(static_cast<double>(rect.bottom - rect.top)));
        return result;
    });

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

XBase::Rect PanelBounds() {
    return PanelRect();
}

bool IsAvailable(std::string& reason) {
    if (!XBase::Platform::IsWindows10OrNewer()) {
        reason = I18n::T("react.fallback.osUnsupported");
        return false;
    }
    if (!XBaseBridge::HasCapability(XBase::Capability::WebView)) {
        reason = I18n::T("react.fallback.webviewUnsupported");
        return false;
    }
    if (!XBase::WebView::IsRuntimeAvailable()) {
        reason = I18n::T("react.fallback.runtimeMissing");
        return false;
    }
    if (!XBase::Platform::FileExists(AppPath())) {
        reason = I18n::T("react.fallback.uiMissing");
        return false;
    }
    reason.clear();
    return true;
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
    if (!XBase::Platform::FileExists(AppPath())) {
        ReportFallback(I18n::T("react.fallback.uiMissing"));
        return false;
    }

    // 独占全屏下浏览器会走抓帧回显，仍然照常切换，由页面顶部提示引导改窗口模式
    if (XBase::WebView::UsesCaptureMode()) {
        Log::Info("React 界面在独占全屏下启用，将以抓帧方式回显");
    }

    s_active = true;
    MenuState::ReactUi = true;
    MenuState::ReactUiFallbackReason.clear();
    s_enablePending = true;
    return true;
}

// 真正创建浏览器的步骤放在帧外执行，避免在绘制中途动窗口和 D3D
void StartPendingEnable() {
    if (!s_enablePending) {
        return;
    }
    s_enablePending = false;

    XBase::WebView::Init();
    XBase::WebView::MapFolder(PanelHost, XBase::Platform::ModDirectory("XMenu"));
    XBase::WebView::SetBounds(PanelRect());
    XBase::WebView::Navigate(AppUrl());
    XBase::WebView::SetVisible(true);
    Log::Info("React 界面已启用，页面来自 XMenu/ui.html");
}

void QueueSaveGame() {
    // 渲染线程与网页消息线程都不该直接动玩家状态，挂到游戏线程那一帧再执行
    s_pendingAction = PendingAction::SaveGame;
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
        s_enablePending = false;
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
    case PendingAction::Move:
        s_pendingAction = PendingAction::None;
        ApplyPanelPosition(s_pendingX, s_pendingY);
        return;
    case PendingAction::SaveGame:
        // 网页消息线程不能直接动玩家状态，存档请求与收起菜单都放到这里由游戏线程执行
        s_pendingAction = PendingAction::None;
        if (Controllers::Player::RequestSaveGame()) {
            XBase::Hooks::SetMenuVisible(false);
        }
        return;
    case PendingAction::None:
    default:
        break;
    }

    if (!s_active) {
        return;
    }

    StartPendingEnable();

    // 运行环境变化时退回 ImGui，避免留下一块无法交互的空白面板
    if (!XBase::WebView::IsRuntimeAvailable()) {
        LeaveReactUi(I18n::T("react.fallback.runtimeGone"));
        return;
    }

    // 页面缺失或加载失败时退回 ImGui，抓帧模式下收起再展开会带出临时错误，不能当成加载失败
    if (!XBase::WebView::UsesCaptureMode()) {
        const XBase::WebView::State state = XBase::WebView::GetState();
        if (state.initialized && state.lastError != 0) {
            LeaveReactUi(I18n::T("react.fallback.loadFailed"));
            return;
        }
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
