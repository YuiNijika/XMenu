#pragma once
#include <cstring>
#include <string>
#include <vector>

namespace I18n {
    enum class Language {
        Zh,
        En,
        Jp,
        Ru
    };

    struct LanguageInfo {
        std::string code;
        std::string name;
        std::string fallback;
    };

    void Init();
    void SetLanguage(Language language);
    void SetLanguage(const std::string& code);
    void SetFallbackLanguage(const std::string& code);
    std::string GetFallbackLanguageCode();
    Language GetLanguage();
    std::string GetCurrentLanguageCode();
    const std::vector<LanguageInfo>& GetAvailableLanguages();
    const char* GetLanguageCode(Language language);
    const char* GetLanguageName(Language language);
    const char* GetLanguageName(const std::string& code);
    const char* T(const char* key);
    const char* T(Language language, const char* key);
    const char* T(const std::string& languageCode, const char* key);
}