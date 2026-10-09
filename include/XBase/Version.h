#pragma once

#include <cstdint>
#include <string>

namespace XBase {

struct VersionTriple {
    int major;
    int minor;
    int patch;
};

const char* GetVersionString();
VersionTriple GetVersion();
std::uint32_t GetVersionNumber();
bool AtLeast(int major, int minor, int patch);

enum class GameVersion {
    SA,
    VC,
    III
};

GameVersion GetGameVersion();
bool IsSA();
bool IsVC();
bool IsIII();
std::string GetVersionName();

} // namespace XBase
