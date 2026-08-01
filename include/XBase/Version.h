#pragma once

#include <string>

namespace XBase {

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