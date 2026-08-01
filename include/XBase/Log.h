#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace XBase::Log {

enum class Level { Debug, Info, Warn, Error };

struct Entry {
    Level level;
    std::string text;
    std::string timestamp;
};

void Init(const char* filePath = nullptr);
void Shutdown();
bool IsInitialized();

void Write(Level level, const char* message);
void Write(Level level, const std::string& message);

void Debug(const char* message);
void Info(const char* message);
void Warn(const char* message);
void Error(const char* message);

void Debug(const std::string& message);
void Info(const std::string& message);
void Warn(const std::string& message);
void Error(const std::string& message);

std::vector<Entry> GetEntries();
std::size_t GetTotalCount();
std::string GetText();
std::string GetLogFilePath();

} // namespace XBase::Log