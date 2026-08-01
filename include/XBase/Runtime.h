#pragma once

namespace XBase::Runtime {

enum class GameTarget {
    Unknown,
    SanAndreas,
    ViceCity,
    III,
};

enum class ValidationFailure {
    None,
    UnsupportedGameVersion,
    OnlineRuntimeDetected,
};

struct ValidationResult {
    bool success = false;
    GameTarget target = GameTarget::Unknown;
    ValidationFailure failure = ValidationFailure::None;
    const char* message = "unknown runtime error";
};

GameTarget GetGameTarget();
const char* GetGameKey();
const char* GetGameName();
ValidationResult ValidateEnvironment();

} // namespace XBase::Runtime