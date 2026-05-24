#pragma once
#include <string>

namespace UpdateChecker {
    enum class VersionStatus {
        Unknown,
        Equal,
        LocalNewer,
        RemoteNewer
    };

    struct UpdateInfo {
        bool available = false;
        std::string currentVersion;
        std::string latestVersion;
        std::string releaseUrl;
        VersionStatus status = VersionStatus::Unknown;
    };

    void Start(const char* apiUrl, const char* currentVersion);
    bool IsChecking();
    bool HasUpdate();
    UpdateInfo GetUpdateInfo();
    void Dismiss();
}