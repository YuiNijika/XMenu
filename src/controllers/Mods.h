#pragma once

#include <XBase/Json.h>

#include <string>

namespace Controllers::Mods {
const char* GameSlug();
void Request(int page = 1, int limit = 12, const std::string& type = "");
XBase::Json::Value Snapshot();
bool Open(const std::string& id);
bool OpenGame();
void Shutdown();
}
