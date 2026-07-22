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
#include <vector>

namespace {
    constexpr std::size_t MaxLogEntries = 600;

    std::ofstream logFile;
    std::mutex logMutex;
    std::deque<Log::Entry> recentEntries;
    std::size_t totalEntries = 0;
    LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter = nullptr;
    std::terminate_handler previousTerminateHandler = nullptr;
    bool crashHandlersInstalled = false;
    bool wroteUtf8Bom = false;

    // 宽字符 → UTF-8
    std::string WideToUtf8(const wchar_t* wide) {
        if (!wide || !wide[0]) {
            return {};
        }
        const int needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (needed <= 1) {
            return {};
        }
        std::string utf8(static_cast<std::size_t>(needed - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8.data(), needed, nullptr, nullptr);
        return utf8;
    }

    // 系统 ANSI(ACP) → UTF-8（兼容外部窄字符串）
    std::string AcpToUtf8(const char* acp) {
        if (!acp || !acp[0]) {
            return {};
        }
        const int wideLen = MultiByteToWideChar(CP_ACP, 0, acp, -1, nullptr, 0);
        if (wideLen <= 1) {
            return acp;
        }
        std::wstring wide(static_cast<std::size_t>(wideLen - 1), L'\0');
        MultiByteToWideChar(CP_ACP, 0, acp, -1, wide.data(), wideLen);
        return WideToUtf8(wide.c_str());
    }

    bool LooksLikeUtf8(const char* text) {
        if (!text) {
            return true;
        }
        const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
        while (*p) {
            if (*p <= 0x7F) {
                ++p;
                continue;
            }
            if ((*p & 0xE0) == 0xC0) {
                if ((p[1] & 0xC0) != 0x80) return false;
                p += 2;
                continue;
            }
            if ((*p & 0xF0) == 0xE0) {
                if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return false;
                p += 3;
                continue;
            }
            if ((*p & 0xF8) == 0xF0) {
                if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return false;
                p += 4;
                continue;
            }
            return false;
        }
        return true;
    }

    // 日志消息统一成 UTF-8：合法 UTF-8 原样保留，否则按系统 ACP 转码
    std::string NormalizeToUtf8(const char* message) {
        if (!message) {
            return {};
        }
        if (LooksLikeUtf8(message)) {
            return message;
        }
        return AcpToUtf8(message);
    }

    std::string DirectoryFromModule(HMODULE module) {
        if (!module) {
            return {};
        }

        wchar_t path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameW(module, path, MAX_PATH);
        if (size == 0) {
            return {};
        }

        std::string directory = WideToUtf8(path);
        const std::size_t slash = directory.find_last_of("\\/");
        if (slash != std::string::npos) {
            return directory.substr(0, slash + 1);
        }
        return {};
    }

    std::string ModuleDirectory() {
        const std::string asiDirectory = DirectoryFromModule(GetModuleHandleW(L"XMenu.asi"));
        if (!asiDirectory.empty()) {
            return asiDirectory;
        }

        HMODULE module = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ModuleDirectory),
            &module
        );

        return DirectoryFromModule(module);
    }

    bool EnsureDirectory(const std::string& utf8Path) {
        if (utf8Path.empty()) {
            return false;
        }
        const int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
        if (wideLen <= 1) {
            return false;
        }
        std::wstring wide(static_cast<std::size_t>(wideLen - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, wide.data(), wideLen);
        if (CreateDirectoryW(wide.c_str(), nullptr)) {
            return true;
        }
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    std::string XMenuDataDirectory() {
        return ModuleDirectory() + "XMenu\\";
    }

    std::wstring LogPathW() {
        const std::string directory = XMenuDataDirectory();
        EnsureDirectory(directory);
        const std::string utf8 = directory + "debug.log";
        const int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (wideLen <= 1) {
            return L"debug.log";
        }
        std::wstring wide(static_cast<std::size_t>(wideLen - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), wideLen);
        return wide;
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
        wchar_t path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameW(nullptr, path, MAX_PATH);
        return size > 0 ? WideToUtf8(path) : std::string("<unknown>");
    }

    std::string GetModulePathFromAddress(void* address) {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(address, &info, sizeof(info)) == 0 || !info.AllocationBase) {
            return "<unknown>";
        }

        wchar_t path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameW(static_cast<HMODULE>(info.AllocationBase), path, MAX_PATH);
        return size > 0 ? WideToUtf8(path) : std::string("<unknown>");
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

    bool OpenLogFileUnlocked(bool truncate) {
        if (logFile.is_open()) {
            logFile.close();
        }

        const std::wstring path = LogPathW();
        // binary：避免 text 模式对字节流做本地化转换，稳定写 UTF-8
        const std::ios::openmode mode = std::ios::binary | (truncate ? std::ios::out | std::ios::trunc : std::ios::out | std::ios::app);
        logFile.open(path, mode);
        if (!logFile.is_open()) {
            wroteUtf8Bom = false;
            return false;
        }

        if (truncate) {
            const char bom[] = { '\xEF', '\xBB', '\xBF' };
            logFile.write(bom, sizeof(bom));
            wroteUtf8Bom = true;
        } else {
            // append 时检查是否空文件，空则补 BOM
            logFile.seekp(0, std::ios::end);
            if (logFile.tellp() == std::streampos(0)) {
                const char bom[] = { '\xEF', '\xBB', '\xBF' };
                logFile.write(bom, sizeof(bom));
                wroteUtf8Bom = true;
            }
        }
        return true;
    }

    void WriteLineUnlocked(const char* level, const char* message) {
        if (!logFile.is_open()) {
            OpenLogFileUnlocked(false);
        }

        const std::string utf8Message = NormalizeToUtf8(message);
        const std::string line = "[" + BuildTimestamp() + "] [" + level + "] " + utf8Message;
        PushEntryUnlocked(level, line);

        if (!logFile.is_open()) {
            return;
        }

        logFile.write(line.data(), static_cast<std::streamsize>(line.size()));
        logFile.put('\n');
        logFile.flush();
    }

    void WriteLine(const char* level, const char* message) {
        std::lock_guard<std::mutex> lock(logMutex);
        WriteLineUnlocked(level, message);
    }

    void WriteCrashLine(const std::string& message) {
        const std::wstring path = LogPathW();
        std::ofstream crashLog(path, std::ios::binary | std::ios::app);
        if (!crashLog.is_open()) {
            return;
        }

        const std::string line = "[" + BuildTimestamp() + "] [CRASH] " + NormalizeToUtf8(message.c_str());
        crashLog.write(line.data(), static_cast<std::streamsize>(line.size()));
        crashLog.put('\n');
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

        if (!OpenLogFileUnlocked(true)) {
            return;
        }

        WriteLineUnlocked("INFO", "XMenu 日志启动 (UTF-8)");
        WriteLineUnlocked("INFO", ("进程: " + GetCurrentProcessPath()
            + " pid=" + std::to_string(GetCurrentProcessId())
            + " thread=" + std::to_string(GetCurrentThreadId())).c_str());
        InstallCrashHandlers();
    }

    void Shutdown() {
        WriteLine("INFO", "XMenu 正在卸载");
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile.close();
        }
        wroteUtf8Bom = false;
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