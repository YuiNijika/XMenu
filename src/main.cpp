#include <windows.h>
#include "plugin.h"
#include "app/Startup.h"
#include "utils/D3DHook.h"
#include "ui/Menu.h"
#include "features/GameLogic.h"
#include "CPad.h"

namespace {
    void InitXMenu() {
        plugin::Events::initGameEvent.after += []() {
            GameLogic::Init();
            D3DHook::Init([]() {
                Menu::Draw();
            });
        };

        plugin::Events::processScriptsEvent += []() {
            GameLogic::Process();
            Menu::Process();

            static bool lastKeyState = false;
            const bool currentKeyState = (GetKeyState('M') & 0x8000) != 0;

            if (currentKeyState && !lastKeyState) {
                D3DHook::SetMenuVisible(!D3DHook::IsMenuVisible());
            }
            lastKeyState = currentKeyState;

            CPad* pad = CPad::GetPad(0);
            if (pad) {
                pad->DisablePlayerControls = D3DHook::IsMenuVisible();
            }
        };
    }
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved) {
    if (nReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hDllHandle);
        if (Startup::Validate()) {
            InitXMenu();
        }
    } else if (nReason == DLL_PROCESS_DETACH) {
        D3DHook::Shutdown();
    }
    return TRUE;
}