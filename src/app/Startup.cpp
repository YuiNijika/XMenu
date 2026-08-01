#include "Startup.h"
#include "utils/Log.h"

#include <XBase/Platform.h>
#include <XBase/Runtime.h>

namespace Startup {
    bool Validate() {
        const XBase::Runtime::ValidationResult validation = XBase::Runtime::ValidateEnvironment();
        if (validation.success) {
            Log::Info("启动校验完成：当前环境可用");
            return true;
        }

        switch (validation.failure) {
        case XBase::Runtime::ValidationFailure::UnsupportedGameVersion:
            Log::Error("启动校验失败：当前游戏版本不受支持");
            XBase::Platform::ShowError(
                "XMenu",
                "Unknown or unsupported game version. A supported original 1.0 executable is required.");
            break;
        case XBase::Runtime::ValidationFailure::OnlineRuntimeDetected:
            Log::Error("启动校验失败：检测到联机运行时");
            XBase::Platform::ShowError(
                "XMenu",
                "Online multiplayer runtime detected. XMenu is disabled for this session.");
            break;
        case XBase::Runtime::ValidationFailure::None:
        default:
            Log::Error("启动校验失败：未知运行时错误");
            XBase::Platform::ShowError("XMenu", validation.message);
            break;
        }
        return false;
    }
}