#include "freecam_sa.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "plugin.h"
#include "extensions/ScriptCommands.h"
#include "CPools.h"
#include "CPlayerPed.h"
#include "CHud.h"
#include "utils/I18n.h"
#include "CCamera.h"
#include "CTimer.h"
#include "CPad.h"
#include "CWorld.h"

static CVector gTotalMouse;
FreecamMgr& Freecam = FreecamMgr::Get();

void FreecamMgr::Enable()
{
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;

    plugin::Command<plugin::Commands::SET_EVERYONE_IGNORE_PLAYER>(0, true);

    // set hud & radar states
    m_bHudState = plugin::patch::Get<BYTE>(0xBA6769); // hud
    m_bRadarState = plugin::patch::Get<BYTE>(0xBA676C); // radar
    plugin::patch::Set<BYTE>(0xBA6769, 0); // disable hud
    plugin::patch::Set<BYTE>(0xBA676C, 2); // disable radar

    // create our dummy character
    CVector pos = player->GetPosition();
    plugin::Command<plugin::Commands::CREATE_RANDOM_CHAR>(pos.x, pos.y, pos.z, &m_nPed);
    m_pPed = CPools::GetPed(m_nPed);
    if (!m_pPed) return;
    
    // Hide ped properly
    m_pPed->bIsVisible = false;

    gTotalMouse.x = player->GetHeading() + 89.6f;
    gTotalMouse.y = 0;
    pos.z -= 20;
    m_pPed->SetPosn(pos);

    plugin::Command<plugin::Commands::FREEZE_CHAR_POSITION_AND_DONT_LOAD_COLLISION>(m_nPed, true);
    plugin::Command<plugin::Commands::SET_LOAD_COLLISION_FOR_CHAR_FLAG>(m_nPed, false);
    plugin::Command<plugin::Commands::SET_CHAR_COLLISION>(m_nPed, false);

    // set camera fov
    TheCamera.LerpFOV(TheCamera.FindCamFOV(), MenuState::FreecamFov, 1000, true);
    plugin::Command<plugin::Commands::CAMERA_PERSIST_FOV>(true);
}

void FreecamMgr::Process()
{
    if (!m_pPed) return;

    int delta = (CTimer::m_snTimeInMilliseconds - CTimer::m_snPreviousTimeInMilliseconds);
    int ratio = 1 / (1 + (delta * MenuState::FreecamSpeedMul));
    int speed = MenuState::FreecamSpeedMul + MenuState::FreecamSpeedMul * ratio * delta;
    CVector pos = m_pPed->GetPosition();

    gTotalMouse.x -= CPad::NewMouseControllerState.x / 250.0f;
    gTotalMouse.y += CPad::NewMouseControllerState.y / 3.0f;

    gTotalMouse.x = gTotalMouse.x > 150.0f ? 150.0f : gTotalMouse.x;
    gTotalMouse.x = gTotalMouse.x < -150.0f ? -150.0f : gTotalMouse.x;

    if (GetAsyncKeyState(VK_RETURN) & 0x8000)
    {
        CPlayerPed* player = FindPlayerPed();
        CVector pos = m_pPed->GetPosition();

        CEntity* playerEntity = player;
        pos.z = CWorld::FindGroundZFor3DCoord(pos.x, pos.y, 1000, nullptr, &playerEntity) + 0.5f;
        plugin::Command<plugin::Commands::SET_CHAR_COORDINATES>(CPools::GetPedRef(player), pos.x, pos.y, pos.z);

        CHud::bScriptDontDisplayRadar = true;
        CHud::m_Wants_To_Draw_Hud = false;
        MenuState::ShowNotice(I18n::T("world.freecamTeleported"), 2.0);
    }

    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && MenuState::FreecamSpeedMul > 1)
    {
        speed /= 2;
    }

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        speed *= 2;
    }
        
    if ((GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState('S') & 0x8000))
    {
        if (GetAsyncKeyState('S') & 0x8000)
        {
            speed *= -1;
        }

        float angle;
        plugin::Command<plugin::Commands::GET_CHAR_HEADING>(m_nPed, &angle);

        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            pos.z += speed * sin(90.0f / 3 * 3.14159f / 180.0f);
        }
        else
        {
            pos.x += speed * cos(angle * 3.14159f / 180.0f);
            pos.y += speed * sin(angle * 3.14159f / 180.0f);

            if (!(GetAsyncKeyState(VK_SPACE) & 0x8000))
            {
                pos.z += speed * 2 * sin(gTotalMouse.y / 3 * 3.14159f / 180.0f);
            }
        }
    }

    if ((GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState('D') & 0x8000))
    {
        if (GetAsyncKeyState('A') & 0x8000)
        {
            speed *= -1;
        }

        float angle;
        plugin::Command<plugin::Commands::GET_CHAR_HEADING>(m_nPed, &angle);
        angle -= 90;

        pos.x += speed * cos(angle * 3.14159f / 180.0f);
        pos.y += speed * sin(angle * 3.14159f / 180.0f);
    }

    if (CPad::NewMouseControllerState.wheelUp)
    {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            if (MenuState::FreecamFov > 10.0f)
            {
                MenuState::FreecamFov -= 2.0f * speed;
            }

            TheCamera.LerpFOV(TheCamera.FindCamFOV(), MenuState::FreecamFov, 250, true);
            plugin::Command<plugin::Commands::CAMERA_PERSIST_FOV>(true);
        }
        else
        {
            if (MenuState::FreecamSpeedMul < 10)
            {
                ++MenuState::FreecamSpeedMul;
            }
        }
    }

    if (CPad::NewMouseControllerState.wheelDown)
    {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            if (MenuState::FreecamFov < 115.0f)
            {
                MenuState::FreecamFov += 2.0f * speed;
            }

            TheCamera.LerpFOV(TheCamera.FindCamFOV(), MenuState::FreecamFov, 250, true);
            plugin::Command<plugin::Commands::CAMERA_PERSIST_FOV>(true);
        }
        else
        {
            if (MenuState::FreecamSpeedMul > 1)
            {
                --MenuState::FreecamSpeedMul;
            }
        }
    }

    m_pPed->SetHeading(gTotalMouse.x);
    plugin::Command<plugin::Commands::ATTACH_CAMERA_TO_CHAR>(m_nPed, 0.0, 0.0, 20.0, 90.0, 180, gTotalMouse.y, 0.0, 2);
    m_pPed->SetPosn(pos);
    plugin::Call<0x4045B0>(&pos); // CIPLStore::AddIplsNeededAtPosn(CVector)
}

void FreecamMgr::Disable()
{
    plugin::Command<plugin::Commands::SET_EVERYONE_IGNORE_PLAYER>(0, false);
    plugin::patch::Set<BYTE>(0xBA6769, m_bHudState); // hud
    plugin::patch::Set<BYTE>(0xBA676C, m_bRadarState); // radar

    plugin::Command<plugin::Commands::DELETE_CHAR>(m_nPed);
    m_pPed = nullptr;
    m_nPed = -1;

    plugin::Command<plugin::Commands::CAMERA_PERSIST_FOV>(false);
    plugin::Command<plugin::Commands::RESTORE_CAMERA_JUMPCUT>();
}

FreecamMgr::FreecamMgr()
{
}