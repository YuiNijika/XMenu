#include "cutscene_sa.h"
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "CCutsceneMgr.h"
#include "CPlayerPed.h"
#include "CPools.h"
#include "CCamera.h"
#include "extensions/ScriptCommands.h"
#include "plugin.h"

CutsceneMgr& Cutscene = CutsceneMgr::Get();
CutsceneMgr::CutsceneMgr()
{
    // Need a way to catch when cutscene skips/ends, doing this via the CdeclEvent for now,
    // though in XMenu it might be better handled explicitly in a Process loop if possible.
    static plugin::CdeclEvent <plugin::AddressList<0x5B195F, plugin::H_JUMP>, plugin::PRIORITY_AFTER, plugin::ArgPickNone, void()> skipCutsceneEvent;
    skipCutsceneEvent += [this]()
    {
        Stop();
    };
}

void CutsceneMgr::Play(const std::string& cutsceneId, const std::string& interior)
{
    if (CCutsceneMgr::ms_running)
    {
        MenuState::ShowNotice("Cutscene is already running", 2.0);
        return;
    }

    CPlayerPed* pPlayer = FindPlayerPed();
    if (pPlayer)
    {
        m_pLastVeh =  pPlayer->bInVehicle ? pPlayer->m_pVehicle : nullptr;
        m_nVehSeat = -1;

        if (m_pLastVeh && m_pLastVeh->m_pDriver != pPlayer)
        {
            for (size_t i = 0; i != 8; ++i)
            {
                if (m_pLastVeh->m_apPassengers[i] == pPlayer)
                {
                    m_nVehSeat = i;
                    break;
                }
            }
        }
        CCutsceneMgr::LoadCutsceneData(cutsceneId.c_str());
        CCutsceneMgr::Update();

        m_nInterior = pPlayer->m_nAreaCode;
        pPlayer->m_nAreaCode = std::stoi(interior);
        plugin::Command<plugin::Commands::SET_AREA_VISIBLE>(pPlayer->m_nAreaCode);
        CutsceneMgr::m_bRunning = true;
        CCutsceneMgr::StartCutscene();
        MenuState::ShowNotice("Cutscene started", 2.0);
    }
}

void CutsceneMgr::Stop()
{
    if (CutsceneMgr::m_bRunning)
    {
        CPlayerPed *pPlayer = FindPlayerPed();
        int hPlayer = CPools::GetPedRef(pPlayer);

        CCutsceneMgr::DeleteCutsceneData();
        CutsceneMgr::m_bRunning = false;
        pPlayer->m_nAreaCode = CutsceneMgr::m_nInterior;
        CutsceneMgr::m_nInterior = 0;
        plugin::Command<plugin::Commands::SET_AREA_VISIBLE>(pPlayer->m_nAreaCode);

        // handle vehicle
        if (CutsceneMgr::m_pLastVeh)
        {
            int hVeh = CPools::GetVehicleRef(CutsceneMgr::m_pLastVeh);
            if (CutsceneMgr::m_nVehSeat == -1)
            {
                plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR>(hPlayer, hVeh);
            }
            else
            {
                plugin::Command<plugin::Commands::WARP_CHAR_INTO_CAR_AS_PASSENGER>(hPlayer, hVeh, CutsceneMgr::m_nVehSeat);
            }
        }
        TheCamera.Fade(0, 1);
    }
}