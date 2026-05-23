#pragma once
#include "plugin.h"
#include "CVector.h"

class CPlayerPed;
class CVehicle;

namespace GameLogic {
    struct ProofState {
        bool bullet = false;
        bool collision = false;
        bool explosion = false;
        bool fire = false;
        bool melee = false;
        bool nonPlayer = false;
    };

    // 玩家
    ProofState GetPlayerProofState(CPlayerPed* player);
    void SetPlayerProofState(CPlayerPed* player, const ProofState& state);
    void ApplyGodMode(CPlayerPed* player, bool enable);
    void SetInfiniteSprint(bool enable);
    void SetFreeHealthcare(bool enable);
    bool GetFreeHealthcare();
    void SetFreeJail(bool enable);
    bool GetFreeJail();
    void GiveMoney(int amount);
    void HealPlayer(CPlayerPed* player);
    void GiveArmour(CPlayerPed* player);
    void ProcessAutoHeal(CPlayerPed* player, bool enable);
    void SetWantedLevel(CPlayerPed* player, int level);
    int GetWantedLevel(CPlayerPed* player);
    void ProcessHardMode(CPlayerPed* player, bool enable);
    
    // 载具
    ProofState GetVehicleProofState(CVehicle* vehicle);
    void SetVehicleProofState(CVehicle* vehicle, const ProofState& state);
    void RepairVehicle(CVehicle* vehicle);
    void StopVehicle(CVehicle* vehicle);
    void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed);
    void SetVehicleEngine(CVehicle* vehicle, bool enable);
    void SetVehicleInvincible(CVehicle* vehicle, bool enable);
    
    // 传送
    void TeleportPlayer(CVector pos);
    void TeleportForward(float distance);
    
    // 武器
    void GiveAllWeapons(CPlayerPed* player);
    void ClearWeapons(CPlayerPed* player);
    void ProcessInfiniteAmmo(CPlayerPed* player, bool enable);
    
    // 世界
    void SetTime(int hour, int minute);
    void GetTime(int& hour, int& minute);
    void SyncTimeWithSystemClock();
    void SetGameSpeed(float speed);
    float GetGameSpeed();

    // 通用
    void Init();
    void Process();
}
