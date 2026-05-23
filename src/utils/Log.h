#pragma once

namespace Log {
    void Init();
    void Shutdown();
    void Info(const char* message);
    void Warn(const char* message);
    void Error(const char* message);
}