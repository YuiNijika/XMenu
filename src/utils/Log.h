#pragma once
#include <string>

namespace Log {
    void Init();
    void Shutdown();
    void Info(const char* message);
    void Info(const std::string& message);
    void Warn(const char* message);
    void Warn(const std::string& message);
    void Error(const char* message);
    void Error(const std::string& message);
}