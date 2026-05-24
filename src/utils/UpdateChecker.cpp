#include "UpdateChecker.h"
#include "utils/AppConfig.h"
#include "utils/Log.h"
#include <windows.h>
#include <urlmon.h>
#include <atomic>
#include <cctype>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

extern const bool XMENU_DEBUG_MODE;

namespace {
    constexpr std::time_t CacheTtlSeconds = 24 * 60 * 60;

    void DebugUpdate(const std::string& message) {
        if (XMENU_DEBUG_MODE) {
            Log::Info(std::string("更新检查调试：") + message);
        }
    }

    std::mutex updateMutex;
    std::atomic<bool> checking{ false };
    std::atomic<bool> started{ false };
    std::atomic<bool> dismissed{ false };
    std::atomic<bool> debugUpdateDialog{ false };
    std::string configuredApiUrl;
    std::string configuredCurrentVersion;
    UpdateChecker::UpdateInfo updateInfo;

    std::string Trim(const std::string& value) {
        std::size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
            ++begin;
        }

        std::size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return value.substr(begin, end - begin);
    }

    std::string NormalizeVersion(std::string value) {
        value = Trim(value);
        if (!value.empty() && (value[0] == 'v' || value[0] == 'V')) {
            value.erase(value.begin());
        }
        return value;
    }

    std::string MainVersion(const std::string& version) {
        const std::string normalized = NormalizeVersion(version);
        const std::size_t dash = normalized.find('-');
        return dash == std::string::npos ? normalized : normalized.substr(0, dash);
    }

    std::string PreRelease(const std::string& version) {
        const std::string normalized = NormalizeVersion(version);
        const std::size_t dash = normalized.find('-');
        return dash == std::string::npos ? std::string() : normalized.substr(dash + 1);
    }

    std::vector<int> ExtractVersionNumbers(const std::string& version) {
        std::vector<int> numbers;
        std::string current;

        for (const char ch : version) {
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                current.push_back(ch);
                continue;
            }

            if (!current.empty()) {
                numbers.push_back(std::atoi(current.c_str()));
                current.clear();
            }
        }

        if (!current.empty()) {
            numbers.push_back(std::atoi(current.c_str()));
        }

        return numbers;
    }

    int CompareVersion(const std::string& left, const std::string& right) {
        const std::vector<int> leftNumbers = ExtractVersionNumbers(MainVersion(left));
        const std::vector<int> rightNumbers = ExtractVersionNumbers(MainVersion(right));
        const std::size_t count = leftNumbers.size() > rightNumbers.size() ? leftNumbers.size() : rightNumbers.size();

        for (std::size_t i = 0; i < count; ++i) {
            const int leftNumber = i < leftNumbers.size() ? leftNumbers[i] : 0;
            const int rightNumber = i < rightNumbers.size() ? rightNumbers[i] : 0;
            if (leftNumber != rightNumber) {
                return leftNumber > rightNumber ? 1 : -1;
            }
        }

        const std::string leftPre = PreRelease(left);
        const std::string rightPre = PreRelease(right);
        if (leftPre.empty() && rightPre.empty()) {
            return 0;
        }
        if (leftPre.empty()) {
            return 1;
        }
        if (rightPre.empty()) {
            return -1;
        }
        if (leftPre == rightPre) {
            return 0;
        }
        return leftPre > rightPre ? 1 : -1;
    }

    UpdateChecker::VersionStatus BuildStatus(const std::string& currentVersion, const std::string& latestVersion) {
        if (currentVersion.empty() || latestVersion.empty()) {
            return UpdateChecker::VersionStatus::Unknown;
        }

        const int comparison = CompareVersion(currentVersion, latestVersion);
        if (comparison > 0) {
            return UpdateChecker::VersionStatus::LocalNewer;
        }
        if (comparison < 0) {
            return UpdateChecker::VersionStatus::RemoteNewer;
        }
        return UpdateChecker::VersionStatus::Equal;
    }

    void ApplyVersionInfo(const std::string& currentVersion, const std::string& latestVersion, const std::string& releaseUrl) {
        std::lock_guard<std::mutex> lock(updateMutex);
        updateInfo.currentVersion = currentVersion;
        updateInfo.latestVersion = latestVersion;
        updateInfo.releaseUrl = releaseUrl;
        updateInfo.status = BuildStatus(currentVersion, latestVersion);
        updateInfo.available = updateInfo.status == UpdateChecker::VersionStatus::RemoteNewer;
        if (updateInfo.available) {
            dismissed = false;
        }
    }

    std::string DecodeJsonString(const std::string& value) {
        std::string decoded;
        decoded.reserve(value.size());

        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] != '\\' || i + 1 >= value.size()) {
                decoded.push_back(value[i]);
                continue;
            }

            const char escaped = value[++i];
            switch (escaped) {
            case 'n': decoded.push_back('\n'); break;
            case 'r': decoded.push_back('\r'); break;
            case 't': decoded.push_back('\t'); break;
            case '"': decoded.push_back('"'); break;
            case '\\': decoded.push_back('\\'); break;
            case '/': decoded.push_back('/'); break;
            default: decoded.push_back(escaped); break;
            }
        }

        return decoded;
    }

    std::string ExtractJsonString(const std::string& json, const char* key) {
        const std::string pattern = std::string("\"") + key + "\"";
        const std::size_t keyPosition = json.find(pattern);
        if (keyPosition == std::string::npos) {
            return {};
        }

        const std::size_t colon = json.find(':', keyPosition + pattern.size());
        if (colon == std::string::npos) {
            return {};
        }

        const std::size_t valueStart = json.find('"', colon + 1);
        if (valueStart == std::string::npos) {
            return {};
        }

        std::size_t valueEnd = valueStart + 1;
        bool escaped = false;
        while (valueEnd < json.size()) {
            const char current = json[valueEnd];
            if (current == '"' && !escaped) {
                break;
            }
            escaped = current == '\\' && !escaped;
            if (current != '\\') {
                escaped = false;
            }
            ++valueEnd;
        }

        if (valueEnd >= json.size()) {
            return {};
        }

        return DecodeJsonString(json.substr(valueStart + 1, valueEnd - valueStart - 1));
    }

    bool LoadCache(std::string& latestVersion, std::string& releaseUrl) {
        AppConfig::UpdateCache cache;
        if (!AppConfig::LoadUpdateCache(cache)) {
            DebugUpdate(std::string("缓存不存在：") + AppConfig::GetConfigPath());
            return false;
        }

        const std::time_t cachedAt = static_cast<std::time_t>(cache.timestamp);
        const std::time_t now = std::time(nullptr);
        if (cachedAt <= 0 || now <= 0 || now - cachedAt > CacheTtlSeconds || cache.tagName.empty()) {
            DebugUpdate(std::string("缓存无效或过期：") + AppConfig::GetConfigPath());
            return false;
        }

        latestVersion = cache.tagName;
        releaseUrl = cache.htmlUrl;
        DebugUpdate(std::string("缓存命中：") + latestVersion);
        return true;
    }

    void SaveCache(const std::string& latestVersion, const std::string& releaseUrl) {
        AppConfig::UpdateCache cache;
        cache.timestamp = static_cast<long long>(std::time(nullptr));
        cache.tagName = latestVersion;
        cache.htmlUrl = releaseUrl;
        AppConfig::SaveUpdateCache(cache);
    }

    bool DownloadText(const char* url, std::string& output) {
        char cachePath[MAX_PATH] = {};
        const HRESULT result = URLDownloadToCacheFileA(nullptr, url, cachePath, MAX_PATH, 0, nullptr);
        if (FAILED(result) || cachePath[0] == '\0') {
            Log::Warn("更新检查失败：无法请求 GitHub Releases API");
            return false;
        }

        std::ifstream file(cachePath, std::ios::binary);
        if (!file.is_open()) {
            Log::Warn("更新检查失败：无法读取 API 响应缓存");
            return false;
        }

        output.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return !output.empty();
    }

    void CheckLatestRelease(std::string apiUrl, std::string currentVersion, bool useCache) {
        checking = true;
        DebugUpdate(std::string("开始检查，API=") + apiUrl + "，本地=" + currentVersion + (useCache ? "，允许缓存" : "，强制刷新"));

        std::string latestVersion;
        std::string releaseUrl;
        if (useCache && LoadCache(latestVersion, releaseUrl)) {
            ApplyVersionInfo(currentVersion, latestVersion, releaseUrl);
            Log::Info(std::string("使用缓存的云端版本：") + latestVersion);
            checking = false;
            return;
        }

        std::string response;
        if (DownloadText(apiUrl.c_str(), response)) {
            latestVersion = ExtractJsonString(response, "tag_name");
            releaseUrl = ExtractJsonString(response, "html_url");

            if (latestVersion.empty()) {
                Log::Warn("更新检查失败：GitHub Releases API 响应缺少 tag_name");
            } else {
                if (releaseUrl.empty()) {
                    releaseUrl = "https://github.com/YuiNijika/XMenu/releases";
                }

                SaveCache(latestVersion, releaseUrl);
                ApplyVersionInfo(currentVersion, latestVersion, releaseUrl);
                Log::Info(std::string("更新检查完成：本地 ") + currentVersion + "，云端 " + latestVersion);
            }
        }

        checking = false;
    }
}

namespace UpdateChecker {
    void Start(const char* apiUrl, const char* currentVersion) {
        if (!apiUrl || !currentVersion || started.exchange(true)) {
            return;
        }

        ApplyVersionInfo(currentVersion, "", "");
        configuredApiUrl = apiUrl;
        configuredCurrentVersion = currentVersion;
        std::thread(CheckLatestRelease, std::string(apiUrl), std::string(currentVersion), true).detach();
    }

    void Refresh() {
        if (checking) {
            DebugUpdate("刷新被跳过：当前已有检查任务");
            return;
        }

        if (configuredApiUrl.empty() || configuredCurrentVersion.empty()) {
            DebugUpdate("刷新被跳过：更新检查尚未初始化");
            return;
        }

        std::thread(CheckLatestRelease, configuredApiUrl, configuredCurrentVersion, false).detach();
    }

    bool IsChecking() {
        return checking;
    }

    bool HasUpdate() {
        if (dismissed) {
            return false;
        }

        if (debugUpdateDialog) {
            std::lock_guard<std::mutex> lock(updateMutex);
            return updateInfo.available;
        }

        if (checking) {
            return false;
        }

        std::lock_guard<std::mutex> lock(updateMutex);
        return updateInfo.available;
    }

    UpdateInfo GetUpdateInfo() {
        std::lock_guard<std::mutex> lock(updateMutex);
        return updateInfo;
    }

    void ForceDebugUpdate() {
        ApplyVersionInfo(configuredCurrentVersion.empty() ? "v0.0.0" : configuredCurrentVersion, "v999.999.999-debug", "https://github.com/YuiNijika/XMenu/releases");
        debugUpdateDialog = true;
        dismissed = false;
        DebugUpdate("已注入调试更新状态，用于测试更新弹窗");
    }

    void Dismiss() {
        debugUpdateDialog = false;
        dismissed = true;
    }
}