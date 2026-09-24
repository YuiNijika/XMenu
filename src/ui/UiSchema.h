#pragma once

#include <XBase/Json.h>

#include <functional>
#include <string>
#include <vector>

namespace UiSchema {

// 读实时状态的控件没有 MenuState 可挂，由宿主登记一对读写函数。
// 控件 id 同时是网页端调用的桥接方法名，两边读的是同一份实时状态
bool RegisterLiveToggle(const char* id, std::function<bool()> read, std::function<void(bool)> write);
bool RegisterLiveNumber(const char* id, std::function<float()> read, std::function<void(float)> write);
bool HasLiveControl(const char* id);

// 下拉选择的选项，label 是要翻译的词条键（translated 为真）还是现成的名字
struct SelectOption {
    std::string value;
    std::string label;
    bool translated = true;
};

// 下拉类控件没有统一的存放位置，由宿主按来源名登记一份选项表与读写函数。
// 语言名这类是现成的名字，主题名这类是词条键，用 translated 区分
bool RegisterSelectSource(const char* name,
                          std::function<std::vector<SelectOption>()> options,
                          std::function<std::string()> read,
                          std::function<void(const std::string&)> write);

// 取已加载的注册表，网页界面通过桥接读的就是这一份，与 ImGui 完全一致
const XBase::Json::Value& Schema();

// 注册表加一份已解析的能力表一起返回。网页界面按 FeatureCapability 名门控，
// 不必依赖桥接方法表，两边因此用同一套判断
XBase::Json::Value SchemaPayload();

// 从模组数据目录读取界面特性注册表，重复调用只会加载一次
bool Load();

// 按 tab 与 page 定位分区，返回该分区是否登记过
bool HasSection(const char* tabId, const char* pageId, const char* sectionId);

// 绘制指定分区下的控件。分区一旦登记就由注册表接管，控件被版本或能力筛掉时不退回原生绘制，
// 返回值表示这次是不是注册表在画，调用方据此决定要不要退回原生兜底
bool DrawSection(const char* tabId, const char* pageId, const char* sectionId);

// 按控件 id 取值、写值与触发动作。宿主靠这三个入口把整个注册表接到网页端，
// 于是再加控件只需要改注册表，不用往桥接表里补方法
XBase::Json::Value ControlValue(const char* id);
bool SetControlValue(const char* id, const XBase::Json::Value& value);
bool RunControl(const char* id);

// 下拉控件的选项与当前值，网页端渲染选择框用
XBase::Json::Value ControlOptions(const char* id);

}
