#pragma once

namespace XBase::Visual {

bool DisplayHud(bool enable);
bool DisplayRadar(bool enable);
bool SetFilter(int id, float strength);
void NotifyGameInit();
void Shutdown();
void Process();

} // namespace XBase::Visual
