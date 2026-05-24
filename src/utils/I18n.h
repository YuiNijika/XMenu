#pragma once

namespace I18n {
    enum class Language {
        Zh,
        En,
        Jp,
        Ru
    };

    void Init();
    void SetLanguage(Language language);
    Language GetLanguage();
    const char* GetLanguageCode(Language language);
    const char* GetLanguageName(Language language);
    const char* T(const char* key);
    const char* T(Language language, const char* key);
}