#pragma once
#include "plugin.h"
#include "CVector.h"

class CPlayerPed;
class CVehicle;
class CPed;

namespace GameLogic {
    struct ProofState {
        bool bullet = false;
        bool collision = false;
        bool explosion = false;
        bool fire = false;
        bool melee = false;
        bool nonPlayer = false;
    };

    struct SpawnVehicleOptions {
        bool asDriver = true;
        bool aircraftInAir = true;
        bool cleanupPrevious = true;
    };

    struct PedSpawnOptions {
        unsigned int modelId = 7;
        int pedType = 4;
        int gangType = 0;
        bool asGang = false;
        float health = 100.0f;
        float armour = 0.0f;
        bool freeze = false;
        bool hostile = false;
        unsigned int weaponModel = 0;
    };

    struct PickupOptions {
        unsigned int modelId = 1240;
        unsigned char type = 3;
        unsigned int quantity = 1;
        unsigned int moneyPerDay = 0;
        bool empty = false;
    };

    struct VehicleAppearanceOptions {
        int primaryColor = 0;
        int secondaryColor = 0;
        int paintjob = -1;
        int modId = 1000;
    };

    struct PlayerAppearanceOptions {
        int skinModel = 0;
        int clothesTexture = 0;
        int clothesModel = 0;
        int clothesBodyPart = 0;
        int statId = 0;
        float statValue = 0.0f;
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
    int GetMoney();
    void SetMoney(int amount);
    bool SetPlayerSkin(unsigned int modelId);
    bool ApplyPlayerClothes(int textureId, int modelId, int bodyPart);
    bool SetPlayerCustomSkin(const char* name);
    bool SetPlayerStat(int statId, float value);
    float GetHealth(CPlayerPed* player);
    void SetHealth(CPlayerPed* player, float value);
    float GetArmour(CPlayerPed* player);
    void SetArmour(CPlayerPed* player, float value);
    CVector GetPlayerPosition(CPlayerPed* player);
    void MovePlayerRelative(CPlayerPed* player, float forward, float right, float up);
    bool IsPlayerDead(CPlayerPed* player);
    void SetKeepStuff(bool enable);
    void HealPlayer(CPlayerPed* player);
    void GiveArmour(CPlayerPed* player);
    void ProcessAutoHeal(CPlayerPed* player, bool enable);
    void SetWantedLevel(CPlayerPed* player, int level);
    int GetWantedLevel(CPlayerPed* player);
    void ProcessHardMode(CPlayerPed* player, bool enable);
    void ProcessRespawnAtDeathPosition(CPlayerPed* player, bool enable);
    void ProcessFreezeWantedLevel(CPlayerPed* player, bool enable, int level);
    void SetManualPlayerProof(CPlayerPed* player, const ProofState& state);
    bool RequestSaveGame();

    // 载具
    ProofState GetVehicleProofState(CVehicle* vehicle);
    void SetVehicleProofState(CVehicle* vehicle, const ProofState& state);
    void RepairVehicle(CVehicle* vehicle);
    void StopVehicle(CVehicle* vehicle);
    void SetVehicleSpeedLock(CVehicle* vehicle, bool enable, float speed);
    void SetVehicleForwardSpeed(CVehicle* vehicle, float speed);
    void SetVehicleEngine(CVehicle* vehicle, bool enable);
    void SetVehicleInvincible(CVehicle* vehicle, bool enable);
    void UnflipVehicle(CVehicle* vehicle);
    void SetVehicleHeavy(CVehicle* vehicle, bool enable);
    void SetVehicleWatertight(CVehicle* vehicle, bool enable);
    float GetVehicleHealth(CVehicle* vehicle);
    void SetVehicleHealth(CVehicle* vehicle, float health);
    bool GetVehicleLights(CVehicle* vehicle);
    void SetVehicleLights(CVehicle* vehicle, bool enable);
    bool GetVehicleLocked(CVehicle* vehicle);
    void SetVehicleLocked(CVehicle* vehicle, bool enable);
    bool GetVehicleVisible(CVehicle* vehicle);
    void SetVehicleVisible(CVehicle* vehicle, bool enable);
    bool GetVehicleAlwaysSkidMarks(CVehicle* vehicle);
    void SetVehicleAlwaysSkidMarks(CVehicle* vehicle, bool enable);
    bool GetVehicleDisableParticles(CVehicle* vehicle);
    void SetVehicleDisableParticles(CVehicle* vehicle, bool enable);
    bool GetVehicleDriverTargetable(CVehicle* vehicle);
    void SetVehicleDriverTargetable(CVehicle* vehicle, bool enable);
    bool GetVehicleHeatSeekingTargetable(CVehicle* vehicle);
    void SetVehicleHeatSeekingTargetable(CVehicle* vehicle, bool enable);
    bool GetVehiclePetrolTankWeakPoint(CVehicle* vehicle);
    void SetVehiclePetrolTankWeakPoint(CVehicle* vehicle, bool enable);
    bool GetVehicleSirenOrAlarm(CVehicle* vehicle);
    void SetVehicleSirenOrAlarm(CVehicle* vehicle, bool enable);
    bool GetVehicleTakeLessDamage(CVehicle* vehicle);
    void SetVehicleTakeLessDamage(CVehicle* vehicle, bool enable);
    void BlowUpAllVehicles();
    bool IsValidVehicleModel(unsigned int modelId);
    bool IsValidPedModel(unsigned int modelId);
    CVehicle* SpawnVehicle(unsigned int modelId, const SpawnVehicleOptions& options);
    void DeleteVehicle(CVehicle* vehicle);
    void ApplyVehicleAppearance(CVehicle* vehicle, const VehicleAppearanceOptions& options);
    void OpenVehicleDoor(CVehicle* vehicle, int doorIndex);
    void PopVehicleDoor(CVehicle* vehicle, int doorIndex);
    void WarpPlayerToVehicleSeat(CVehicle* vehicle, int seatIndex);
    void ProcessAutoDrive(CVehicle* vehicle, bool enable, float speed);
    void SetTrafficDensity(float density);
    void SetFlyingCars(bool enable);
    
    // Ped
    CPed* SpawnPedNearPlayer(const PedSpawnOptions& options);
    CPed* SpawnPedAtMarker(const PedSpawnOptions& options);
    void DeletePed(CPed* ped);
    void ApplyPedOptions(CPed* ped, const PedSpawnOptions& options);
    void SetElvisEverywhere(bool enable);
    void SetEveryoneArmed(bool enable);
    void SetPedsMayhem(bool enable);
    void SetPedsAtkRocket(bool enable);
    void SetPedsRiot(bool enable);
    void SetSlutMagnet(bool enable);
    void SetGangsControl(bool enable);
    void SetGangsEverywhere(bool enable);
    
    // 场景
    bool PlayPlayerAnimation(const char* group, const char* name, bool loop);
    void StopPlayerAnimation();
    bool SpawnParticleAtPlayer(const char* name);
    void ProcessSmokingEffect(CPlayerPed* player, bool enable);
    void ProcessFliesEffect(CPlayerPed* player, bool enable);
    bool StartCutscene(const char* name);
    void StopCutscene();
    bool IsCutsceneRunning();
    const char* GetMissionStatus();
    void StartMission(int missionId);
    void FailMission();
    
    // 视觉
    void DisplayHud(bool enable);
    void DisplayRadar(bool enable);
    void SetVisualFilter(bool enable, int filterId, float strength);
    void TeleportPlayer(CVector pos, int interiorID = 0);
    void TeleportMapPosition(CVector pos, bool spawnUnderwater);
    bool TeleportMarker(bool spawnUnderwater);
    void TeleportForward(float distance);
    
    // 武器
    void GiveAllWeapons(CPlayerPed* player);
    void GiveWeapon(CPlayerPed* player, unsigned int weaponType, unsigned int ammo);
    void GiveWeaponModel(CPlayerPed* player, unsigned int weaponModel, unsigned int ammo);
    void DropWeapon(CPlayerPed* player);
    void DropCurrentWeapon(CPlayerPed* player);
    void ClearWeapons(CPlayerPed* player);
    int RemoveTrackedPickups();
    void ProcessInfiniteAmmo(CPlayerPed* player, bool enable);
    void SetFastReload(CPlayerPed* player, bool enable);
    void ProcessWeaponTweaks(CPlayerPed* player, bool hugeDamage, bool longRange, bool rapidFire, bool dualWield, bool moveAim, bool moveFire, bool noSpread);
    void ResetWeaponStats();
    
    // 世界
    void SetTime(int hour, int minute);
    void GetTime(int& hour, int& minute);
    void SyncTimeWithSystemClock();
    void SetGameSpeed(float speed);
    float GetGameSpeed();
    int GetFpsLimit();
    void SetFpsLimit(int limit);
    void SetDisableReplay(bool enable);
    void SetDisableCheats(bool enable);
    void SetForbiddenAreaWanted(bool enable);
    void SetFreePayNSpray(bool enable);
    void SetFasterClock(bool enable);
    void SetFreezeTime(bool enable);
    void ProcessSolidWater(CPlayerPed* player, bool enable);
    void SetNoWaterPhysics(bool enable);
    int GetDaysPassed();
    void SetDaysPassed(int days);
    float GetGravity();
    void SetGravity(float gravity);
    int SpawnPickupNearPlayer(const PickupOptions& options);
    bool UpdateLastPickup(const PickupOptions& options);
    bool RemoveLastPickup();

    // 通用
    void Init();
    void Process();
}
