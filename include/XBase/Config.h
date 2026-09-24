#pragma once

#include <string>

namespace XBase::Config {

void Init(const std::string& filePath = "");

// 模组配置默认写到 XBase 目录下以模组名命名的子目录里的配置文件
void InitForMod(const char* modName);
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