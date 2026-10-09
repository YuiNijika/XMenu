#pragma once

#include <XBase/Json.h>

namespace Controllers::Targeting {

XBase::Json::Value ConfigPayload();
XBase::Json::Value ActionsPayload();
void ApplyConfig(const XBase::Json::Value& params);
void UpdateLabels();

}
