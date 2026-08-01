#pragma once

#include <cstdint>
#include <string>

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

bool EnsureDirectory(const std::string& path);
bool FileExists(const std::string& path);

} // namespace XBase::Platform