#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace XBase::Platform {

// Shows a blocking platform error dialog.
bool ShowError(const char* title, const char* message);

// Opens a URL or document with the operating system's registered handler.
bool OpenExternal(const char* target);

// Copies UTF-8 text to the operating system clipboard.
bool SetClipboardText(const char* text);

// Monotonic process-local time suitable for UI delays and elapsed durations.
std::uint64_t MonotonicMilliseconds();

// Returns the directory containing a loaded module, including the trailing separator.
std::string ModuleDirectory(const char* moduleName);

// Returns the directory containing the XBase implementation module.
std::string CurrentModuleDirectory();
bool IsModuleLoaded(const char* moduleName);

bool EnsureDirectory(const std::string& path);
bool DirectoryExists(const std::string& path);
bool FileExists(const std::string& path);
bool ReadTextFile(const std::string& path, std::string& output);
bool WriteTextFile(const std::string& path, const std::string& content);
std::vector<std::string> ListDirectories(const std::string& path);
bool ReadModuleResource(int resourceId, std::string& output);

// Downloads a text resource through the platform transport.
bool DownloadText(const char* url, std::string& output);

} // namespace XBase::Platform