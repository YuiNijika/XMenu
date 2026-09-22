#pragma once

// React 界面：网页面板承载前端页面，通过 WebBridge 调用 XBase
namespace Controllers::ReactUi {

void Install();
void Process();

bool IsActive();
bool Enable(bool enable);

} // namespace Controllers::ReactUi
