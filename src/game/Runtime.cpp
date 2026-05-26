#include "Runtime.h"
#include "utils/Log.h"
#include <cstring>
#include <windows.h>

namespace {
    GameRuntime::RuntimeInfo currentRuntime;

    bool ReadUInt(unsigned int address, unsigned int& value) {
        __try {
            value = *reinterpret_cast<const unsigned int*>(address);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            value = 0;
            return false;
        }
    }

    bool MatchUInt(unsigned int address, unsigned int expected) {
        unsigned int value = 0;
        return ReadUInt(address, value) && value == expected;
    }

    GameRuntime::RuntimeInfo MakeRuntime(GameRuntime::Target target, const char* key, const char* name) {
        GameRuntime::RuntimeInfo info;
        info.target = target;
        info.key = key;
        info.name = name;
        info.supported = target != GameRuntime::Target::Unknown;
        return info;
    }
}

namespace GameRuntime {
    RuntimeInfo Detect() {
        if (MatchUInt(0x401000, 0x53EC8B55) || MatchUInt(0x401000, 0x16197BE9)
            || MatchUInt(0x8245BC, 0x94BF) || MatchUInt(0x8252FC, 0x94BF)
            || MatchUInt(0x82533C, 0x94BF) || MatchUInt(0x858D51, 0x3539F633)
            || MatchUInt(0x858C61, 0x3539F633)) {
            return MakeRuntime(Target::SA, "sa", "GTA San Andreas");
        }

        if (MatchUInt(0x667BF0, 0x53E58955) || MatchUInt(0x667C40, 0x53E58955)
            || MatchUInt(0x666BA0, 0x53E58955)) {
            return MakeRuntime(Target::VC, "vc", "GTA Vice City");
        }

        if (MatchUInt(0x5C1E70, 0x53E58955) || MatchUInt(0x5C2130, 0x53E58955)
            || MatchUInt(0x5C6FD0, 0x53E58955)) {
            return MakeRuntime(Target::III, "iii", "GTA III");
        }

        return MakeRuntime(Target::Unknown, "unknown", "Unknown");
    }

    bool Init() {
        currentRuntime = Detect();
        if (!currentRuntime.supported) {
            Log::Error("未识别到支持的 GTA 运行时");
            return false;
        }

        Log::Info(std::string("运行时已识别: ") + currentRuntime.name + " (" + currentRuntime.key + ")");
        return true;
    }

    bool ValidateEnvironment() {
        const RuntimeInfo& runtime = Current();
        if (!runtime.supported) {
            Log::Error("启动校验失败：当前游戏不可识别");
            return false;
        }

        if (runtime.target == Target::SA && (GetModuleHandleA("SAMP.dll") || GetModuleHandleA("SAMP.asi"))) {
            Log::Error("启动校验失败：检测到 SA 联机运行时");
            return false;
        }

        if (runtime.target == Target::VC && (GetModuleHandleA("vcmp-proxy.dll") || GetModuleHandleA("vcmp-proxy.asi"))) {
            Log::Error("启动校验失败：检测到 VC 联机运行时");
            return false;
        }

        Log::Info(std::string("启动校验完成：") + runtime.name + " 环境可用");
        return true;
    }

    const RuntimeInfo& Current() {
        if (!currentRuntime.supported) {
            currentRuntime = Detect();
        }
        return currentRuntime;
    }

    const char* CurrentKey() {
        return Current().key;
    }

    const char* CurrentName() {
        return Current().name;
    }
}