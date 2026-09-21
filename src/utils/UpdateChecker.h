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

    struct PromptPolicy {
        long long remindAfter = 0;   // unix 秒；0 表示未设置稍后提醒
        std::string skippedVersion;  // 已跳过的版本，出现更新版本时恢复提醒
        bool remindersDisabled = false;
    };

    // 双源检测：优先 GTAMODX，失败回退 GitHub
    void Start(const char* currentVersion);
    void Refresh();
    bool IsChecking();
    bool HasUpdate();
    UpdateInfo GetUpdateInfo();
    void Dismiss();
    void ForceDebugUpdate();

    // 自动弹窗策略：可稍后提醒、跳过当前版本或关闭提醒
    void SnoozeHours(int hours);
    void SkipCurrentVersion();
    void DisableReminders();
    void RestoreReminders();
    PromptPolicy GetPromptPolicy();
    bool ShouldPrompt();
    void Prompt();

    const char* SourceDisplayName(UpdateSource source);
}