#include "Ped.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "CPools.h"
#include "CPlayerPed.h"
#include "CVector.h"
#include "CPed.h"

namespace {
    CPed* lastSpawnedPed = nullptr;

    GameLogic::PedSpawnOptions CurrentOptions() {
        GameLogic::PedSpawnOptions options;
        options.modelId = static_cast<unsigned int>(MenuState::PedSpawnModel < 0 ? 0 : MenuState::PedSpawnModel);
        options.pedType = MenuState::PedSpawnType;
        options.gangType = MenuState::PedGangType;
        options.asGang = MenuState::PedSpawnAsGang;
        options.health = MenuState::PedHealth;
        options.armour = MenuState::PedArmour;
        options.freeze = MenuState::PedFreeze;
        options.hostile = MenuState::PedHostile;
        options.weaponModel = static_cast<unsigned int>(MenuState::PedWeaponModel < 0 ? 0 : MenuState::PedWeaponModel);
        return options;
    }
}

namespace Controllers::Ped {
    CPed* GetLastSpawnedPed() {
        return lastSpawnedPed;
    }

    bool GetNoFire() {
        return MenuState::PedsNoFire;
    }

    void SetSpawnLimits(bool limitPolice, bool limitGangs, int maxPolice, int maxGangs) {
        MenuState::PedsLimitPolice = limitPolice;
        MenuState::PedsLimitGangs = limitGangs;
        MenuState::PedsMaxNearbyPolice = maxPolice < 0 ? 0 : maxPolice;
        MenuState::PedsMaxNearbyGangs = maxGangs < 0 ? 0 : maxGangs;
    }

    bool SpawnNearPlayer(const SpawnOptions& options) {
        lastSpawnedPed = GameLogic::SpawnPedNearPlayer(options);
        if (!lastSpawnedPed) {
            MenuState::ShowNotice("Ped spawn failed", 2.0);
            Log::Warn("Ped 生成失败：玩家附近生成未返回有效对象");
            return false;
        }
        MenuState::ShowNotice("Ped spawned", 1.5);
        return true;
    }

    bool SpawnAtMarker(const SpawnOptions& options) {
        lastSpawnedPed = GameLogic::SpawnPedAtMarker(options);
        if (!lastSpawnedPed) {
            MenuState::ShowNotice("Ped marker spawn unavailable", 2.0);
            Log::Warn("Ped 标记点生成失败或当前版本不支持");
            return false;
        }
        MenuState::ShowNotice("Ped spawned at marker", 1.5);
        return true;
    }

    bool SpawnNearPlayer() {
        return SpawnNearPlayer(CurrentOptions());
    }

    bool SpawnAtMarker() {
        return SpawnAtMarker(CurrentOptions());
    }

    void DeleteLastSpawnedPed() {
        GameLogic::DeletePed(lastSpawnedPed);
        lastSpawnedPed = nullptr;
    }

    void Process() {
        if (lastSpawnedPed) {
            GameLogic::ApplyPedOptions(lastSpawnedPed, CurrentOptions());
        }

        CPlayerPed* player = FindPlayerPed();
        if (player) {
            GameLogic::ProcessSmokingEffect(player, MenuState::SmokingEffect);
            GameLogic::ProcessFliesEffect(player, MenuState::FliesEffect);
        }

        // 粘性：仅 re-assert 内存 flag 型开关（任务脚本可能清掉）
        // Mayhem/Riot 在 III 是一次性 cheat 调用，不能每帧触发
        GameLogic::SetElvisEverywhere(MenuState::ElvisEverywhere);
        GameLogic::SetEveryoneArmed(MenuState::EveryoneArmed);
        GameLogic::SetPedsNoFire(MenuState::PedsNoFire);
        GameLogic::SetPedsAtkRocket(MenuState::PedsAtkRocket);
        GameLogic::SetSlutMagnet(MenuState::SlutMagnet);
        GameLogic::SetGangsControl(MenuState::GangsControl);
        GameLogic::SetGangsEverywhere(MenuState::GangsEverywhere);
        GameLogic::SetPedNoProstitutes(MenuState::PedNoProstitutes);
        GameLogic::SetPedNastyLimbs(MenuState::PedNastyLimbs);
#ifdef GTASA
        GameLogic::SetPedsMayhem(MenuState::PedsMayhem);
        GameLogic::SetPedsRiot(MenuState::PedsRiot);
        GameLogic::SetGangWarsActive(MenuState::GangWarsActive);
#endif

        // Fallback no-fire: 池扫描压制
        // III 无 hook，回退有效；SA/VC 作为双保险
        // 节流：每 N 帧检查一次
        static int s_nofireTick = 0;
        if ((++s_nofireTick & 7) == 0) {  // ~每 8 帧
            if (MenuState::PedsNoFire && CPools::ms_pPedPool) {
                for (CPed* p : CPools::ms_pPedPool) {
                    if (!p) continue;
                    if (GameLogic::ShouldSuppressPedFire(p)) {
                        GameLogic::ClearPedAiming(p);
                    }
                }
            }
        }

        // Spawn limit culling (nearby ~120m, non-mission). Runs under same throttle window.
        if (MenuState::PedsLimitPolice || MenuState::PedsLimitGangs) {
            CPlayerPed* pl = FindPlayerPed();
            if (pl && CPools::ms_pPedPool) {
                CVector ppos = pl->GetPosition();
                int countCop = 0, countGang = 0;
                for (CPed* p : CPools::ms_pPedPool) {
                    if (!p || p == pl) continue;
                    if (p->m_fHealth <= 0.0f || GameLogic::IsMissionPed(p)) continue;
                    CVector pp = p->GetPosition();
                    float d2 = (pp.x-ppos.x)*(pp.x-ppos.x) + (pp.y-ppos.y)*(pp.y-ppos.y) + (pp.z-ppos.z)*(pp.z-ppos.z);
                    if (d2 > 120.0f*120.0f) continue;
                    if (GameLogic::IsCopPed(p) && MenuState::PedsLimitPolice) ++countCop;
                    else if (GameLogic::IsGangPed(p) && MenuState::PedsLimitGangs) ++countGang;
                }
                for (CPed* p : CPools::ms_pPedPool) {
                    if (!p || p == pl) continue;
                    if (p->m_fHealth <= 0.0f || GameLogic::IsMissionPed(p)) continue;
                    CVector pp = p->GetPosition();
                    float d2 = (pp.x-ppos.x)*(pp.x-ppos.x) + (pp.y-ppos.y)*(pp.y-ppos.y) + (pp.z-ppos.z)*(pp.z-ppos.z);
                    if (d2 > 120.0f*120.0f) continue;
                    if (MenuState::PedsLimitPolice && GameLogic::IsCopPed(p) && countCop > MenuState::PedsMaxNearbyPolice) {
                        GameLogic::DeletePed(p); --countCop; continue;
                    }
                    if (MenuState::PedsLimitGangs && GameLogic::IsGangPed(p) && countGang > MenuState::PedsMaxNearbyGangs) {
                        GameLogic::DeletePed(p); --countGang; continue;
                    }
                }
            }
        }
    }

    void SetElvisEverywhere(bool enable) {
        GameLogic::SetElvisEverywhere(enable);
    }
    void SetEveryoneArmed(bool enable) {
        GameLogic::SetEveryoneArmed(enable);
    }
    void SetPedsMayhem(bool enable) {
        GameLogic::SetPedsMayhem(enable);
    }
    void SetPedsAtkRocket(bool enable) {
        GameLogic::SetPedsAtkRocket(enable);
    }
    void SetPedsRiot(bool enable) {
        GameLogic::SetPedsRiot(enable);
    }
    void SetSlutMagnet(bool enable) {
        GameLogic::SetSlutMagnet(enable);
    }
    void SetGangsControl(bool enable) {
        GameLogic::SetGangsControl(enable);
    }
    void SetGangsEverywhere(bool enable) {
        GameLogic::SetGangsEverywhere(enable);
    }

    void SetNoProstitutes(bool enable) {
        GameLogic::SetPedNoProstitutes(enable);
    }

    void SetNastyLimbs(bool enable) {
        GameLogic::SetPedNastyLimbs(enable);
    }

    void SetGangWarsActive(bool enable) {
        GameLogic::SetGangWarsActive(enable);
    }

    void StartGangWar(bool offensive) {
        GameLogic::StartGangWar(offensive);
    }

    void EndGangWar() {
        GameLogic::EndGangWar();
    }

    int GetGangZoneDensity(int gangId) {
        return GameLogic::GetGangZoneDensity(gangId);
    }

    void SetGangZoneDensity(int gangId, int density) {
        GameLogic::SetGangZoneDensity(gangId, density);
    }

    unsigned int GetGangMemberModel(unsigned int gangId, unsigned int memberId) {
        return GameLogic::GetGangMemberModel(gangId, memberId);
    }

    void SetGangMemberModel(unsigned int gangId, unsigned int memberId, unsigned int model) {
        GameLogic::SetGangMemberModel(gangId, memberId, model);
    }

    void ResetGangModels() {
        GameLogic::ResetGangModels();
    }

    void SetGangWeapons(unsigned int gangId, int weapon1, int weapon2, int weapon3) {
        GameLogic::SetGangWeapons(gangId, weapon1, weapon2, weapon3);
    }

    void SetPedsNoFire(bool enable) {
        GameLogic::SetPedsNoFire(enable);
    }
}