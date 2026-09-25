#pragma once

#include <cstdint>
#include <string>

namespace XBase {

// XBase 自身版本，发布安装包与线上文档都以它为准。
// mod 判断最低运行环境用 AtLeast 或 kVersionNumber，字符串只用于展示
inline constexpr const char* kVersionString = "v0.1.0-alpha1";

struct VersionTriple {
    int major;
    int minor;
    int patch;
};

inline constexpr VersionTriple kVersion{0, 1, 0};

// 数字编码 major 左移 16 位 minor 左移 8 位 patch，预发布后缀不参与比较
inline constexpr std::uint32_t kVersionNumber =
    (static_cast<std::uint32_t>(kVersion.major) << 16)
    | (static_cast<std::uint32_t>(kVersion.minor) << 8)
    | static_cast<std::uint32_t>(kVersion.patch);

inline bool AtLeast(int major, int minor, int patch) {
    const std::uint32_t required = (static_cast<std::uint32_t>(major) << 16)
        | (static_cast<std::uint32_t>(minor) << 8)
        | static_cast<std::uint32_t>(patch);
    return kVersionNumber >= required;
}

enum class GameVersion {
    SA,
    VC,
    III
};

inline GameVersion GetGameVersion() {
#if defined(GTASA)
    return GameVersion::SA;
#elif defined(GTAVC)
    return GameVersion::VC;
#else
    return GameVersion::III;
#endif
}

inline bool IsSA() { return GetGameVersion() == GameVersion::SA; }
inline bool IsVC() { return GetGameVersion() == GameVersion::VC; }
inline bool IsIII() { return GetGameVersion() == GameVersion::III; }

inline std::string GetVersionName() {
    switch (GetGameVersion()) {
    case GameVersion::SA: return "SA";
    case GameVersion::VC: return "VC";
    default: return "III";
    }
}

} // namespace XBase