#include "UpdateChecker.h"
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

namespace {
    constexpr std::time_t CacheTtlSeconds = 24 * 60 * 60;

    std::mutex updateMutex;
    std::atomic<bool> checking{ false };
    std::atomic<bool> started{ false };
    std::atomic<bool> dismissed{ false };
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

    std::string CachePath() {
        char path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (size == 0) {
            return "XMenu.update.cache";
        }

        std::string directory(path, size);
        const std::size_t slash = directory.find_last_of("\\/");
        if (slash != std::string::npos) {
            directory = directory.substr(0, slash + 1);
        } else {
            directory.clear();
        }
        return directory + "XMenu.update.cache";
    }

    bool LoadCache(std::string& latestVersion, std::string& releaseUrl) {
        std::ifstream file(CachePath(), std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::string timestampLine;
        std::getline(file, timestampLine);
        std::getline(file, latestVersion);
        std::getline(file, releaseUrl);

        const std::time_t cachedAt = static_cast<std::time_t>(std::atoll(timestampLine.c_str()));
        const std::time_t now = std::time(nullptr);
        if (cachedAt <= 0 || now <= 0 || now - cachedAt > CacheTtlSeconds || latestVersion.empty()) {
            return false;
        }

        return true;
    }

    void SaveCache(const std::string& latestVersion, const std::string& releaseUrl) {
        std::ofstream file(CachePath(), std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            Log::Warn("更新检查缓存写入失败");
            return;
        }

        file << static_cast<long long>(std::time(nullptr)) << '\n'
             << latestVersion << '\n'
             << releaseUrl << '\n';
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

    void CheckLatestRelease(std::string apiUrl, std::string currentVersion) {
        checking = true;

        std::string latestVersion;
        std::string releaseUrl;
        if (LoadCache(latestVersion, releaseUrl)) {
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
        std::thread(CheckLatestRelease, std::string(apiUrl), std::string(currentVersion)).detach();
    }

    bool IsChecking() {
        return checking;
    }

    bool HasUpdate() {
        if (checking || dismissed) {
            return false;
        }

        std::lock_guard<std::mutex> lock(updateMutex);
        return updateInfo.available;
    }

    UpdateInfo GetUpdateInfo() {
        std::lock_guard<std::mutex> lock(updateMutex);
        return updateInfo;
    }

    void Dismiss() {
        dismissed = true;
    }
}