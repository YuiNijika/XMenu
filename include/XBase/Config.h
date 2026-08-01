#pragma once

#include <string>

namespace XBase::Config {

void Init(const std::string& filePath = "");
void Save();
const std::string& GetFilePath();

std::string GetString(const std::string& key, const std::string& def = "");
int GetInt(const std::string& key, int def = 0);
float GetFloat(const std::string& key, float def = 0.0f);
bool GetBool(const std::string& key, bool def = false);

void SetString(const std::string& key, const std::string& value);
void SetInt(const std::string& key, int value);
void SetFloat(const std::string& key, float value);
void SetBool(const std::string& key, bool value);

bool HasKey(const std::string& key);

} // namespace XBase::Config