#pragma once

namespace XBase::Host {

using Callback = void (*)();

struct Callbacks {
    Callback onGameInit = nullptr;
    Callback onProcess = nullptr;
};

bool Install(const Callbacks& callbacks);
void Shutdown();
bool IsInstalled();
bool ShowMessage(const char* message);

} // namespace XBase::Host