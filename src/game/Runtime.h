#pragma once

namespace GameRuntime {
    enum class Target {
        Unknown,
        SA,
        VC,
        III,
    };

    struct RuntimeInfo {
        Target target = Target::Unknown;
        const char* key = "unknown";
        const char* name = "Unknown";
        bool supported = false;
    };

    RuntimeInfo Detect();
    bool Init();
    bool ValidateEnvironment();
    const RuntimeInfo& Current();
    const char* CurrentKey();
    const char* CurrentName();
}