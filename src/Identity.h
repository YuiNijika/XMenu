#pragma once

#include <string>

#include <XBase/Package.h>

// 身份信息单一真源是 XBase/Mods/XMenu/package.json，这里只做加载与缓存，
// 源码里不再写死名称版本作者；清单缺失时字段保持为空，界面显示 unknown
namespace ModIdentity {

inline std::string Name;
inline std::string Version;
inline std::string Author;
inline std::string Url;

// 读 package.json 并覆盖缓存，进程内调一次即可（Bootstrap 阶段）
inline void Load() {
    XBase::Package::Info info;
    XBase::Package::Load("XMenu", info);
    if (!info.valid) {
        return;
    }
    if (!info.name.empty()) Name = info.name;
    if (!info.version.empty()) Version = info.version;
    if (!info.author.empty()) Author = info.author;
    if (!info.homepage.empty()) Url = info.homepage;
}

} // namespace ModIdentity
