#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// mod 清单 XBase/Mods/<ModName>/package.json 的读取与约束判定。
// 清单声明 mod 自身的 name / version / author 与所需 XBase 版本（engines.xbase），
// Bootstrap 挂载时校验约束，不满足就拒绝挂载并弹出原因

namespace XBase::Package {

struct Info {
    bool valid = false;            // 文件存在且解析成功
    std::string name;              // 缺省用目录名
    std::string version;
    std::string author;
    std::string description;
    std::string license;           // SPDX 标识符，例如 MIT
    std::string homepage;
    std::string xbaseRequirement;  // engines.xbase 原文，例如 ">=0.1.0"
    // dependencies 的键值对，值是 Node 语义的版本区间。
    // 名字对应其它 mod 的按对方清单版本校验；
    // 对应不到 mod 的是第三方运行库声明（如 asi loader），只声明不做自动校验
    std::vector<std::pair<std::string, std::string>> dependencies;
};

// 读 XBase/Mods/<modName>/package.json。文件不存在时返回假且 valid 为假，字段给目录名兜底
bool Load(const std::string& modName, Info& out);

// 判定约束是否被当前 XBase 版本满足。
// 支持 || 分隔的任一组，组内空格分隔的多个条件同时满足；
// 单条支持 >= > <= < ^ ~ == 与精确版本，连字符区间 1.2.3 - 2.0.0，
// 通配写法 1.x / 1.2.x / *，空串或 * 表示不限。区间语义对齐 npm 的 node-semver
bool Satisfies(const std::string& requirement, std::uint32_t runtimeNumber);

// 版本字符串转数字编码，前缀 v 忽略，预发布后缀不参与比较；格式不合法返回假
bool ParseVersion(const std::string& text, std::uint32_t& number);

// 发布通道标记：latest / alpha / beta / rc / stable / nightly / dev，大小写不敏感。
// 这类值不做数字比较，按版本字符串里是否带该通道字样判定
bool IsChannelToken(const std::string& value);

// 同 Satisfies，但直接拿版本字符串判定，通道标记可以真正匹配上
// （例如要求 alpha 时，对方版本串必须带 alpha 字样）；版本串为空视为不满足
bool SatisfiesText(const std::string& requirement, const std::string& runtimeVersion);

// 便捷封装：清单里的 engines.xbase 与 dependencies 逐一校验。
// 清单不存在视为无约束返回真；存在但解析失败或校验不通过时返回假并给出原因
bool Validate(const std::string& modName, std::string& failureReason);

} // namespace XBase::Package
