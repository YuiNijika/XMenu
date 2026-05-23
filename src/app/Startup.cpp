#include "Startup.h"
#include "defines.h"
#include "plugin.h"
#include "GameVersion.h"
#include <windows.h>

namespace {
    bool IsSupportedGameVersion() {
        const unsigned int gameVersion = plugin::GetGameVersion();

#ifdef GTASA
        return gameVersion == GAME_10US_HOODLUM || gameVersion == GAME_10US_COMPACT;
#else
        return gameVersion == BY_GAME(0, GAME_10EN, GAME_10EN);
#endif
    }

    bool IsUnsupportedOnlineRuntimeLoaded() {
#ifdef GTASA
        return GetModuleHandleA("SAMP.dll") || GetModuleHandleA("SAMP.asi");
#elif GTAVC
        return GetModuleHandleA("vcmp-proxy.dll") || GetModuleHandleA("vcmp-proxy.asi");
#else
        return false;
#endif
    }
}

namespace Startup {
    bool Validate() {
        if (!IsSupportedGameVersion()) {
            MessageBoxA(
                HWND_DESKTOP,
                "Unknown game version. GTA " BY_GAME("SA v1.0 US Hoodlum or Compact", "VC v1.0 EN", "III v1.0 EN") " is required.",
                "XMenu",
                MB_ICONERROR
            );
            return false;
        }

        if (IsUnsupportedOnlineRuntimeLoaded()) {
            MessageBoxA(HWND_DESKTOP, "Online multiplayer runtime detected. XMenu is disabled for this session.", "XMenu", MB_ICONERROR);
            return false;
        }

        return true;
    }
}