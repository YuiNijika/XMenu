#include <windows.h>
#include <shlobj.h>
#include <urlmon.h>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

const char* XMENU_AUTHOR = "鼠子(YuiNijika)";
const char* XMENU_URL = "https://gtamodx.com/mods/xmenu";
const char* XMENU_GITHUB = "https://github.com/YuiNijika/XMenu";
const char* XMENU_GITHUB_API = "https://api.github.com/repos/YuiNijika/XMenu/releases/latest";
const char* XMENU_QQ_GROUP = "https://gtamodx.com/qqun";

namespace {
    constexpr const char* ManifestFileName = "install_manifest.json";
    constexpr const wchar_t* InstallerTitle = L"XMenu Installer";

    std::wstring WideFromAnsi(const std::string& value) {
        if (value.empty()) {
            return L"";
        }
        const int size = MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, nullptr, 0);
        if (size <= 0) {
            return L"";
        }
        std::wstring output(static_cast<std::size_t>(size), L'\0');
        MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, output.data(), size);
        output.pop_back();
        return output;
    }

    std::wstring WideFromUtf8(const std::string& value) {
        if (value.empty()) {
            return L"";
        }
        const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
        if (size <= 0) {
            return L"";
        }
        std::wstring output(static_cast<std::size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, output.data(), size);
        output.pop_back();
        return output;
    }

    std::string AnsiFromWide(const std::wstring& value) {
        if (value.empty()) {
            return "";
        }
        const int size = WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) {
            return "";
        }
        std::string output(static_cast<std::size_t>(size), '\0');
        WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, output.data(), size, nullptr, nullptr);
        output.pop_back();
        return output;
    }

    struct ReleaseInfo {
        std::string tagName;
        std::string assetUrl;
        std::string assetName;
    };

    struct InstalledFile {
        std::string relativePath;
        unsigned long long size = 0;
        unsigned int hash = 2166136261u;
    };

    std::string NormalizeSlashes(std::string value) {
        for (char& ch : value) {
            if (ch == '/') {
                ch = '\\';
            }
        }
        return value;
    }

    std::string JoinPath(const std::string& left, const std::string& right) {
        if (left.empty()) {
            return NormalizeSlashes(right);
        }
        if (right.empty()) {
            return NormalizeSlashes(left);
        }
        const char last = left[left.size() - 1];
        if (last == '\\' || last == '/') {
            return NormalizeSlashes(left + right);
        }
        return NormalizeSlashes(left + "\\" + right);
    }

    std::string ParentDirectory(const std::string& path) {
        const std::size_t slash = path.find_last_of("\\/");
        if (slash == std::string::npos) {
            return "";
        }
        return path.substr(0, slash);
    }

    std::string FileNameOf(const std::string& path) {
        const std::size_t slash = path.find_last_of("\\/");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    bool PathExists(const std::string& path) {
        return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    }

    bool IsDirectory(const std::string& path) {
        const DWORD attr = GetFileAttributesA(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    bool EnsureDirectory(const std::string& path) {
        if (path.empty() || IsDirectory(path)) {
            return true;
        }
        const std::string parent = ParentDirectory(path);
        if (!parent.empty() && !EnsureDirectory(parent)) {
            return false;
        }
        if (CreateDirectoryA(path.c_str(), nullptr)) {
            return true;
        }
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    std::string ReadTextFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool WriteTextFile(const std::string& path, const std::string& text) {
        EnsureDirectory(ParentDirectory(path));
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }
        file << text;
        return !file.fail();
    }

    std::string JsonStringValue(const std::string& json, const std::string& key, std::size_t start = 0) {
        const std::string marker = "\"" + key + "\"";
        std::size_t pos = json.find(marker, start);
        if (pos == std::string::npos) {
            return "";
        }
        pos = json.find(':', pos + marker.size());
        if (pos == std::string::npos) {
            return "";
        }
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) {
            return "";
        }
        std::string value;
        bool escaped = false;
        for (++pos; pos < json.size(); ++pos) {
            const char ch = json[pos];
            if (escaped) {
                value.push_back(ch);
                escaped = false;
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                continue;
            }
            if (ch == '"') {
                break;
            }
            value.push_back(ch);
        }
        return value;
    }

    bool EndsWithInsensitive(const std::string& value, const std::string& suffix) {
        if (value.size() < suffix.size()) {
            return false;
        }
        for (std::size_t i = 0; i < suffix.size(); ++i) {
            const char a = static_cast<char>(tolower(static_cast<unsigned char>(value[value.size() - suffix.size() + i])));
            const char b = static_cast<char>(tolower(static_cast<unsigned char>(suffix[i])));
            if (a != b) {
                return false;
            }
        }
        return true;
    }

    bool ContainsInsensitive(const std::string& value, const std::string& needle) {
        if (needle.empty() || value.size() < needle.size()) {
            return false;
        }
        for (std::size_t i = 0; i + needle.size() <= value.size(); ++i) {
            bool matched = true;
            for (std::size_t j = 0; j < needle.size(); ++j) {
                const char a = static_cast<char>(tolower(static_cast<unsigned char>(value[i + j])));
                const char b = static_cast<char>(tolower(static_cast<unsigned char>(needle[j])));
                if (a != b) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return true;
            }
        }
        return false;
    }

    ReleaseInfo ParseReleaseInfo(const std::string& json) {
        ReleaseInfo info;
        info.tagName = JsonStringValue(json, "tag_name");

        std::size_t search = 0;
        while (true) {
            const std::size_t urlPos = json.find("\"browser_download_url\"", search);
            if (urlPos == std::string::npos) {
                break;
            }
            const std::string url = JsonStringValue(json, "browser_download_url", urlPos);
            const std::string name = FileNameOf(url);
            if (EndsWithInsensitive(name, ".zip")) {
                info.assetUrl = url;
                info.assetName = name;
                return info;
            }
            search = urlPos + 1;
        }
        return info;
    }

    std::string TempPathFor(const std::string& fileName) {
        char tempPath[MAX_PATH]{};
        GetTempPathA(MAX_PATH, tempPath);
        return JoinPath(tempPath, std::string("XMenuInstaller_") + fileName);
    }

    bool DownloadFile(const std::string& url, const std::string& path) {
        DeleteFileA(path.c_str());
        return SUCCEEDED(URLDownloadToFileA(nullptr, url.c_str(), path.c_str(), 0, nullptr));
    }

    bool RunHiddenAndWait(const std::string& commandLine) {
        STARTUPINFOA startup{};
        PROCESS_INFORMATION process{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;

        std::vector<char> mutableCommand(commandLine.begin(), commandLine.end());
        mutableCommand.push_back('\0');

        if (!CreateProcessA(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
            return false;
        }

        WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exitCode = 1;
        GetExitCodeProcess(process.hProcess, &exitCode);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return exitCode == 0;
    }

    bool ExtractZip(const std::string& zipPath, const std::string& outputDir) {
        EnsureDirectory(outputDir);
        const std::string command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -LiteralPath '" + zipPath + "' -DestinationPath '" + outputDir + "' -Force\"";
        return RunHiddenAndWait(command);
    }

    bool CopyFileEnsureDirectory(const std::string& source, const std::string& target, bool overwrite) {
        EnsureDirectory(ParentDirectory(target));
        return CopyFileA(source.c_str(), target.c_str(), overwrite ? FALSE : TRUE) != 0;
    }

    unsigned int UpdateFnv1a(unsigned int hash, const unsigned char* data, DWORD size) {
        for (DWORD i = 0; i < size; ++i) {
            hash ^= data[i];
            hash *= 16777619u;
        }
        return hash;
    }

    InstalledFile BuildFileRecord(const std::string& root, const std::string& relativePath) {
        InstalledFile record;
        record.relativePath = NormalizeSlashes(relativePath);
        record.hash = 2166136261u;

        const std::string path = JoinPath(root, relativePath);
        HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            return record;
        }

        LARGE_INTEGER size{};
        GetFileSizeEx(file, &size);
        record.size = static_cast<unsigned long long>(size.QuadPart);

        unsigned char buffer[8192];
        DWORD read = 0;
        while (ReadFile(file, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
            record.hash = UpdateFnv1a(record.hash, buffer, read);
        }
        CloseHandle(file);
        return record;
    }

    void CollectFiles(const std::string& root, const std::string& relativeDir, std::vector<std::string>& output) {
        const std::string searchDir = JoinPath(root, relativeDir);
        WIN32_FIND_DATAA data{};
        HANDLE find = FindFirstFileA(JoinPath(searchDir, "*").c_str(), &data);
        if (find == INVALID_HANDLE_VALUE) {
            return;
        }

        do {
            const std::string name = data.cFileName;
            if (name == "." || name == "..") {
                continue;
            }
            const std::string relativePath = relativeDir.empty() ? name : JoinPath(relativeDir, name);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                CollectFiles(root, relativePath, output);
            } else {
                output.push_back(relativePath);
            }
        } while (FindNextFileA(find, &data));
        FindClose(find);
    }

    std::string FindFirstFileByName(const std::string& root, const std::string& fileName) {
        std::vector<std::string> files;
        CollectFiles(root, "", files);
        for (const std::string& relative : files) {
            if (_stricmp(FileNameOf(relative).c_str(), fileName.c_str()) == 0) {
                return JoinPath(root, relative);
            }
        }
        return "";
    }

    std::vector<std::string> FindAllFilesByName(const std::string& root, const std::string& fileName) {
        std::vector<std::string> result;
        std::vector<std::string> files;
        CollectFiles(root, "", files);
        for (const std::string& relative : files) {
            if (_stricmp(FileNameOf(relative).c_str(), fileName.c_str()) == 0) {
                result.push_back(JoinPath(root, relative));
            }
        }
        return result;
    }

    std::vector<std::string> FindAllFilesByNamePart(const std::string& root, const std::string& namePart) {
        std::vector<std::string> result;
        std::vector<std::string> files;
        CollectFiles(root, "", files);
        for (const std::string& relative : files) {
            const std::string name = FileNameOf(relative);
            if (ContainsInsensitive(name, namePart)) {
                result.push_back(JoinPath(root, relative));
            }
        }
        return result;
    }

    std::string FindDirectoryContaining(const std::string& root, const std::string& fileName) {
        const std::string file = FindFirstFileByName(root, fileName);
        return file.empty() ? "" : ParentDirectory(file);
    }

    int AskOverwriteDependency(const std::string& fileName, const std::string& target) {
        const std::wstring message = L"检测到游戏根目录已有依赖：\n\n" + WideFromAnsi(target) + L"\n\n选择“是”覆盖，选择“否”跳过。";
        return MessageBoxW(HWND_DESKTOP, message.c_str(), WideFromAnsi(fileName).c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
    }

    std::string PickGameRoot() {
        BROWSEINFOW info{};
        info.hwndOwner = HWND_DESKTOP;
        info.lpszTitle = L"选择 GTA 游戏根目录";
        info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        PIDLIST_ABSOLUTE idList = SHBrowseForFolderW(&info);
        if (!idList) {
            return "";
        }

        wchar_t path[MAX_PATH]{};
        SHGetPathFromIDListW(idList, path);
        CoTaskMemFree(idList);
        return AnsiFromWide(path);
    }

    std::string ReadInstalledVersion(const std::string& gameRoot) {
        const std::string configPath = JoinPath(gameRoot, "plugins\\XMenu\\config.json");
        const std::string config = ReadTextFile(configPath);
        const std::string xmenuSection = "\"XMenu\"";
        const std::size_t section = config.find(xmenuSection);
        return section == std::string::npos ? JsonStringValue(config, "version") : JsonStringValue(config, "version", section);
    }

    bool VerifyInstalledFiles(const std::string& gameRoot, std::string& report) {
        const std::string manifest = ReadTextFile(JoinPath(gameRoot, std::string("plugins\\XMenu\\") + ManifestFileName));
        if (manifest.empty()) {
            report = "未找到安装清单，无法做完整性检测。";
            return false;
        }

        bool ok = true;
        std::size_t pos = 0;
        while (true) {
            const std::size_t pathPos = manifest.find("\"path\"", pos);
            if (pathPos == std::string::npos) {
                break;
            }
            const std::string relativePath = JsonStringValue(manifest, "path", pathPos);
            const std::size_t sizePos = manifest.find("\"size\"", pathPos);
            const std::size_t hashPos = manifest.find("\"hash\"", pathPos);
            if (relativePath.empty() || sizePos == std::string::npos || hashPos == std::string::npos) {
                break;
            }

            const unsigned long long expectedSize = strtoull(manifest.c_str() + manifest.find(':', sizePos) + 1, nullptr, 10);
            const unsigned int expectedHash = static_cast<unsigned int>(strtoul(manifest.c_str() + manifest.find(':', hashPos) + 1, nullptr, 10));
            const InstalledFile actual = BuildFileRecord(gameRoot, relativePath);
            if (!PathExists(JoinPath(gameRoot, relativePath)) || actual.size != expectedSize || actual.hash != expectedHash) {
                ok = false;
                report += "异常: " + relativePath + "\r\n";
            }
            pos = hashPos + 1;
        }

        if (ok) {
            report = "当前安装文件完整。";
        }
        return ok;
    }

    void AddManifestRecord(std::vector<InstalledFile>& files, const std::string& gameRoot, const std::string& relativePath) {
        files.push_back(BuildFileRecord(gameRoot, relativePath));
    }

    std::string EscapeJson(const std::string& value) {
        std::string result;
        for (const char ch : value) {
            if (ch == '\\' || ch == '"') {
                result.push_back('\\');
            }
            result.push_back(ch);
        }
        return result;
    }

    bool SyncConfigVersion(const std::string& gameRoot, const std::string& version) {
        const std::string configPath = JoinPath(gameRoot, "plugins\\XMenu\\config.json");
        std::string config = ReadTextFile(configPath);
        const std::string escapedVersion = EscapeJson(version);

        if (config.empty()) {
            return WriteTextFile(configPath, "{\n  \"XMenu\": {\n    \"version\": \"" + escapedVersion + "\"\n  }\n}\n");
        }

        const std::string xmenuKey = "\"XMenu\"";
        const std::size_t xmenuPos = config.find(xmenuKey);
        if (xmenuPos == std::string::npos) {
            const std::size_t objectStart = config.find('{');
            if (objectStart == std::string::npos) {
                return WriteTextFile(configPath, "{\n  \"XMenu\": {\n    \"version\": \"" + escapedVersion + "\"\n  }\n}\n");
            }
            const std::size_t objectEnd = config.find('}', objectStart + 1);
            const bool hasExistingRootFields = objectEnd != std::string::npos && config.find('"', objectStart + 1) < objectEnd;
            config.insert(objectStart + 1, std::string("\n  \"XMenu\": {\n    \"version\": \"") + escapedVersion + "\"\n  }" + (hasExistingRootFields ? "," : ""));
            return WriteTextFile(configPath, config);
        }

        const std::size_t xmenuObjectStart = config.find('{', xmenuPos + xmenuKey.size());
        if (xmenuObjectStart == std::string::npos) {
            return false;
        }

        const std::size_t versionPos = config.find("\"version\"", xmenuObjectStart);
        const std::size_t xmenuObjectEnd = config.find('}', xmenuObjectStart + 1);
        if (xmenuObjectEnd == std::string::npos) {
            return false;
        }

        if (versionPos != std::string::npos && versionPos < xmenuObjectEnd) {
            const std::size_t colon = config.find(':', versionPos);
            const std::size_t valueStart = colon == std::string::npos ? std::string::npos : config.find('"', colon + 1);
            const std::size_t valueEnd = valueStart == std::string::npos ? std::string::npos : config.find('"', valueStart + 1);
            if (valueEnd == std::string::npos) {
                return false;
            }
            config.replace(valueStart + 1, valueEnd - valueStart - 1, escapedVersion);
            return WriteTextFile(configPath, config);
        }

        const bool hasExistingFields = config.find('"', xmenuObjectStart + 1) < xmenuObjectEnd;
        config.insert(xmenuObjectStart + 1, std::string("\n    \"version\": \"") + escapedVersion + "\"" + (hasExistingFields ? "," : ""));
        return WriteTextFile(configPath, config);
    }

    bool WriteManifest(const std::string& gameRoot, const std::string& version, const std::vector<InstalledFile>& files) {
        std::ostringstream json;
        json << "{\n";
        json << "  \"version\": \"" << EscapeJson(version) << "\",\n";
        json << "  \"files\": [\n";
        for (std::size_t i = 0; i < files.size(); ++i) {
            json << "    {\"path\": \"" << EscapeJson(files[i].relativePath) << "\", \"size\": " << files[i].size << ", \"hash\": " << files[i].hash << "}";
            if (i + 1 < files.size()) {
                json << ",";
            }
            json << "\n";
        }
        json << "  ]\n";
        json << "}\n";
        return WriteTextFile(JoinPath(gameRoot, std::string("plugins\\XMenu\\") + ManifestFileName), json.str());
    }

    bool InstallDirectoryContents(const std::string& sourceDir, const std::string& targetDir, const std::string& targetRelativePrefix, const std::string& gameRoot, std::vector<InstalledFile>& manifestFiles) {
        std::vector<std::string> files;
        CollectFiles(sourceDir, "", files);
        for (const std::string& relative : files) {
            const std::string source = JoinPath(sourceDir, relative);
            const std::string target = JoinPath(targetDir, relative);
            if (!CopyFileEnsureDirectory(source, target, true)) {
                return false;
            }
            AddManifestRecord(manifestFiles, gameRoot, JoinPath(targetRelativePrefix, relative));
        }
        return true;
    }

    bool InstallRelease(const std::string& extractedRoot, const std::string& gameRoot, const std::string& version) {
        const std::string pluginsDir = JoinPath(gameRoot, "plugins");
        const std::string xmenuDir = JoinPath(pluginsDir, "XMenu");
        EnsureDirectory(pluginsDir);
        EnsureDirectory(xmenuDir);

        std::vector<InstalledFile> manifestFiles;

        const std::string asi = FindFirstFileByName(extractedRoot, "XMenu.asi");
        if (asi.empty() || !CopyFileEnsureDirectory(asi, JoinPath(pluginsDir, "XMenu.asi"), true)) {
            MessageBoxW(HWND_DESKTOP, L"安装失败：发布包中缺少 XMenu.asi。", InstallerTitle, MB_ICONERROR);
            return false;
        }
        AddManifestRecord(manifestFiles, gameRoot, "plugins\\XMenu.asi");

        const char* payloads[] = {"XMenuSA.dll", "XMenuVC.dll", "XMenuIII.dll"};
        for (const char* payload : payloads) {
            const std::string source = FindFirstFileByName(extractedRoot, payload);
            if (!source.empty()) {
                CopyFileEnsureDirectory(source, JoinPath(xmenuDir, payload), true);
                AddManifestRecord(manifestFiles, gameRoot, JoinPath("plugins\\XMenu", payload));
            }
        }

        const std::string dataDir = FindDirectoryContaining(extractedRoot, "maps.json");
        if (!dataDir.empty()) {
            std::string rootDataDir = dataDir;
            for (int i = 0; i < 2; ++i) {
                const std::string parent = ParentDirectory(rootDataDir);
                if (parent.empty()) {
                    break;
                }
                if (EndsWithInsensitive(FileNameOf(rootDataDir), "sa") || EndsWithInsensitive(FileNameOf(rootDataDir), "vc") || EndsWithInsensitive(FileNameOf(rootDataDir), "iii")) {
                    rootDataDir = parent;
                    break;
                }
                rootDataDir = parent;
            }
            InstallDirectoryContents(rootDataDir, JoinPath(xmenuDir, "data"), "plugins\\XMenu\\data", gameRoot, manifestFiles);
        }

        const std::vector<std::string> silentPatchFiles = FindAllFilesByNamePart(extractedRoot, "SilentPatch");
        for (const std::string& source : silentPatchFiles) {
            const std::string name = FileNameOf(source);
            if (!EndsWithInsensitive(name, ".asi") && !EndsWithInsensitive(name, ".dll")) {
                continue;
            }
            CopyFileEnsureDirectory(source, JoinPath(pluginsDir, name), true);
            AddManifestRecord(manifestFiles, gameRoot, JoinPath("plugins", name));
        }

        const char* rootDependencies[] = {"Ultimate-ASI-Loader.asi", "dinput8.dll", "d3d8.dll", "d3d9.dll"};
        for (const char* dep : rootDependencies) {
            const std::string source = FindFirstFileByName(extractedRoot, dep);
            if (source.empty()) {
                continue;
            }
            const std::string target = JoinPath(gameRoot, dep);
            bool overwrite = true;
            if (PathExists(target)) {
                overwrite = AskOverwriteDependency(dep, target) == IDYES;
            }
            if (overwrite && CopyFileEnsureDirectory(source, target, true)) {
                AddManifestRecord(manifestFiles, gameRoot, dep);
            }
        }

        SyncConfigVersion(gameRoot, version);
        WriteManifest(gameRoot, version, manifestFiles);
        return true;
    }

    bool RunInstaller() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const std::string gameRoot = PickGameRoot();
        if (gameRoot.empty()) {
            CoUninitialize();
            return false;
        }

        const std::string installedVersion = ReadInstalledVersion(gameRoot);
        std::string integrityReport;
        const bool integrityOk = VerifyInstalledFiles(gameRoot, integrityReport);
        if (!installedVersion.empty()) {
            const std::wstring message = L"当前本地版本：" + WideFromUtf8(installedVersion) + L"\n" + WideFromUtf8(integrityReport) + L"\n\n继续检查 GitHub 最新版本并安装？";
            if (MessageBoxW(HWND_DESKTOP, message.c_str(), InstallerTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) != IDYES) {
                CoUninitialize();
                return integrityOk;
            }
        }

        const std::string apiJsonPath = TempPathFor("release.json");
        if (!DownloadFile(XMENU_GITHUB_API, apiJsonPath)) {
            MessageBoxW(HWND_DESKTOP, L"请求 GitHub API 失败。", InstallerTitle, MB_ICONERROR);
            CoUninitialize();
            return false;
        }

        const ReleaseInfo release = ParseReleaseInfo(ReadTextFile(apiJsonPath));
        if (release.tagName.empty() || release.assetUrl.empty()) {
            MessageBoxW(HWND_DESKTOP, L"GitHub release 中未找到可安装的 zip 资产。", InstallerTitle, MB_ICONERROR);
            CoUninitialize();
            return false;
        }

        const std::string zipPath = TempPathFor(release.assetName.empty() ? "release.zip" : release.assetName);
        if (!DownloadFile(release.assetUrl, zipPath)) {
            MessageBoxW(HWND_DESKTOP, L"下载安装包失败。", InstallerTitle, MB_ICONERROR);
            CoUninitialize();
            return false;
        }

        const std::string extractDir = TempPathFor("extract");
        RunHiddenAndWait("cmd.exe /c rmdir /s /q \"" + extractDir + "\"");
        if (!ExtractZip(zipPath, extractDir)) {
            MessageBoxW(HWND_DESKTOP, L"解压安装包失败。", InstallerTitle, MB_ICONERROR);
            CoUninitialize();
            return false;
        }

        const bool ok = InstallRelease(extractDir, gameRoot, release.tagName);
        if (ok) {
            std::string finalReport;
            VerifyInstalledFiles(gameRoot, finalReport);
            const std::wstring message = L"XMenu 安装/更新完成。\n\n版本：" + WideFromUtf8(release.tagName) + L"\n" + WideFromUtf8(finalReport);
            MessageBoxW(HWND_DESKTOP, message.c_str(), InstallerTitle, MB_ICONINFORMATION);
        }
        CoUninitialize();
        return ok;
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return RunInstaller() ? 0 : 1;
}
