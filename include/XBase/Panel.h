#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Capabilities.h"
#include "Input.h"

namespace XBase::Panel {

// XBase 提供壳与前端，模组只描述界面并挂钩子。
// 面板状态归共享运行时所有，多个模组挂进同一个面板，谁都不用写前端。
enum class ControlKind {
    Toggle,
    Float,
    Int,
    Action,
    Select,
    Text,
    Textarea,
    Color,
    Progress,
    Custom,
    Radio,
    MultiSelect,
    Heading,
    Separator,
};

struct Option {
    // value 是给模组自己看的标识，label 是显示给玩家的文本
    std::string value;
    std::string label;
};

struct Control {
    ControlKind kind = ControlKind::Toggle;

    // 全局唯一。约定按 模组名.分区.项 起名，重名的绑定会互相覆盖
    std::string id;

    // 直接给显示文本，多语言由模组自己决定，XBase 不代管词条
    std::string label;
    std::string hint;

    // 有边界才渲染成拖动条，否则渲染成输入框
    bool bounded = false;
    double min = 0.0;
    double max = 0.0;
    double step = 0.0;

    // 数值显示格式，形如 %.1f，留空按类型给默认
    std::string format;

    // 文本输入/自定义控件使用。Custom 支持内联内容，也支持从模组资源目录加载文件。
    // 文件路径相对于 XBase\Mods\<modId>\，例如 ui\custom.html / ui\custom.js / ui\custom.css。
    std::string text;
    std::string placeholder;
    std::string html;
    std::string script;
    std::string style;
    std::string htmlFile;
    std::string scriptFile;
    std::string styleFile;

    // 只读控件不显示编辑器；Progress/Color 默认只读，Text 可选绑定文本。
    bool readOnly = false;

    // 能力不支持时控件置灰而不是消失，玩家能看到这个功能确实存在
    std::optional<FeatureCapability> capability;

    // 依赖同一分区里另一个控件，值为真才显示，前置叹号取反
    std::string visibleWhen;

    // 限定游戏版本，取值 sa / vc / iii，留空表示三个版本都显示
    std::vector<std::string> games;

    // 仅下拉使用，读写的值是这里的下标
    std::vector<Option> options;
};

struct Section {
    std::string id;
    std::string label;
    std::string hint;

    int columns = 1;

    // 内联分区按固定宽度排布并自动换行，适合一排同类型的动作按钮
    bool inlineLayout = false;

    std::optional<FeatureCapability> capability;

    std::vector<Control> controls;
};

struct Page {
    std::string id;
    std::string label;
    std::vector<Section> sections;
};

struct ModSpec {
    // 与 XBasePayloadBaseName 保持一致；同一个 ASI 只能拥有一个 Sidebar 项。
    std::string modId;
    // 由 XBase 内部填充的宿主模块身份，模组通常无需设置。
    std::string ownerId;
    std::string title;
    std::string subtitle;
    std::string version;
    std::vector<Page> pages;
};

// 开关读写 0 与 1，数值直接读写，下拉读写的是 options 下标。
// 回调在游戏线程触发，动不了的与安全相关的动作要自己挂起到游戏线程
using ValueRead = std::function<double()>;
using ValueWrite = std::function<void(double)>;
using TextRead = std::function<std::string()>;
using TextWrite = std::function<void(const std::string&)>;
using ActionFn = std::function<void()>;

// 挂载一整个模组的界面，同一个 modId 重复挂载会整体替换
bool Mount(const ModSpec& spec);
void Unmount(const std::string& modId);

// 钩子。控件必须先出现在 Mount 的结构里，绑定才有意义
bool BindValue(const std::string& controlId, ValueRead read, ValueWrite write);
bool BindText(const std::string& controlId, TextRead read, TextWrite write);
bool BindAction(const std::string& controlId, ActionFn run);

// 模组自己改了状态后告诉面板刷新显示，否则网页会停在旧值上
void NotifyChanged(const std::string& controlId, double value);
void NotifyTextChanged(const std::string& controlId, const std::string& value);

// 网页视图运行时与面板资源都在才算可用，缺面板资源时打开会直接失败
bool IsAvailable();

// modId 留空表示回到上次打开的模组
bool Show(const std::string& modId = "");
void Hide();
void Toggle();
bool IsVisible();

// Panel 的全局热键由 XBase 固定为 P，传入值仅为旧版本兼容参数。
void SetHotkey(const Input::Hotkey& hotkey);
Input::Hotkey GetHotkey();

// 下面这组由 Core 的 Panel 域分发，宿主不要自己调
void Init();
bool IsInitialized();
void NotifyGameInit();
void Process();
void Shutdown();

} // namespace XBase::Panel
