#include "Ped.h"
#include <XBase/Ped.h>
#include <XBase/Types.h>
#include "ui/MenuState.h"
#include "utils/Log.h"

namespace {
    XBase::Types::PedSpawnOptions CurrentOptions() {
        XBase::Types::PedSpawnOptions options;
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
    XBase::PedId GetLastSpawnedId() { return XBase::Ped::GetLastSpawnedId(); }
    bool SpawnNearPlayer() {
        const SpawnOptions options = CurrentOptions();
        const bool success = XBase::Ped::SpawnNearPlayer(options.modelId, options);
        if (!success) {
            MenuState::ShowNotice("Ped spawn failed", 2.0);
            Log::Warn("Ped 生成失败：玩家附近生成未返回有效对象");
        } else {
            MenuState::ShowNotice("Ped spawned", 1.5);
        }
        return success;
    }

    bool SpawnAtMarker() {
        const SpawnOptions options = CurrentOptions();
        const bool success = XBase::Ped::SpawnAtMarker(options.modelId, options);
        if (!success) {
            MenuState::ShowNotice("Ped marker spawn unavailable", 2.0);
            Log::Warn("Ped 标记点生成失败或当前版本不支持");
        } else {
            MenuState::ShowNotice("Ped spawned at marker", 1.5);
        }
        return success;
    }

    void DeleteLastSpawnedPed() {
        XBase::Ped::DeleteLastSpawned();
    }

    void SetBigHead(bool enable) { XBase::Ped::SetBigHead(enable); }
    void SetThinBody(bool enable) { XBase::Ped::SetThinBody(enable); }
    void Process() {
        XBase::Ped::SetBigHead(MenuState::BigHeadMode);
        XBase::Ped::SetThinBody(MenuState::ThinBodyMode);
    }

    void SetElvisEverywhere(bool enable) { XBase::Ped::SetElvisEverywhere(enable); }
    void SetEveryoneArmed(bool enable) { XBase::Ped::SetEveryoneArmed(enable); }
    void SetPedsMayhem(bool enable) { XBase::Ped::SetPedsMayhem(enable); }
    void SetPedsAtkRocket(bool enable) { XBase::Ped::SetPedsAtkRocket(enable); }
    void SetPedsRiot(bool enable) { XBase::Ped::SetPedsRiot(enable); }
    void SetPedsNoFire(bool enable) { XBase::Ped::SetNoFire(enable); }
    void SetSlutMagnet(bool enable) { XBase::Ped::SetSlutMagnet(enable); }
    void SetGangsControl(bool enable) { XBase::Ped::SetGangsControl(enable); }
    void SetGangsEverywhere(bool enable) { XBase::Ped::SetGangsEverywhere(enable); }
    void SetNoProstitutes(bool enable) { XBase::Ped::SetNoProstitutes(enable); }
    void SetNastyLimbs(bool enable) { XBase::Ped::SetNastyLimbs(enable); }
    void SetGangWarsActive(bool enable) { XBase::Ped::SetGangWarsActive(enable); }
    void StartGangWar(bool offensive) { XBase::Ped::StartGangWar(offensive); }
    void EndGangWar() { XBase::Ped::EndGangWar(); }
    int GetGangZoneDensity(int gangId) { return XBase::Ped::GetGangZoneDensity(gangId); }
    void SetGangZoneDensity(int gangId, int density) { XBase::Ped::SetGangZoneDensity(gangId, density); }
    unsigned int GetGangMemberModel(unsigned int gangId, unsigned int memberId) {
        return XBase::Ped::GetGangMemberModel(gangId, memberId);
    }
    void SetGangMemberModel(unsigned int gangId, unsigned int memberId, unsigned int model) {
        XBase::Ped::SetGangMemberModel(gangId, memberId, model);
    }
    void ResetGangModels() { XBase::Ped::ResetGangModels(); }
    void SetGangWeapons(unsigned int gangId, int weapon1, int weapon2, int weapon3) {
        XBase::Ped::SetGangWeapons(gangId, weapon1, weapon2, weapon3);
    }
}