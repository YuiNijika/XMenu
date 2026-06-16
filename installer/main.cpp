#include <windows.h>
#include <shlobj.h>
#include <urlmon.h>
#include <commctrl.h>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <stdexcept>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

    struct DownloadSource {
        const wchar_t* label;
        const char* proxyPrefix;
    };

    const DownloadSource DownloadSources[] = {
        { L"GitHub 官方源", "" },
        { L"gh-proxy.org Cloudflare (v4)", "https://gh-proxy.org/" },
        { L"v4.gh-proxy.org Cloudflare (v4)", "https://v4.gh-proxy.org/" },
        { L"v6.gh-proxy.org Cloudflare (v4/v6)", "https://v6.gh-proxy.org/" },
        { L"cdn.gh-proxy.org Fastly (v4)", "https://cdn.gh-proxy.org/" }
    };

    constexpr int DefaultDownloadSourceIndex = 0;

    struct ReleaseInfo {
        std::string tagName;
        std::string assetUrl;
        std::string assetName;
        std::string body;
    };

    struct InstalledFile {
        std::string relativePath;
        unsigned long long size = 0;
        unsigned int hash = 2166136261u;
    };

    struct InstallOptions {
        bool installXMenuSA = false;
        bool installXMenuVC = false;
        bool installXMenuIII = false;
        bool installRootDependencies = true;
    };

    enum class GameType {
        Unknown,
        GTA3,
        GTAVC,
        GTASA
    };

    const wchar_t* GameTypeName(GameType gameType) {
        switch (gameType) {
        case GameType::GTA3:
            return L"GTA III";
        case GameType::GTAVC:
            return L"GTA Vice City";
        case GameType::GTASA:
            return L"GTA San Andreas";
        default:
            return L"未知";
        }
    }

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

    std::string CurrentTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
        localtime_s(&localTime, &now);

        char buffer[32]{};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
        return buffer;
    }

    void AppendInstallLog(const std::string& gameRoot, const std::string& message) {
        if (gameRoot.empty()) {
            return;
        }
        const std::string logPath = JoinPath(gameRoot, "plugins\\XMenu\\install.log");
        EnsureDirectory(ParentDirectory(logPath));
        std::ofstream file(logPath, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            return;
        }
        file << "[" << CurrentTimestamp() << "] " << message << "\r\n";
    }

    std::string FormatDuration(DWORD startTick, DWORD endTick) {
        const DWORD elapsedMs = endTick >= startTick ? endTick - startTick : 0;
        std::ostringstream output;
        output << (elapsedMs / 1000) << ".";
        const DWORD decimal = (elapsedMs % 1000) / 100;
        output << decimal << "s";
        return output.str();
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
                if (ch == 'n') value.push_back('\n');
                else if (ch == 'r') value.push_back('\r');
                else if (ch == 't') value.push_back('\t');
                else value.push_back(ch);
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
        info.body = JsonStringValue(json, "body");

        struct CandidateAsset {
            std::string url;
            std::string name;
        };

        std::vector<CandidateAsset> zipAssets;
        std::size_t search = 0;
        while (true) {
            const std::size_t urlPos = json.find("\"browser_download_url\"", search);
            if (urlPos == std::string::npos) {
                break;
            }
            const std::string url = JsonStringValue(json, "browser_download_url", urlPos);
            const std::string name = FileNameOf(url);
            if (EndsWithInsensitive(name, ".zip")) {
                zipAssets.push_back({ url, name });
            }
            search = urlPos + 1;
        }

        for (const CandidateAsset& asset : zipAssets) {
            if (_stricmp(asset.name.c_str(), "XMenuIII.VC.SA.zip") == 0) {
                info.assetUrl = asset.url;
                info.assetName = asset.name;
                return info;
            }
        }

        for (const CandidateAsset& asset : zipAssets) {
            if (ContainsInsensitive(asset.name, "XMenu")) {
                info.assetUrl = asset.url;
                info.assetName = asset.name;
                return info;
            }
        }

        if (!zipAssets.empty()) {
            info.assetUrl = zipAssets.front().url;
            info.assetName = zipAssets.front().name;
        }
        return info;
    }

    std::string TempPathFor(const std::string& fileName) {
        char tempPath[MAX_PATH]{};
        GetTempPathA(MAX_PATH, tempPath);
        return JoinPath(tempPath, std::string("XMenuInstaller_") + fileName);
    }

    class DownloadProgressCallback final : public IBindStatusCallback {
    public:
        using ProgressHandler = void (*)(ULONG current, ULONG total);

        explicit DownloadProgressCallback(ProgressHandler handler) : handler_(handler) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
            if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) {
                *object = static_cast<IBindStatusCallback*>(this);
                return S_OK;
            }
            *object = nullptr;
            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override {
            return 1;
        }

        ULONG STDMETHODCALLTYPE Release() override {
            return 1;
        }

        HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD, IBinding*) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE GetPriority(LONG*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnLowResource(DWORD) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT, LPCWSTR) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD*, BINDINFO*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD, DWORD, FORMATETC*, STGMEDIUM*) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID, IUnknown*) override { return S_OK; }

        HRESULT STDMETHODCALLTYPE OnProgress(ULONG progress, ULONG progressMax, ULONG statusCode, LPCWSTR) override {
            if (handler_ && (statusCode == BINDSTATUS_DOWNLOADINGDATA || statusCode == BINDSTATUS_ENDDOWNLOADDATA)) {
                handler_(progress, progressMax);
            }
            return S_OK;
        }

    private:
        ProgressHandler handler_ = nullptr;
    };

    std::string LastSystemErrorText(HRESULT hr) {
        std::ostringstream stream;
        stream << "HRESULT=0x";
        stream << std::hex << static_cast<unsigned long>(hr);
        return stream.str();
    }

    std::string BuildDownloadUrl(const std::string& url, int sourceIndex) {
        if (sourceIndex <= 0 || sourceIndex >= static_cast<int>(sizeof(DownloadSources) / sizeof(DownloadSources[0]))) {
            return url;
        }
        return std::string(DownloadSources[sourceIndex].proxyPrefix) + url;
    }

    void ThrowDownloadFailure(const std::string& sourceName, const std::string& url, HRESULT hr) {
        std::ostringstream message;
        message << "download failed, source=" << sourceName << ", url=" << url << ", " << LastSystemErrorText(hr);
        throw std::runtime_error(message.str());
    }

    void DownloadFileOrThrow(const std::string& url, const std::string& path, const std::string& sourceName, DownloadProgressCallback::ProgressHandler progressHandler = nullptr) {
        DeleteFileA(path.c_str());
        DownloadProgressCallback callback(progressHandler);
        const HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), path.c_str(), 0, progressHandler ? &callback : nullptr);
        if (FAILED(hr)) {
            ThrowDownloadFailure(sourceName, url, hr);
        }
        if (!PathExists(path)) {
            throw std::runtime_error("download failed, file was not created: " + path);
        }
    }

    bool DownloadFile(const std::string& url, const std::string& path, DownloadProgressCallback::ProgressHandler progressHandler = nullptr) {
        try {
            DownloadFileOrThrow(url, path, "GitHub official", progressHandler);
            return true;
        } catch (...) {
            return false;
        }
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

    void ExtractNestedArchivesForDependencies(const std::string& extractedRoot, const std::string& gameRoot) {
        std::vector<std::string> files;
        CollectFiles(extractedRoot, "", files);
        int archiveIndex = 0;
        for (const std::string& relative : files) {
            const std::string name = FileNameOf(relative);
            if (!EndsWithInsensitive(name, ".zip")) {
                continue;
            }
            if (!ContainsInsensitive(relative, "SilentPatch")
                && !ContainsInsensitive(relative, "Ultimate")
                && !ContainsInsensitive(relative, "ASI")
                && !ContainsInsensitive(relative, "D3D8to9")) {
                continue;
            }
            const std::string nestedDir = JoinPath(extractedRoot, "_nested_dependency_" + std::to_string(++archiveIndex));
            const std::string zipPath = JoinPath(extractedRoot, relative);
            if (ExtractZip(zipPath, nestedDir)) {
                AppendInstallLog(gameRoot, "Extracted nested dependency archive: " + relative + " -> " + nestedDir);
            } else {
                AppendInstallLog(gameRoot, "Failed to extract nested dependency archive: " + relative);
            }
        }
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

    bool IsInstallablePluginBinary(const std::string& fileName) {
        return EndsWithInsensitive(fileName, ".asi") || EndsWithInsensitive(fileName, ".dll");
    }

    bool LooksLikeSilentPatch(const std::string& relativePath) {
        const std::string name = FileNameOf(relativePath);
        return IsInstallablePluginBinary(name) && ContainsInsensitive(relativePath, "SilentPatch");
    }

    bool IsRootDependencyName(const std::string& fileName) {
        return _stricmp(fileName.c_str(), "Ultimate-ASI-Loader.asi") == 0
            || _stricmp(fileName.c_str(), "dinput8.dll") == 0
            || _stricmp(fileName.c_str(), "d3d8.dll") == 0
            || _stricmp(fileName.c_str(), "d3d9.dll") == 0
            || (EndsWithInsensitive(fileName, ".asi") && ContainsInsensitive(fileName, "Ultimate") && ContainsInsensitive(fileName, "ASI"))
            || (EndsWithInsensitive(fileName, ".dll") && ContainsInsensitive(fileName, "D3D8to9"));
    }

    std::vector<std::string> CollectInstallableSilentPatchFiles(const std::string& extractedRoot) {
        std::vector<std::string> result;
        std::vector<std::string> files;
        CollectFiles(extractedRoot, "", files);
        for (const std::string& relative : files) {
            if (LooksLikeSilentPatch(relative)) {
                result.push_back(JoinPath(extractedRoot, relative));
            }
        }
        return result;
    }

    std::vector<std::string> CollectInstallableRootDependencies(const std::string& extractedRoot) {
        std::vector<std::string> result;
        std::vector<std::string> files;
        CollectFiles(extractedRoot, "", files);
        for (const std::string& relative : files) {
            const std::string name = FileNameOf(relative);
            if (IsRootDependencyName(name)) {
                result.push_back(JoinPath(extractedRoot, relative));
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

    GameType DetectGameType(const std::string& gameRoot) {
        if (PathExists(JoinPath(gameRoot, "gta_sa.exe"))) {
            return GameType::GTASA;
        }
        if (PathExists(JoinPath(gameRoot, "gta-vc.exe")) || PathExists(JoinPath(gameRoot, "gtavc.exe"))) {
            return GameType::GTAVC;
        }
        if (PathExists(JoinPath(gameRoot, "gta3.exe"))) {
            return GameType::GTA3;
        }
        return GameType::Unknown;
    }

    std::string GameModuleName(GameType gameType) {
        switch (gameType) {
        case GameType::GTA3:
            return "XMenuIII.dll";
        case GameType::GTAVC:
            return "XMenuVC.dll";
        case GameType::GTASA:
            return "XMenuSA.dll";
        default:
            return "";
        }
    }

    InstallOptions DefaultInstallOptionsForGame(GameType gameType) {
        InstallOptions options;
        options.installXMenuIII = gameType == GameType::GTA3;
        options.installXMenuVC = gameType == GameType::GTAVC;
        options.installXMenuSA = gameType == GameType::GTASA;
        return options;
    }

    std::string BuildSelectedComponentSummary(const InstallOptions& options) {
        std::ostringstream summary;
        summary << "- XMenu.asi -> plugins\\XMenu.asi\r\n";
        if (options.installXMenuIII) {
            summary << "- GTA III 模块 -> plugins\\XMenu\\XMenuIII.dll\r\n";
        }
        if (options.installXMenuVC) {
            summary << "- GTA VC 模块 -> plugins\\XMenu\\XMenuVC.dll\r\n";
        }
        if (options.installXMenuSA) {
            summary << "- GTA SA 模块 -> plugins\\XMenu\\XMenuSA.dll\r\n";
        }
        summary << "- XMenu data/config -> plugins\\XMenu\\\r\n";
        summary << "- SilentPatch -> plugins\\\r\n";
        summary << (options.installRootDependencies ? "- Ultimate ASI Loader / D3D8to9 -> 游戏根目录\r\n" : "- Ultimate ASI Loader / D3D8to9 -> 跳过\r\n");
        return summary.str();
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
        json << "  \"installedAt\": \"" << EscapeJson(CurrentTimestamp()) << "\",\n";
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

    bool HasInstallableSilentPatch(const std::string& extractedRoot) {
        return !CollectInstallableSilentPatchFiles(extractedRoot).empty();
    }

    bool HasInstallableRootDependencies(const std::string& extractedRoot) {
        return !CollectInstallableRootDependencies(extractedRoot).empty();
    }

    std::string BuildComponentSummary(const std::string& extractedRoot, const InstallOptions& options) {
        std::ostringstream summary;
        summary << "- XMenu.asi -> plugins\\XMenu.asi\r\n";
        summary << "- XMenu payload/config/data -> plugins\\XMenu\\\r\n";

        if (HasInstallableSilentPatch(extractedRoot)) {
            summary << "- SilentPatch -> plugins\\\r\n";
        }

        if (HasInstallableRootDependencies(extractedRoot)) {
            summary << (options.installRootDependencies ? "- Ultimate ASI Loader / D3D8to9 -> 游戏根目录\r\n" : "- Ultimate ASI Loader / D3D8to9 -> 跳过\r\n");
        }

        return summary.str();
    }

    bool ConfirmInstallPlan(const std::string& gameRoot, const std::string& installedVersion, const std::string& githubVersion, const std::string& assetName, const std::string& integrityReport, const std::string& componentSummary) {
        const bool isUpdate = !installedVersion.empty();
        std::wstring message;
        message += isUpdate ? L"将执行更新：\n\n" : L"将执行安装：\n\n";
        message += L"游戏目录：" + WideFromAnsi(gameRoot) + L"\n";
        if (isUpdate) {
            message += L"本地版本：" + WideFromUtf8(installedVersion) + L"\n";
            message += L"GitHub 版本：" + WideFromUtf8(githubVersion) + L"\n";
        } else {
            message += L"GitHub 版本：" + WideFromUtf8(githubVersion) + L"\n";
        }
        message += L"Release 包：" + WideFromUtf8(assetName) + L"\n";
        message += L"完整性状态：" + WideFromUtf8(integrityReport) + L"\n\n";
        message += L"将安装的组件：\n" + WideFromUtf8(componentSummary) + L"\n继续？";

        return MessageBoxW(HWND_DESKTOP, message.c_str(), InstallerTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDYES;
    }

    bool AskOptionalComponent(const wchar_t* title, const std::wstring& description) {
        const std::wstring message = description + L"\n\n选择“是”安装，选择“否”跳过。";
        return MessageBoxW(HWND_DESKTOP, message.c_str(), title, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDYES;
    }

    InstallOptions PickInstallOptions(const std::string& gameRoot, const std::string& extractedRoot) {
        InstallOptions options;

        if (HasInstallableSilentPatch(extractedRoot)) {
            AppendInstallLog(gameRoot, "SilentPatch component will be installed to plugins");
        } else {
            AppendInstallLog(gameRoot, "SilentPatch component not found in release asset");
        }

        if (HasInstallableRootDependencies(extractedRoot)) {
            options.installRootDependencies = AskOptionalComponent(L"选择安装组件", L"发布包中检测到 Ultimate ASI Loader / D3D8to9 依赖。\n安装位置：游戏根目录");
        } else {
            options.installRootDependencies = false;
            AppendInstallLog(gameRoot, "Root dependency component not found in release asset");
        }

        return options;
    }

    bool InstallRelease(const std::string& extractedRoot, const std::string& gameRoot, const std::string& version, const InstallOptions& options) {
        const std::string pluginsDir = JoinPath(gameRoot, "plugins");
        const std::string xmenuDir = JoinPath(pluginsDir, "XMenu");
        EnsureDirectory(pluginsDir);
        EnsureDirectory(xmenuDir);

        std::vector<InstalledFile> manifestFiles;
        ExtractNestedArchivesForDependencies(extractedRoot, gameRoot);

        const std::string asi = FindFirstFileByName(extractedRoot, "XMenu.asi");
        if (asi.empty() || !CopyFileEnsureDirectory(asi, JoinPath(pluginsDir, "XMenu.asi"), true)) {
            AppendInstallLog(gameRoot, "ERROR missing or failed to copy plugins\\XMenu.asi");
            MessageBoxW(HWND_DESKTOP, L"安装失败：发布包中缺少 XMenu.asi。", InstallerTitle, MB_ICONERROR);
            return false;
        }
        AddManifestRecord(manifestFiles, gameRoot, "plugins\\XMenu.asi");
        AppendInstallLog(gameRoot, "Installed plugins\\XMenu.asi");

        struct PayloadInstall {
            const char* fileName;
            bool enabled;
        };
        const PayloadInstall payloads[] = {
            {"XMenuSA.dll", options.installXMenuSA},
            {"XMenuVC.dll", options.installXMenuVC},
            {"XMenuIII.dll", options.installXMenuIII}
        };
        for (const PayloadInstall& payload : payloads) {
            if (!payload.enabled) {
                continue;
            }
            const std::string source = FindFirstFileByName(extractedRoot, payload.fileName);
            if (!source.empty()) {
                CopyFileEnsureDirectory(source, JoinPath(xmenuDir, payload.fileName), true);
                AddManifestRecord(manifestFiles, gameRoot, JoinPath("plugins\\XMenu", payload.fileName));
                AppendInstallLog(gameRoot, std::string("Installed plugins\\XMenu\\") + payload.fileName);
            } else {
                AppendInstallLog(gameRoot, std::string("Selected payload missing in release asset: ") + payload.fileName);
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
            AppendInstallLog(gameRoot, "Installed plugins\\XMenu\\data");
        }

        const std::vector<std::string> silentPatchFiles = CollectInstallableSilentPatchFiles(extractedRoot);
        if (silentPatchFiles.empty()) {
            AppendInstallLog(gameRoot, "SilentPatch files not found after scanning release asset");
        }
        for (const std::string& source : silentPatchFiles) {
            const std::string name = FileNameOf(source);
            const std::string target = JoinPath(pluginsDir, name);
            if (CopyFileEnsureDirectory(source, target, true)) {
                AddManifestRecord(manifestFiles, gameRoot, JoinPath("plugins", name));
                AppendInstallLog(gameRoot, "Installed plugins\\" + name + " from " + source);
            } else {
                AppendInstallLog(gameRoot, "ERROR failed to install SilentPatch file: " + source + " -> " + target);
            }
        }

        if (options.installRootDependencies) {
            const std::vector<std::string> rootDependencies = CollectInstallableRootDependencies(extractedRoot);
            if (rootDependencies.empty()) {
                AppendInstallLog(gameRoot, "Root dependency files not found after scanning release asset");
            }
            for (const std::string& source : rootDependencies) {
                const std::string dep = FileNameOf(source);
                const std::string target = JoinPath(gameRoot, dep);
                bool overwrite = true;
                if (PathExists(target)) {
                    overwrite = AskOverwriteDependency(dep, target) == IDYES;
                }
                if (overwrite && CopyFileEnsureDirectory(source, target, true)) {
                    AddManifestRecord(manifestFiles, gameRoot, dep);
                    AppendInstallLog(gameRoot, "Installed " + dep + " from " + source);
                } else if (!overwrite) {
                    AppendInstallLog(gameRoot, "Skipped existing root dependency " + dep);
                } else {
                    AppendInstallLog(gameRoot, "ERROR failed to install root dependency: " + source + " -> " + target);
                }
            }
        } else {
            AppendInstallLog(gameRoot, "Skipped root dependencies by user choice");
        }

        SyncConfigVersion(gameRoot, version);
        WriteManifest(gameRoot, version, manifestFiles);
        AppendInstallLog(gameRoot, "Manifest and config updated for version " + version);
        return true;
    }

    struct InstallerUiState {
        HINSTANCE instance = nullptr;
        HWND window = nullptr;
        HWND pathEdit = nullptr;
        HWND browseButton = nullptr;
        HWND gameTypeText = nullptr;
        HWND localVersionText = nullptr;
        HWND releaseVersionText = nullptr;
        HWND releaseNotesEdit = nullptr;
        HWND moduleIII = nullptr;
        HWND moduleVC = nullptr;
        HWND moduleSA = nullptr;
        HWND rootDependencies = nullptr;
        HWND downloadSource = nullptr;
        HWND installButton = nullptr;
        HWND statusText = nullptr;
        HWND progressBar = nullptr;
        HWND logBox = nullptr;
        HFONT titleFont = nullptr;
        HFONT normalFont = nullptr;
        HFONT smallFont = nullptr;
        HBRUSH windowBrush = nullptr;
        std::string gameRoot;
        GameType gameType = GameType::Unknown;
        ReleaseInfo currentRelease;
        bool isReleaseFetched = false;
    };

    constexpr int ControlBrowse = 1001;
    constexpr int ControlInstall = 1002;
    constexpr int ControlModuleIII = 1003;
    constexpr int ControlModuleVC = 1004;
    constexpr int ControlModuleSA = 1005;
    constexpr int ControlRootDependencies = 1006;
    constexpr int ControlDownloadSource = 1007;
    constexpr int ControlFetchRelease = 1008;

    InstallerUiState gUi;

    void SetStatus(const std::wstring& status) {
        if (gUi.statusText) {
            SetWindowTextW(gUi.statusText, status.c_str());
        }
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void AppendUiLog(const std::wstring& message) {
        if (!gUi.logBox) {
            return;
        }
        const int length = GetWindowTextLengthW(gUi.logBox);
        SendMessageW(gUi.logBox, EM_SETSEL, length, length);
        SendMessageW(gUi.logBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(message.c_str()));
        SendMessageW(gUi.logBox, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L"\r\n"));
    }

    void SetControlText(HWND hwnd, const std::wstring& text) {
        SetWindowTextW(hwnd, text.c_str());
    }

    void SetModuleControlsEnabled(bool enabled) {
        EnableWindow(gUi.moduleIII, enabled);
        EnableWindow(gUi.moduleVC, enabled);
        EnableWindow(gUi.moduleSA, enabled);
        EnableWindow(gUi.rootDependencies, enabled);
        EnableWindow(gUi.downloadSource, enabled);
        EnableWindow(gUi.installButton, enabled);
    }

    void FetchReleaseInfo() {
        EnableWindow(gUi.browseButton, FALSE);
        EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), FALSE);
        SetStatus(L"状态：请求 GitHub latest release...");
        AppendUiLog(L"开始请求 GitHub latest release...");

        const std::string apiJsonPath = TempPathFor("release.json");
        if (!DownloadFile(XMENU_GITHUB_API, apiJsonPath)) {
            SetStatus(L"状态：请求 GitHub API 失败");
            AppendUiLog(L"请求 GitHub API 失败。");
            MessageBoxW(gUi.window, L"请求 GitHub API 失败。", InstallerTitle, MB_ICONERROR);
            EnableWindow(gUi.browseButton, TRUE);
            EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
            return;
        }

        gUi.currentRelease = ParseReleaseInfo(ReadTextFile(apiJsonPath));
        if (gUi.currentRelease.tagName.empty() || gUi.currentRelease.assetUrl.empty()) {
            SetStatus(L"状态：未找到可安装的 release 包");
            AppendUiLog(L"GitHub release 中未找到可安装的 zip 包。");
            MessageBoxW(gUi.window, L"GitHub release 中未找到可安装的 zip 包。请上传 XMenuIII.VC.SA.zip 到 release。", InstallerTitle, MB_ICONERROR);
            EnableWindow(gUi.browseButton, TRUE);
            EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
            return;
        }

        gUi.isReleaseFetched = true;
        SetStatus(L"状态：已获取最新版本信息");
        AppendUiLog(L"获取到 GitHub 最新版本：" + WideFromUtf8(gUi.currentRelease.tagName));
        
        SetControlText(gUi.releaseVersionText, L"最新版本：" + WideFromUtf8(gUi.currentRelease.tagName));
        
        std::wstring releaseNotes = WideFromUtf8(gUi.currentRelease.body);
        if (releaseNotes.empty()) {
            releaseNotes = L"该版本没有提供更新日志。";
        }
        
        std::wstring formattedNotes;
        for (wchar_t c : releaseNotes) {
            if (c == L'\n') {
                formattedNotes += L"\r\n";
            } else if (c != L'\r') {
                formattedNotes += c;
            }
        }
        SetControlText(gUi.releaseNotesEdit, formattedNotes);

        if (!gUi.gameRoot.empty()) {
            SetModuleControlsEnabled(true);
        }
        
        EnableWindow(gUi.browseButton, TRUE);
        EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
    }

    int SelectedDownloadSourceIndex() {
        if (!gUi.downloadSource) {
            return DefaultDownloadSourceIndex;
        }
        const LRESULT selected = SendMessageW(gUi.downloadSource, CB_GETCURSEL, 0, 0);
        if (selected < 0 || selected >= static_cast<LRESULT>(sizeof(DownloadSources) / sizeof(DownloadSources[0]))) {
            return DefaultDownloadSourceIndex;
        }
        return static_cast<int>(selected);
    }

    const DownloadSource& SelectedDownloadSource() {
        return DownloadSources[SelectedDownloadSourceIndex()];
    }

    void ApplyFont(HWND hwnd, HFONT font) {
        if (hwnd && font) {
            SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }

    void ApplyNormalFont(HWND hwnd) {
        ApplyFont(hwnd, gUi.normalFont);
    }

    void ApplySmallFont(HWND hwnd) {
        ApplyFont(hwnd, gUi.smallFont);
    }

    void ApplyTitleFont(HWND hwnd) {
        ApplyFont(hwnd, gUi.titleFont);
    }

    HFONT CreateUiFont(int pointSize, int weight) {
        HDC dc = GetDC(nullptr);
        const int height = -MulDiv(pointSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
        ReleaseDC(nullptr, dc);
        return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    }

    void InitUiFonts() {
        gUi.titleFont = CreateUiFont(18, FW_SEMIBOLD);
        gUi.normalFont = CreateUiFont(9, FW_NORMAL);
        gUi.smallFont = CreateUiFont(8, FW_NORMAL);
        gUi.windowBrush = CreateSolidBrush(RGB(250, 250, 250));
    }

    void DestroyUiFonts() {
        if (gUi.titleFont) {
            DeleteObject(gUi.titleFont);
            gUi.titleFont = nullptr;
        }
        if (gUi.normalFont) {
            DeleteObject(gUi.normalFont);
            gUi.normalFont = nullptr;
        }
        if (gUi.smallFont) {
            DeleteObject(gUi.smallFont);
            gUi.smallFont = nullptr;
        }
        if (gUi.windowBrush) {
            DeleteObject(gUi.windowBrush);
            gUi.windowBrush = nullptr;
        }
    }

    void SetProgress(int value) {
        if (gUi.progressBar) {
            SendMessageW(gUi.progressBar, PBM_SETPOS, value, 0);
        }
    }

    void OnDownloadProgress(ULONG current, ULONG total) {
        if (total > 0) {
            const int percent = static_cast<int>((static_cast<unsigned long long>(current) * 100ull) / total);
            SetProgress(percent);
            std::wostringstream status;
            status << L"下载安装包：" << percent << L"%";
            SetStatus(status.str());
        } else {
            SetStatus(L"下载安装包：正在接收数据...");
        }
    }

    void SetCheckbox(HWND hwnd, bool checked) {
        SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    bool IsCheckboxChecked(HWND hwnd) {
        return SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    InstallOptions ReadOptionsFromUi() {
        InstallOptions options;
        options.installXMenuIII = IsCheckboxChecked(gUi.moduleIII);
        options.installXMenuVC = IsCheckboxChecked(gUi.moduleVC);
        options.installXMenuSA = IsCheckboxChecked(gUi.moduleSA);
        options.installRootDependencies = IsCheckboxChecked(gUi.rootDependencies);
        return options;
    }

    bool HasSelectedGameModule(const InstallOptions& options) {
        return options.installXMenuIII || options.installXMenuVC || options.installXMenuSA;
    }

    void ApplySelectedGameRoot(const std::string& gameRoot) {
        gUi.gameRoot = gameRoot;
        gUi.gameType = DetectGameType(gameRoot);

        SetControlText(gUi.pathEdit, WideFromAnsi(gameRoot));
        SetControlText(gUi.gameTypeText, std::wstring(L"检测到游戏：") + GameTypeName(gUi.gameType));

        const std::string installedVersion = ReadInstalledVersion(gameRoot);
        SetControlText(gUi.localVersionText, L"本地版本：" + WideFromUtf8(installedVersion.empty() ? "未安装" : installedVersion));

        const InstallOptions defaults = DefaultInstallOptionsForGame(gUi.gameType);
        SetCheckbox(gUi.moduleIII, defaults.installXMenuIII);
        SetCheckbox(gUi.moduleVC, defaults.installXMenuVC);
        SetCheckbox(gUi.moduleSA, defaults.installXMenuSA);
        SetCheckbox(gUi.rootDependencies, true);
        SetProgress(0);
        SetStatus(L"状态：等待安装/更新");
        if (gUi.isReleaseFetched) {
            SetModuleControlsEnabled(true);
        } else {
            AppendUiLog(L"请先获取最新版本信息。");
        }

        AppendUiLog(L"已选择游戏目录。确认安装模块后，点击“安装/更新”。");
    }

    bool ExecuteInstallFromUi() {
        if (gUi.gameRoot.empty()) {
            MessageBoxW(gUi.window, L"请先选择游戏目录。", InstallerTitle, MB_ICONWARNING);
            return false;
        }

        if (!gUi.isReleaseFetched) {
            MessageBoxW(gUi.window, L"请先获取版本信息。", InstallerTitle, MB_ICONWARNING);
            return false;
        }

        InstallOptions options = ReadOptionsFromUi();
        if (!HasSelectedGameModule(options)) {
            MessageBoxW(gUi.window, L"请至少选择一个 XMenu 游戏模块。", InstallerTitle, MB_ICONWARNING);
            return false;
        }

        EnableWindow(gUi.installButton, FALSE);
        EnableWindow(gUi.browseButton, FALSE);
        EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), FALSE);
        SetProgress(0);

        const DWORD startedTick = GetTickCount();
        const std::string startedAt = CurrentTimestamp();
        AppendInstallLog(gUi.gameRoot, "===== XMenu installer started at " + startedAt + " =====");
        AppendInstallLog(gUi.gameRoot, "Game root: " + gUi.gameRoot);
        AppendInstallLog(gUi.gameRoot, "Selected components:\r\n" + BuildSelectedComponentSummary(options));

        const std::string installedVersion = ReadInstalledVersion(gUi.gameRoot);
        std::string integrityReport;
        VerifyInstalledFiles(gUi.gameRoot, integrityReport);
        AppendInstallLog(gUi.gameRoot, "Installed version: " + (installedVersion.empty() ? std::string("none") : installedVersion));
        AppendInstallLog(gUi.gameRoot, "Integrity before install: " + integrityReport);

        const ReleaseInfo release = gUi.currentRelease;

        AppendUiLog(L"GitHub 版本：" + WideFromUtf8(release.tagName));
        AppendInstallLog(gUi.gameRoot, "Release version: " + release.tagName);
        AppendInstallLog(gUi.gameRoot, "Selected asset: " + release.assetName);
        AppendInstallLog(gUi.gameRoot, "Asset official URL: " + release.assetUrl);

        const int downloadSourceIndex = SelectedDownloadSourceIndex();
        const DownloadSource& downloadSource = SelectedDownloadSource();
        const std::string downloadUrl = BuildDownloadUrl(release.assetUrl, downloadSourceIndex);
        const std::string downloadSourceName = AnsiFromWide(downloadSource.label);

        std::wstring confirm;
        confirm += installedVersion.empty() ? L"将执行安装：\n\n" : L"将执行更新：\n\n";
        confirm += L"本地版本：" + WideFromUtf8(installedVersion.empty() ? "未安装" : installedVersion) + L"\n";
        confirm += L"GitHub 版本：" + WideFromUtf8(release.tagName) + L"\n";
        confirm += L"Release 包：" + WideFromUtf8(release.assetName) + L"\n";
        confirm += L"下载源：" + WideFromUtf8(downloadSourceName) + L"\n";
        confirm += L"完整性状态：" + WideFromUtf8(integrityReport) + L"\n\n";
        confirm += L"选择的组件：\n" + WideFromUtf8(BuildSelectedComponentSummary(options)) + L"\n继续安装？";
        if (MessageBoxW(gUi.window, confirm.c_str(), InstallerTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) != IDYES) {
            AppendUiLog(L"用户取消安装。");
            AppendInstallLog(gUi.gameRoot, "User cancelled install after release check");
            EnableWindow(gUi.installButton, TRUE);
            EnableWindow(gUi.browseButton, TRUE);
            EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
            return false;
        }

        const std::string zipPath = TempPathFor(release.assetName.empty() ? "release.zip" : release.assetName);
        SetProgress(0);
        SetStatus(L"状态：准备下载安装包...");
        AppendUiLog(L"开始下载安装包，下载源：" + WideFromUtf8(downloadSourceName));
        AppendInstallLog(gUi.gameRoot, "Download source: " + downloadSourceName);
        AppendInstallLog(gUi.gameRoot, "Download URL: " + downloadUrl);
        AppendInstallLog(gUi.gameRoot, "Downloading asset to: " + zipPath);
        try {
            DownloadFileOrThrow(downloadUrl, zipPath, downloadSourceName, OnDownloadProgress);
        } catch (const std::exception& error) {
            const std::string errorText = error.what();
            SetStatus(L"状态：下载安装包失败");
            AppendUiLog(L"下载安装包失败：" + WideFromUtf8(errorText));
            AppendInstallLog(gUi.gameRoot, "ERROR asset download failed: " + errorText);
            const std::wstring message = L"下载安装包失败。\n\n下载源：" + WideFromUtf8(downloadSourceName)
                + L"\n错误：" + WideFromUtf8(errorText)
                + L"\n\n可以切换到 gh-proxy / v4 / v6 / Fastly 后重试。";
            MessageBoxW(gUi.window, message.c_str(), InstallerTitle, MB_ICONERROR);
            EnableWindow(gUi.installButton, TRUE);
            EnableWindow(gUi.browseButton, TRUE);
            EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
            return false;
        }

        const std::string extractDir = TempPathFor("extract");
        RunHiddenAndWait("cmd.exe /c rmdir /s /q \"" + extractDir + "\"");
        SetProgress(100);
        SetStatus(L"状态：解压安装包...");
        AppendUiLog(L"开始解压安装包...");
        AppendInstallLog(gUi.gameRoot, "Extracting asset to: " + extractDir);
        if (!ExtractZip(zipPath, extractDir)) {
            AppendUiLog(L"解压安装包失败。");
            AppendInstallLog(gUi.gameRoot, "ERROR asset extraction failed");
            MessageBoxW(gUi.window, L"解压安装包失败。", InstallerTitle, MB_ICONERROR);
            EnableWindow(gUi.installButton, TRUE);
            EnableWindow(gUi.browseButton, TRUE);
            EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
            return false;
        }

        AppendUiLog(L"开始写入游戏目录...");
        SetStatus(L"状态：写入游戏目录...");
        const bool ok = InstallRelease(extractDir, gUi.gameRoot, release.tagName, options);
        const DWORD finishedTick = GetTickCount();
        const std::string finishedAt = CurrentTimestamp();
        const std::string duration = FormatDuration(startedTick, finishedTick);

        if (ok) {
            std::string finalReport;
            VerifyInstalledFiles(gUi.gameRoot, finalReport);
            AppendInstallLog(gUi.gameRoot, "Integrity after install: " + finalReport);
            AppendInstallLog(gUi.gameRoot, "Installer finished at " + finishedAt + ", duration " + duration);
            AppendUiLog(L"安装/更新完成。");
            SetControlText(gUi.localVersionText, L"本地版本：" + WideFromUtf8(release.tagName));

            const std::wstring message = L"XMenu 安装/更新完成。\n\n版本：" + WideFromUtf8(release.tagName)
                + L"\n包：" + WideFromUtf8(release.assetName)
                + L"\n开始时间：" + WideFromUtf8(startedAt)
                + L"\n结束时间：" + WideFromUtf8(finishedAt)
                + L"\n耗时：" + WideFromUtf8(duration)
                + L"\n" + WideFromUtf8(finalReport);
            MessageBoxW(gUi.window, message.c_str(), InstallerTitle, MB_ICONINFORMATION);
        } else {
            AppendInstallLog(gUi.gameRoot, "ERROR install failed at " + finishedAt + ", duration " + duration);
            AppendUiLog(L"安装失败。请查看 install.log。");
        }

        EnableWindow(gUi.installButton, TRUE);
        EnableWindow(gUi.browseButton, TRUE);
        EnableWindow(GetDlgItem(gUi.window, ControlFetchRelease), TRUE);
        return ok;
    }

    HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        return hwnd;
    }

    HWND CreateTitleLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyTitleFont(hwnd);
        return hwnd;
    }

    HWND CreateSmallLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplySmallFont(hwnd);
        return hwnd;
    }

    HWND CreateSectionTitle(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        return hwnd;
    }

    HWND CreateButton(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        return hwnd;
    }

    HWND CreateCheckbox(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        return hwnd;
    }

    HWND CreateReadonlyEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        return hwnd;
    }

    HWND CreateMultilineReadonlyEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL, x, y, w, h, parent, nullptr, gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        return hwnd;
    }

    HWND CreateDownloadSourceCombo(HWND parent, int x, int y, int w, int h) {
        HWND hwnd = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ControlDownloadSource)), gUi.instance, nullptr);
        ApplyNormalFont(hwnd);
        for (const DownloadSource& source : DownloadSources) {
            SendMessageW(hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(source.label));
        }
        SendMessageW(hwnd, CB_SETCURSEL, DefaultDownloadSourceIndex, 0);
        return hwnd;
    }

    void CreateInstallerControls(HWND window) {
        CreateTitleLabel(window, L"XMenu Installer", 24, 18, 260, 34);
        CreateSmallLabel(window, L"选择 GTA 游戏目录，确认模块，然后一键安装或更新 XMenu。", 26, 50, 560, 20);

        CreateSectionTitle(window, L"1. 游戏目录", 24, 86, 180, 24);
        CreateLabel(window, L"目录", 42, 118, 56, 24);
        gUi.pathEdit = CreateReadonlyEdit(window, L"尚未选择游戏目录", 98, 114, 440, 28);
        gUi.browseButton = CreateButton(window, L"选择目录", ControlBrowse, 550, 113, 88, 30);
        gUi.gameTypeText = CreateSmallLabel(window, L"检测到游戏：未选择", 98, 150, 250, 22);
        gUi.localVersionText = CreateSmallLabel(window, L"本地版本：未检测", 350, 150, 250, 22);

        CreateSectionTitle(window, L"2. 最新版本信息", 24, 186, 180, 24);
        gUi.releaseVersionText = CreateSmallLabel(window, L"最新版本：尚未获取", 42, 220, 250, 22);
        CreateButton(window, L"获取版本信息", ControlFetchRelease, 550, 214, 88, 30);
        CreateLabel(window, L"更新日志", 42, 250, 80, 24);
        gUi.releaseNotesEdit = CreateMultilineReadonlyEdit(window, L"请点击“获取版本信息”按钮以加载最新的发布日志和安装包信息。", 42, 274, 596, 120);

        CreateSectionTitle(window, L"3. 安装模块", 24, 410, 180, 24);
        gUi.moduleIII = CreateCheckbox(window, L"GTA III 模块", ControlModuleIII, 42, 440, 180, 24);
        gUi.moduleVC = CreateCheckbox(window, L"GTA Vice City 模块", ControlModuleVC, 42, 468, 180, 24);
        gUi.moduleSA = CreateCheckbox(window, L"GTA San Andreas 模块", ControlModuleSA, 42, 496, 210, 24);
        CreateSmallLabel(window, L"SilentPatch：固定随 XMenu 安装到 plugins", 330, 440, 280, 22);
        gUi.rootDependencies = CreateCheckbox(window, L"安装 Ultimate ASI Loader / D3D8to9 到游戏根目录", ControlRootDependencies, 330, 468, 300, 24);
        CreateSmallLabel(window, L"同名根目录依赖会在安装时再次询问覆盖或跳过。", 330, 498, 300, 22);
        CreateLabel(window, L"下载源", 42, 530, 56, 24);
        gUi.downloadSource = CreateDownloadSourceCombo(window, 98, 526, 280, 150);
        CreateSmallLabel(window, L"默认官方源；下载慢或失败时可切换代理。", 390, 530, 250, 22);

        CreateSectionTitle(window, L"4. 安装进度", 24, 576, 180, 24);
        gUi.statusText = CreateLabel(window, L"状态：等待选择游戏目录", 42, 608, 420, 24);
        gUi.progressBar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, 42, 638, 470, 18, window, nullptr, gUi.instance, nullptr);
        SendMessageW(gUi.progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(gUi.progressBar, PBM_SETPOS, 0, 0);
        gUi.installButton = CreateButton(window, L"安装 / 更新", ControlInstall, 528, 608, 108, 48);

        CreateSectionTitle(window, L"日志", 24, 680, 180, 24);
        gUi.logBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"安装器已启动。请先选择游戏目录并获取版本信息。\r\n", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 42, 710, 594, 70, window, nullptr, gUi.instance, nullptr);
        ApplySmallFont(gUi.logBox);
        SetModuleControlsEnabled(false);
    }

    LRESULT CALLBACK InstallerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_CREATE:
            gUi.window = window;
            InitUiFonts();
            CreateInstallerControls(window);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case ControlBrowse: {
                const std::string gameRoot = PickGameRoot();
                if (!gameRoot.empty()) {
                    ApplySelectedGameRoot(gameRoot);
                }
                return 0;
            }
            case ControlFetchRelease:
                FetchReleaseInfo();
                return 0;
            case ControlInstall:
                ExecuteInstallFromUi();
                return 0;
            default:
                return 0;
            }
        case WM_CTLCOLORSTATIC: {
            const HWND control = reinterpret_cast<HWND>(lParam);
            if (control == gUi.pathEdit || control == gUi.logBox || control == gUi.releaseNotesEdit) {
                SetBkMode(reinterpret_cast<HDC>(wParam), OPAQUE);
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
            }
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            return reinterpret_cast<LRESULT>(gUi.windowBrush ? gUi.windowBrush : GetStockObject(WHITE_BRUSH));
        }
        case WM_CTLCOLORBTN: {
            const HWND control = reinterpret_cast<HWND>(lParam);
            if (control == gUi.moduleIII || control == gUi.moduleVC || control == gUi.moduleSA || control == gUi.rootDependencies) {
                SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
                return reinterpret_cast<LRESULT>(gUi.windowBrush ? gUi.windowBrush : GetStockObject(WHITE_BRUSH));
            }
            return DefWindowProcW(window, message, wParam, lParam);
        }
        case WM_ERASEBKGND: {
            RECT rect{};
            GetClientRect(window, &rect);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, gUi.windowBrush ? gUi.windowBrush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            return 1;
        }
        case WM_DESTROY:
            DestroyUiFonts();
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    int RunInstallerUi(HINSTANCE instance, int showCommand) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        gUi.instance = instance;

        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = InstallerWindowProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = L"XMenuInstallerWindow";
        windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        windowClass.hbrBackground = gUi.windowBrush ? gUi.windowBrush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        RegisterClassW(&windowClass);

        HWND window = CreateWindowExW(0, windowClass.lpszClassName, InstallerTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 740, 840, nullptr, nullptr, instance, nullptr);
        if (!window) {
            CoUninitialize();
            return 1;
        }

        ShowWindow(window, showCommand);
        UpdateWindow(window);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        CoUninitialize();
        return static_cast<int>(msg.wParam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    return RunInstallerUi(instance, showCommand);
}
