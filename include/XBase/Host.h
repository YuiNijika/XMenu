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

// Shows a message through the game help text. Only safe from game logic
// callbacks. From UI or render code use QueueMessage, which defers the call to
// the next script event and keeps the render callback free of game state access.
bool ShowMessage(const char* message);
bool QueueMessage(const char* message);

} // namespace XBase::Host