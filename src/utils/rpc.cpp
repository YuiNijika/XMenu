#include "rpc.h"
#include "defines.h"
#include "utils/Log.h"
#include "utils/I18n.h"
#include "ui/Menu.h"
#include "controllers/Vehicle.h"
#include <format>
#include <algorithm>
#include <filesystem>
#include "CPlayerPed.h"
#include "CPlayerInfo.h"
#include "CPools.h"
#include "CWorld.h"
#include "extensions/ScriptCommands.h"
#include "plugin.h"
#include "CTimer.h"
#include <ctime>

namespace {
    bool bInit = false;
    void* hDll = nullptr;
    void* f_Update = nullptr;
}

// discord server ids
const char* id = BY_GAME("951199292981403669", "951448264195059712", "951457540573655080");

static char asciitolower(char in) 
{
    if (in <= 'Z' && in >= 'A')
    {
        return in - ('Z' - 'z');
    }
    
    return in;
}

void RPC::Shutdown()
{
    if (hDll)
    {
        void* f_Shutdown = GetProcAddress((HMODULE)hDll, "Discord_Shutdown");
        if (f_Shutdown)
        {
            typedef void (*Discord_Shutdown_t)();
            ((Discord_Shutdown_t)f_Shutdown)();
        }
        FreeLibrary((HMODULE)hDll);
        hDll = nullptr;
    }
}

void RPC::Init()
{
    const char* dllPath = PLUGIN_PATH("XMenu/dlls/discord-rpc.dll");

    // check if the dll exits
    if (!std::filesystem::exists(dllPath))
    {
        Log::Error("Discord RPC DLL not found");
        return;
    }

    if (!hDll)
    {
        hDll = LoadLibrary(dllPath);
    }

    if (hDll)
    {
        void* f_Init = (void*)GetProcAddress((HMODULE)hDll, "Discord_Initialize");
        f_Update = (void*)GetProcAddress((HMODULE)hDll, "Discord_UpdatePresence");

        if (f_Init && f_Update)
        {
            typedef void (*Discord_Initialize_t)(const char*, void*, int, const char*);
            ((Discord_Initialize_t)f_Init)(id, nullptr, 1, nullptr);
            drpc.startTimestamp = time(0);
            bInit = true;
        }
        else
        {
            Log::Error("Discord RPC Init failed");
        }
    }
}

void RPC::Process()
{
    if (!bInit)
    {
        return;
    }

    static std::string detailsText, stateText, smallImg, smallImgText, largeImg, largeImgText;
    static size_t curImage = 1;
    static size_t timer = CTimer::m_snTimeInMilliseconds;
    
    CPlayerInfo *pInfo = &CWorld::Players[CWorld::PlayerInFocus];
    CPlayerPed *pPed = pInfo->m_pPed;

    if (pPed)
    {
        size_t curTimer = CTimer::m_snTimeInMilliseconds;

#ifdef GTASA
        size_t wantedLevel = pPed->GetWantedLevel();
#else
        size_t wantedLevel = pPed->m_pWanted->m_nWantedLevel;
#endif

        if (wantedLevel > 0)
        {
            detailsText = std::format("Wanted Level: ");
            for (size_t i = 0; i < wantedLevel; ++i)
            {
                detailsText += "*";
            }

            if (wantedLevel > 3)
            {
                stateText = "On Rampage";
            }   
        }
        else
        {
            detailsText = std::format("Money: ${}", pInfo->m_nMoney);
        }

#ifndef GTA3
        if (pPed->m_nAreaCode != 0) // world
        {
            stateText = TEXT("RPC.InsideInterior");
        }
#endif
        
        if (BY_GAME(pPed->bInVehicle, pPed->m_bInVehicle, pPed->m_bInVehicle))
        {
            smallImgText = "Driving";
            smallImg = "drive";
        }
        else
        {
            smallImg = "walk";
            smallImgText = "Walking";
        }

        stateText = "Browsing XMenu";

        if (plugin::Command<plugin::Commands::IS_CHAR_DEAD>(CPools::GetPedRef(pPed)))
        {
            stateText = "Wasted";
        }
        
        if (BY_GAME(pPed->bIsBeingArrested, false, false))
        {
            stateText = "Busted";
        }

        largeImgText = "Armour: " + std::to_string((int)pPed->m_fArmour) + " - Health: " + std::to_string((int)pPed->m_fHealth);
        largeImg = "sa" + std::to_string(curImage);
        
        drpc.details = detailsText.c_str();
        drpc.state = stateText.c_str();
        drpc.largeImageKey = largeImg.c_str();
        drpc.largeImageText = largeImgText.c_str();
        drpc.smallImageKey = smallImg.c_str();
        drpc.smallImageText = smallImgText.c_str();
        drpc.largeImageKey = largeImg.c_str();
        drpc.largeImageText = largeImgText.c_str();

        if (f_Update)
        {
            typedef void (*Discord_UpdatePresence_t)(void*);
            ((Discord_UpdatePresence_t)f_Update)(&drpc);
        }

        if (curTimer - timer > 5*60000)
        {
            curImage++;

            if (curImage > 5) // Must upload images to discord servers
            {
                curImage = 1;
            }
            timer = curTimer;
        }
    }
}