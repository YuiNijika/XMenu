#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace XBase::Platform {

// Converts between UTF-8 used for internal strings and UTF-16 used by Win32 wide APIs.
// Paths stored and logged internally are always UTF-8, so convert only at Win32 boundaries.
std::wstring Utf8ToWide(const std::string& value);
std::string WideToUtf8(const std::wstring& value);

// Shows a blocking platform error dialog.
bool ShowError(const char* title, const char* message);

// Opens a URL or document with the registered handler of the operating system.
bool OpenExternal(const char* target);

// Copies UTF-8 text to the operating system clipboard.
bool SetClipboardText(const char* text);

// Monotonic process-local time suitable for UI delays and elapsed durations.
std::uint64_t MonotonicMilliseconds();

// Returns the directory containing a loaded module, including the trailing separator.
std::string ModuleDirectory(const char* moduleName);

// Returns the directory containing the XBase implementation module.
std::string CurrentModuleDirectory();
std::string GameDirectory();
bool IsModuleLoaded(const char* moduleName);

// Windows 10 及以上返回 true，用于挑默认界面
bool IsWindows10OrNewer();

// 所有 XBase 数据统一放在游戏根目录的 XBase 下，与具体 asi 位置无关
std::string XBaseDirectory();

// 与游戏目录无关的应用数据目录位于用户应用数据下的 com.yuinijika.xbase
// 缓存、WebView2 用户数据这类不该随游戏目录被整体打包分享的东西放这里。
// 取不到 AppData 时返回空字符串，调用方自行决定是否退回游戏目录。
std::string AppDataDirectory();

// 每个模组的数据放在 XBase 目录下以模组名命名的子目录
// 二进制统一放 XBase 目录下的 Library 子目录，与数据分开
// Mod 开头的函数与目录同名是正式命名，Runtime 开头的函数只是兼容别名，两者返回同一路径
std::string RuntimeDirectory(const char* modName);
std::string RuntimeConfigPath(const char* modName);
std::string RuntimeLogPath(const char* modName);
std::string ModDirectory(const char* modName);
std::string ModConfigPath(const char* modName);
std::string ModLogPath(const char* modName);

bool EnsureDirectory(const std::string& path);
bool DirectoryExists(const std::string& path);
bool FileExists(const std::string& path);
bool ReadTextFile(const std::string& path, std::string& output);
bool WriteTextFile(const std::string& path, const std::string& content);
bool ReadBinaryFile(const std::string& path, std::string& output);
bool WriteBinaryFile(const std::string& path, const std::string& content);
std::vector<std::string> ListDirectories(const std::string& path);
bool ReadModuleResource(int resourceId, std::string& output);

// Downloads a text resource through the platform transport.
bool DownloadText(const char* url, std::string& output);

} // namespace XBase::Platform