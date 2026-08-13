#include "Log.h"

#include <XBase/Log.h>
#include <XBase/Platform.h>

namespace {
const char* LevelName(XBase::Log::Level level) {
    switch (level) {
    case XBase::Log::Level::Debug: return "DEBUG";
    case XBase::Log::Level::Info: return "INFO";
    case XBase::Log::Level::Warn: return "WARN";
    case XBase::Log::Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}
} // namespace

namespace Log {

void Init() {
    std::string directory = XBase::Platform::ModuleDirectory("XMenu.asi");
    if (directory.empty()) directory = XBase::Platform::CurrentModuleDirectory();
    directory += "XMenu\\";
    XBase::Platform::EnsureDirectory(directory);
    const std::string path = directory + "debug.log";
    XBase::Log::Init(path.c_str());
}

void Shutdown() {
    XBase::Log::Shutdown();
}

void Info(const char* message) {
    XBase::Log::Info(message);
}

void Info(const std::string& message) {
    XBase::Log::Info(message);
}

void Warn(const char* message) {
    XBase::Log::Warn(message);
}

void Warn(const std::string& message) {
    XBase::Log::Warn(message);
}

void Error(const char* message) {
    XBase::Log::Error(message);
}

void Error(const std::string& message) {
    XBase::Log::Error(message);
}

std::vector<Entry> GetEntries() {
    const std::vector<XBase::Log::Entry> source = XBase::Log::GetEntries();
    std::vector<Entry> entries;
    entries.reserve(source.size());
    for (const XBase::Log::Entry& entry : source) {
        entries.push_back({
            LevelName(entry.level),
            "[" + entry.timestamp + "] [" + LevelName(entry.level) + "] " + entry.text,
        });
    }
    return entries;
}

std::size_t GetTotalCount() {
    return XBase::Log::GetTotalCount();
}

std::string GetText() {
    return XBase::Log::GetText();
}

} // namespace Log