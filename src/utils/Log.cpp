#include "Log.h"
#include <windows.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <exception>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace {
    constexpr std::size_t MaxLogEntries = 600;

    std::ofstream logFile;
    std::mutex logMutex;
    std::deque<Log::Entry> recentEntries;
    std::size_t totalEntries = 0;
    LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter = nullptr;
    std::terminate_handler previousTerminateHandler = nullptr;
    bool crashHandlersInstalled = false;

    std::string ModuleDirectory() {
        HMODULE module = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&ModuleDirectory),
            &module
        );

        char path[MAX_PATH] = {};
        DWORD size = 0;
        if (module) {
            size = GetModuleFileNameA(module, path, MAX_PATH);
        }
        if (size == 0) {
            return "";
        }

        std::string directory(path, size);
        const std::size_t slash = directory.find_last_of("\\/");
        if (slash != std::string::npos) {
            return directory.substr(0, slash + 1);
        }
        return "";
    }

    std::string LogPath() {
        return ModuleDirectory() + "XMenu.log";
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

    std::string HexValue(std::uintptr_t value) {
        std::ostringstream stream;
        stream << "0x" << std::uppercase << std::hex << value;
        return stream.str();
    }

    std::string GetCurrentProcessPath() {
        char path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameA(nullptr, path, MAX_PATH);
        return size > 0 ? std::string(path, size) : std::string("<unknown>");
    }

    std::string GetModulePathFromAddress(void* address) {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(address, &info, sizeof(info)) == 0 || !info.AllocationBase) {
            return "<unknown>";
        }

        char path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameA(static_cast<HMODULE>(info.AllocationBase), path, MAX_PATH);
        return size > 0 ? std::string(path, size) : std::string("<unknown>");
    }

    const char* ExceptionCodeName(DWORD code) {
        switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
        default: return "UNKNOWN_EXCEPTION";
        }
    }

    void PushEntryUnlocked(const char* level, const std::string& line) {
        ++totalEntries;
        recentEntries.push_back({ level, line });
        while (recentEntries.size() > MaxLogEntries) {
            recentEntries.pop_front();
        }
    }

    void WriteLineUnlocked(const char* level, const char* message) {
        if (!logFile.is_open()) {
            logFile.open(LogPath(), std::ios::app);
        }

        const std::string line = "[" + BuildTimestamp() + "] [" + level + "] " + message;
        PushEntryUnlocked(level, line);

        if (!logFile.is_open()) {
            return;
        }

        logFile << line << '\n';
        logFile.flush();
    }

    void WriteLine(const char* level, const char* message) {
        std::lock_guard<std::mutex> lock(logMutex);
        WriteLineUnlocked(level, message);
    }

    void WriteCrashLine(const std::string& message) {
        std::ofstream crashLog(LogPath(), std::ios::app);
        if (!crashLog.is_open()) {
            return;
        }

        crashLog << "[" << BuildTimestamp() << "] [CRASH] " << message << '\n';
        crashLog.flush();
    }

    std::string BuildExceptionSummary(EXCEPTION_POINTERS* exceptionInfo) {
        if (!exceptionInfo || !exceptionInfo->ExceptionRecord) {
            return "异常信息为空";
        }

        const EXCEPTION_RECORD* record = exceptionInfo->ExceptionRecord;
        void* address = record->ExceptionAddress;

        std::ostringstream stream;
        stream << ExceptionCodeName(record->ExceptionCode)
               << " code=" << HexValue(record->ExceptionCode)
               << " address=" << HexValue(reinterpret_cast<std::uintptr_t>(address))
               << " module=" << GetModulePathFromAddress(address)
               << " thread=" << GetCurrentThreadId();

        if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
            const ULONG_PTR operation = record->ExceptionInformation[0];
            const ULONG_PTR target = record->ExceptionInformation[1];
            stream << " access=" << (operation == 0 ? "read" : operation == 1 ? "write" : operation == 8 ? "execute" : "unknown")
                   << " target=" << HexValue(static_cast<std::uintptr_t>(target));
        }

        if (record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR && record->NumberParameters >= 3) {
            stream << " ntstatus=" << HexValue(static_cast<std::uintptr_t>(record->ExceptionInformation[2]));
        }

#if defined(_M_IX86)
        if (exceptionInfo->ContextRecord) {
            const CONTEXT* context = exceptionInfo->ContextRecord;
            stream << " eax=" << HexValue(context->Eax)
                   << " ebx=" << HexValue(context->Ebx)
                   << " ecx=" << HexValue(context->Ecx)
                   << " edx=" << HexValue(context->Edx)
                   << " esi=" << HexValue(context->Esi)
                   << " edi=" << HexValue(context->Edi)
                   << " ebp=" << HexValue(context->Ebp)
                   << " esp=" << HexValue(context->Esp)
                   << " eip=" << HexValue(context->Eip);
        }
#endif

        return stream.str();
    }

    LONG WINAPI XMenuUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
        WriteCrashLine("捕获到未处理异常，游戏可能即将崩溃");
        WriteCrashLine(BuildExceptionSummary(exceptionInfo));

        if (previousExceptionFilter) {
            return previousExceptionFilter(exceptionInfo);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void XMenuTerminateHandler() {
        WriteCrashLine("捕获到 std::terminate，通常来自未捕获 C++ 异常或运行库终止");
        if (previousTerminateHandler) {
            previousTerminateHandler();
            return;
        }
        std::abort();
    }

    void InstallCrashHandlers() {
        if (crashHandlersInstalled) {
            return;
        }

        previousExceptionFilter = SetUnhandledExceptionFilter(XMenuUnhandledExceptionFilter);
        previousTerminateHandler = std::set_terminate(XMenuTerminateHandler);
        crashHandlersInstalled = true;
        WriteLineUnlocked("INFO", "崩溃日志捕获已启用");
    }
}

namespace Log {
    void Init() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            return;
        }

        logFile.open(LogPath(), std::ios::out | std::ios::trunc);
        if (logFile.is_open()) {
            const std::string line = "[" + BuildTimestamp() + "] [INFO] XMenu 日志启动";
            PushEntryUnlocked("INFO", line);
            logFile << line << '\n';

            std::ostringstream processLine;
            processLine << "[" << BuildTimestamp() << "] [INFO] 进程: " << GetCurrentProcessPath()
                        << " pid=" << GetCurrentProcessId()
                        << " thread=" << GetCurrentThreadId();
            PushEntryUnlocked("INFO", processLine.str());
            logFile << processLine.str() << '\n';
            logFile.flush();
            InstallCrashHandlers();
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

    std::vector<Entry> GetEntries() {
        std::lock_guard<std::mutex> lock(logMutex);
        return std::vector<Entry>(recentEntries.begin(), recentEntries.end());
    }

    std::size_t GetTotalCount() {
        std::lock_guard<std::mutex> lock(logMutex);
        return totalEntries;
    }

    std::string GetText() {
        std::lock_guard<std::mutex> lock(logMutex);
        std::ostringstream stream;
        for (const Entry& entry : recentEntries) {
            stream << entry.line << '\n';
        }
        return stream.str();
    }
}
