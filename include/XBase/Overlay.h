#pragma once

namespace XBase::Overlay {

void Init();
void Process();
void Shutdown();
void Draw();
void SetVisible(bool enable);
bool IsVisible();
void Toggle();
void SetPosition(bool topLeft, bool topRight, bool bottomLeft, bool bottomRight);

} // namespace XBase::Overlay
