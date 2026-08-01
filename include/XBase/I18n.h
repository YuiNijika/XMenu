#pragma once

#include <string>
#include <vector>

namespace XBase::I18n {

struct LanguageInfo {
    std::string code;
    std::string name;
};

void Init(const std::string& directory = "");
void SetLanguage(const std::string& code);
std::string GetCurrentLanguage();
std::vector<LanguageInfo> GetAvailableLanguages();

const char* T(const char* key);
const char* Translate(const char* key);

} // namespace XBase::I18n