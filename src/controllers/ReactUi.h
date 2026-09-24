#pragma once

#include <XBase/UI.h>

#include <string>

// React 界面，网页面板承载前端页面，通过网页桥接调用 XBase
namespace Controllers::ReactUi {

void Install();
void Process();

bool IsActive();
bool Enable(bool enable);

// 启用前先跑这一套检查，包括系统版本、网页视图能力、运行时和页面文件
bool IsAvailable(std::string& reason);

// 面板在屏幕上的位置与大小，网页端拖动与缩放时用
XBase::Rect PanelBounds();

// 存档不能从界面线程直接触发，挂到游戏线程那一帧去执行
void QueueSaveGame();

} // namespace Controllers::ReactUi
