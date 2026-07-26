#pragma once
#include <string>

namespace UpdateChecker {
    enum class VersionStatus {
        Unknown,
        Equal,
        LocalNewer,
        RemoteNewer
    };

    enum class UpdateSource {
        Unknown,
        GTAMODX,
        GitHub
    };

    struct UpdateInfo {
        bool available = false;
        std::string currentVersion;
        std::string latestVersion;
        std::string releaseUrl;
        UpdateSource source = UpdateSource::Unknown;
        std::string sourceName;
        VersionStatus status = VersionStatus::Unknown;
    };

    // 双源检测：优先 GTAMODX，失败回退 GitHub
    void Start(const char* currentVersion);
    void Refresh();
    bool IsChecking();
    bool HasUpdate();
    UpdateInfo GetUpdateInfo();
    void Dismiss();
    void ForceDebugUpdate();

    const char* SourceDisplayName(UpdateSource source);
}