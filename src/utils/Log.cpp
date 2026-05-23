#include "Log.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace {
    std::ofstream logFile;
    std::mutex logMutex;

    const char* LogPath() {
        return "XMenu.log";
    }

    std::string BuildTimestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        localtime_s(&localTime, &nowTime);

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return stream.str();
    }

    void WriteLine(const char* level, const char* message) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (!logFile.is_open()) {
            logFile.open(LogPath(), std::ios::app);
        }

        if (!logFile.is_open()) {
            return;
        }

        logFile << "[" << BuildTimestamp() << "] [" << level << "] " << message << '\n';
        logFile.flush();
    }
}

namespace Log {
    void Init() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            return;
        }

        logFile.open(LogPath(), std::ios::app);
        if (logFile.is_open()) {
            logFile << "\n[" << BuildTimestamp() << "] [INFO] XMenu 日志启动" << '\n';
            logFile.flush();
        }
    }

    void Shutdown() {
        WriteLine("INFO", "XMenu 正在卸载");
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void Info(const char* message) {
        WriteLine("INFO", message);
    }

    void Info(const std::string& message) {
        WriteLine("INFO", message.c_str());
    }

    void Warn(const char* message) {
        WriteLine("WARN", message);
    }

    void Warn(const std::string& message) {
        WriteLine("WARN", message.c_str());
    }

    void Error(const char* message) {
        WriteLine("ERROR", message);
    }

    void Error(const std::string& message) {
        WriteLine("ERROR", message.c_str());
    }
}