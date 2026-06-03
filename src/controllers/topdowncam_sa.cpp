#include "topdowncam_sa.h"
#include "ui/MenuState.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CCamera.h"
#include "extensions/ScriptCommands.h"
#include "plugin.h"

TopDownCamera& TopDownCam = TopDownCamera::Get();

TopDownCamera::TopDownCamera()
{
}

void TopDownCamera::Process()
{ 
    if (!MenuState::TopDownCamEnabled) {
        return;
    }

    CPlayerPed *player = FindPlayerPed();
    if (!player) return;

    CVector     pos    = player->GetPosition ();
    float       targetOffset = static_cast<float>(MenuState::TopDownCamZoom);
    static float s_curOffset = targetOffset;

    // drunk effect causes issues
    plugin::Command<plugin::Commands::SET_PLAYER_DRUNKENNESS> (0, 0);

    CVehicle *vehicle = FindPlayerVehicle(-1, false);

    if (vehicle)
    {
        float speed = vehicle->m_vecMoveSpeed.Magnitude();
        targetOffset += std::min(speed * 35.0f, 40.0f);
    }

    // Smooth transition
    s_curOffset += (targetOffset - s_curOffset) * 0.05f;

    CVector playerOffset = CVector (pos.x, pos.y, pos.z + 2.0f);
    CVector cameraPos = CVector (playerOffset.x, playerOffset.y, playerOffset.z + s_curOffset);

    CColPoint outColPoint;
    CEntity * outEntity;

    // TODO: Which variable? X, Y or Z for the look direction?

    if (CWorld::ProcessLineOfSight (playerOffset, cameraPos, outColPoint,
                                    outEntity, true, true, true, true, true,
                                    true, true, true))
    {
        plugin::Command<plugin::Commands::SET_FIXED_CAMERA_POSITION> (
            outColPoint.m_vecPoint.x, outColPoint.m_vecPoint.y,
            outColPoint.m_vecPoint.z, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        plugin::Command<plugin::Commands::SET_FIXED_CAMERA_POSITION> (
            cameraPos.x, cameraPos.y, cameraPos.z, 0.0f, 0.0f, 0.0f);
    }

    plugin::Command<plugin::Commands::POINT_CAMERA_AT_POINT> (pos.x, pos.y,
            pos.z, 2);

    TheCamera.m_fGenerationDistMultiplier = 10.0f;
    TheCamera.m_fLODDistMultiplier        = 10.0f;
}

void TopDownCamera::Disable()
{
    plugin::Command<plugin::Commands::RESTORE_CAMERA_JUMPCUT>();
}
