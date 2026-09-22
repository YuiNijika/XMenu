#pragma once

#include <functional>
#include <string>

#include "Json.h"

namespace XBase::WebBridge {

// 宿主可以注册自己的方法，网页侧同样用 window.xbase.call 调用
using MethodHandler = std::function<Json::Value(const Json::Value& params)>;
bool RegisterMethod(const std::string& method, MethodHandler handler);
void UnregisterMethod(const std::string& method);

// 网页通过 window.xbase.call 调用 XBase 公共 API，能力不足的方法返回错误
void Install();
void Shutdown();
bool IsInstalled();

// 原生向网页推事件，网页用 window.xbase.on 订阅
bool Emit(const std::string& event, const Json::Value& payload);

} // namespace XBase::WebBridge
