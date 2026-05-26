#include "I18n.h"
#include "utils/JsonLoader.h"
#include "utils/Log.h"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace {
    using Dictionary = std::unordered_map<std::string, std::string>;

    std::string currentLanguageCode = "zh";
    std::string fallbackLanguageCode = "zh";
    std::unordered_map<std::string, Dictionary> dictionaries;
    std::unordered_map<std::string, std::string> fallbacks;
    std::vector<I18n::LanguageInfo> availableLanguages;

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

    void ParseFlatJson(const std::string& content, Dictionary& dictionary) {
        std::size_t cursor = 0;
        while (cursor < content.size()) {
            const std::size_t keyStartQuote = content.find('"', cursor);
            if (keyStartQuote == std::string::npos) break;

            const std::size_t keyEndQuote = content.find('"', keyStartQuote + 1);
            if (keyEndQuote == std::string::npos) break;

            const std::size_t colon = content.find(':', keyEndQuote + 1);
            if (colon == std::string::npos) break;

            const std::size_t valueStartQuote = content.find('"', colon + 1);
            if (valueStartQuote == std::string::npos) break;

            std::size_t valueEndQuote = valueStartQuote + 1;
            bool escaped = false;
            while (valueEndQuote < content.size()) {
                const char current = content[valueEndQuote];
                if (current == '"' && !escaped) break;
                escaped = current == '\\' && !escaped;
                if (current != '\\') escaped = false;
                ++valueEndQuote;
            }

            if (valueEndQuote >= content.size()) break;

            const std::string key = DecodeJsonString(content.substr(keyStartQuote + 1, keyEndQuote - keyStartQuote - 1));
            const std::string value = DecodeJsonString(content.substr(valueStartQuote + 1, valueEndQuote - valueStartQuote - 1));
            dictionary[key] = value;
            cursor = valueEndQuote + 1;
        }
    }

    bool FileExists(const std::string& path) {
        const DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    bool DirectoryExists(const std::string& path) {
        const DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    std::string DirectoryFromModule(HMODULE module) {
        char path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameA(module, path, MAX_PATH);
        if (size == 0) return "";

        std::string directory(path, size);
        const std::size_t slash = directory.find_last_of("\\/");
        if (slash == std::string::npos) return "";
        return directory.substr(0, slash + 1);
    }

    std::string ModuleDirectory() {
        HMODULE module = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&ModuleDirectory),
            &module
        );
        return DirectoryFromModule(module);
    }

    std::string AsiDirectory() {
        const std::string asiDirectory = DirectoryFromModule(GetModuleHandleA("XMenu.asi"));
        return asiDirectory.empty() ? ModuleDirectory() : asiDirectory;
    }

    std::string NormalizeDirectory(std::string path) {
        if (!path.empty() && path.back() != '\\' && path.back() != '/') {
            path += "\\";
        }
        return path;
    }

    bool LoadDictionaryFile(const std::string& languageCode, const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;

        const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        Dictionary& dictionary = dictionaries[languageCode];
        const std::size_t before = dictionary.size();
        ParseFlatJson(content, dictionary);
        return dictionary.size() > before;
    }

    I18n::LanguageInfo* FindLanguageInfo(const std::string& code) {
        for (I18n::LanguageInfo& language : availableLanguages) {
            if (language.code == code) return &language;
        }
        return nullptr;
    }

    void UpsertLanguageInfo(const std::string& code, const std::string& name, const std::string& fallback) {
        if (code.empty()) return;

        I18n::LanguageInfo* existing = FindLanguageInfo(code);
        if (existing) {
            if (!name.empty()) existing->name = name;
            if (!fallback.empty()) existing->fallback = fallback;
        } else {
            I18n::LanguageInfo language;
            language.code = code;
            language.name = name.empty() ? code : name;
            language.fallback = fallback;
            availableLanguages.push_back(language);
        }

        if (!fallback.empty()) {
            fallbacks[code] = fallback;
        }
    }

    bool LoadLanguageIndex(const std::string& languageDir, const std::string& directoryCode) {
        const std::string indexPath = languageDir + "index.json";
        if (!FileExists(indexPath)) return false;

        const JsonLoader::JsonValue index = JsonLoader::LoadFromFile(indexPath);
        if (index.type != JsonLoader::JsonValue::OBJECT) return false;

        const std::string code = JsonLoader::GetString(index, "code", directoryCode);
        const std::string name = JsonLoader::GetString(index, "name", code);
        const std::string fallback = JsonLoader::GetString(index, "fallback", code == "zh" ? "" : "zh");
        UpsertLanguageInfo(code, name, fallback);

        bool loaded = false;
        const std::vector<JsonLoader::JsonValue>& files = JsonLoader::GetArray(index, "files");
        for (const JsonLoader::JsonValue& file : files) {
            if (file.type != JsonLoader::JsonValue::STRING || file.string_value.empty()) continue;
            loaded = LoadDictionaryFile(code, languageDir + file.string_value) || loaded;
        }

        if (loaded) {
            Log::Info(std::string("语言包加载完成: ") + indexPath + "，词条数=" + std::to_string(dictionaries[code].size()));
        }
        return loaded;
    }

    bool LoadLegacyLanguage(const std::string& baseDir, const std::string& code, const std::string& name) {
        if (!LoadDictionaryFile(code, baseDir + code + ".json")) return false;
        UpsertLanguageInfo(code, name, code == "zh" ? "" : "zh");
        return true;
    }

    bool ScanLanguageBase(const std::string& rawBaseDir) {
        const std::string baseDir = NormalizeDirectory(rawBaseDir);
        if (!DirectoryExists(baseDir)) return false;

        bool loaded = false;
        WIN32_FIND_DATAA data = {};
        HANDLE find = FindFirstFileA((baseDir + "*").c_str(), &data);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                const std::string name = data.cFileName;
                if (name == "." || name == "..") continue;
                if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) continue;
                loaded = LoadLanguageIndex(baseDir + name + "\\", name) || loaded;
            } while (FindNextFileA(find, &data));
            FindClose(find);
        }

        loaded = LoadLegacyLanguage(baseDir, "zh", (const char*)u8"简体中文") || loaded;
        loaded = LoadLegacyLanguage(baseDir, "en", "English") || loaded;
        loaded = LoadLegacyLanguage(baseDir, "jp", (const char*)u8"日本語") || loaded;
        loaded = LoadLegacyLanguage(baseDir, "ru", (const char*)u8"Русский") || loaded;
        return loaded;
    }

    void LoadFallbackZh() {
        Dictionary& zh = dictionaries["zh"];
        if (!zh.empty()) return;

        zh["window.title"] = (const char*)u8"XMenu 作者：%s - GTAMODX";
        zh["tab.player"] = (const char*)u8"玩家";
        zh["tab.vehicle"] = (const char*)u8"载具";
        zh["tab.teleport"] = (const char*)u8"传送";
        zh["tab.weapon"] = (const char*)u8"武器";
        zh["tab.world"] = (const char*)u8"世界";
        zh["tab.ped"] = (const char*)u8"行人";
        zh["tab.scene"] = (const char*)u8"场景";
        zh["tab.visual"] = (const char*)u8"视觉";
        zh["tab.settings"] = (const char*)u8"设置";
        zh["tab.about"] = (const char*)u8"关于";
        zh["settings.interfaceLanguage"] = (const char*)u8"界面语言";
        zh["settings.applyImmediately"] = (const char*)u8"切换后立即生效";
        zh["status.language"] = (const char*)u8"语言";
        UpsertLanguageInfo("zh", (const char*)u8"简体中文", "");
    }

    const std::string& CodeForLanguage(I18n::Language language) {
        static const std::string zh = "zh";
        static const std::string en = "en";
        static const std::string jp = "jp";
        static const std::string ru = "ru";

        switch (language) {
        case I18n::Language::En: return en;
        case I18n::Language::Jp: return jp;
        case I18n::Language::Ru: return ru;
        case I18n::Language::Zh:
        default: return zh;
        }
    }

    const char* Lookup(const std::string& languageCode, const char* key, int depth = 0) {
        if (depth > 4) return nullptr;

        const auto dictionary = dictionaries.find(languageCode);
        if (dictionary != dictionaries.end()) {
            const auto translated = dictionary->second.find(key);
            if (translated != dictionary->second.end()) return translated->second.c_str();
        }

        if (!fallbackLanguageCode.empty() && fallbackLanguageCode != languageCode) {
            if (const char* translated = Lookup(fallbackLanguageCode, key, depth + 1)) return translated;
        }

        const auto fallback = fallbacks.find(languageCode);
        if (fallback != fallbacks.end() && !fallback->second.empty() && fallback->second != languageCode) {
            if (const char* translated = Lookup(fallback->second, key, depth + 1)) return translated;
        }

        if (languageCode != "zh") {
            if (const char* translated = Lookup("zh", key, depth + 1)) return translated;
        }

        return nullptr;
    }
}

namespace I18n {
    void Init() {
        Log::Info("开始初始化 i18n 系统...");

        dictionaries.clear();
        fallbacks.clear();
        availableLanguages.clear();

        const std::string baseDir = AsiDirectory();
        bool loadedFromPackagedData = false;
        if (!baseDir.empty()) {
            loadedFromPackagedData = ScanLanguageBase(baseDir + "XMenu\\data\\i18n\\");
            if (!loadedFromPackagedData) {
                ScanLanguageBase(baseDir + "XMenu\\i18n\\");
                ScanLanguageBase(baseDir + "i18n\\");
            }
        }
        if (!loadedFromPackagedData) {
            if (!ScanLanguageBase("XMenu\\data\\i18n\\")) {
                ScanLanguageBase("XMenu\\i18n\\");
                ScanLanguageBase("src\\data\\i18n\\");
            }
        }

        LoadFallbackZh();
        if (!FindLanguageInfo(currentLanguageCode)) {
            currentLanguageCode = "zh";
        }

        Log::Info(std::string("i18n 初始化完成，语言数=") + std::to_string(availableLanguages.size()) + "，当前语言=" + currentLanguageCode);
    }

    void SetLanguage(Language language) {
        SetLanguage(CodeForLanguage(language));
    }

    void SetLanguage(const std::string& code) {
        if (!FindLanguageInfo(code)) {
            Log::Warn(std::string("语言不存在: ") + code);
            return;
        }
        currentLanguageCode = code;
        Log::Info(std::string("语言已切换为: ") + GetLanguageName(code));
    }

    void SetFallbackLanguage(const std::string& code) {
        if (!FindLanguageInfo(code)) {
            Log::Warn(std::string("回退语言不存在: ") + code);
            return;
        }
        fallbackLanguageCode = code;
        Log::Info(std::string("缺失翻译回退语言已切换为: ") + GetLanguageName(code));
    }

    std::string GetFallbackLanguageCode() {
        return fallbackLanguageCode;
    }

    Language GetLanguage() {
        if (currentLanguageCode == "en") return Language::En;
        if (currentLanguageCode == "jp") return Language::Jp;
        if (currentLanguageCode == "ru") return Language::Ru;
        return Language::Zh;
    }

    std::string GetCurrentLanguageCode() {
        return currentLanguageCode;
    }

    const std::vector<LanguageInfo>& GetAvailableLanguages() {
        return availableLanguages;
    }

    const char* GetLanguageCode(Language language) {
        return CodeForLanguage(language).c_str();
    }

    const char* GetLanguageName(Language language) {
        return GetLanguageName(CodeForLanguage(language));
    }

    const char* GetLanguageName(const std::string& code) {
        const LanguageInfo* language = nullptr;
        for (const LanguageInfo& item : availableLanguages) {
            if (item.code == code) {
                language = &item;
                break;
            }
        }
        return language ? language->name.c_str() : code.c_str();
    }

    const char* T(Language language, const char* key) {
        return T(CodeForLanguage(language), key);
    }

    const char* T(const std::string& languageCode, const char* key) {
        if (const char* translated = Lookup(languageCode, key)) return translated;
        return key;
    }

    const char* T(const char* key) {
        if (const char* translated = Lookup(currentLanguageCode, key)) return translated;

        static std::unordered_map<std::string, bool> loggedKeys;
        static std::size_t loggedCount = 0;
        constexpr std::size_t maxLoggedMissingKeys = 32;
        if (loggedKeys.find(key) == loggedKeys.end()) {
            loggedKeys[key] = true;
            if (loggedCount < maxLoggedMissingKeys) {
                ++loggedCount;
                Log::Warn(std::string("未找到翻译键: ") + key + "，当前语言: " + currentLanguageCode);
                if (loggedCount == maxLoggedMissingKeys) {
                    Log::Warn("未找到翻译键日志达到上限，后续缺失项将静默回退为 key");
                }
            }
        }
        return key;
    }
}