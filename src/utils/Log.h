#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace Log {
    struct Entry {
        std::string level;
        std::string line;
    };

    void Init();
    void Shutdown();
    void Info(const char* message);
    void Info(const std::string& message);
    void Warn(const char* message);
    void Warn(const std::string& message);
    void Error(const char* message);
    void Error(const std::string& message);
    std::vector<Entry> GetEntries();
    std::size_t GetTotalCount();
    std::string GetText();
}
