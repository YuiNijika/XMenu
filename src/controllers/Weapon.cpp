#include "Weapon.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "plugin.h"
#include "CPlayerPed.h"

namespace Controllers::Weapon {
    bool HasPlayer() {
        return FindPlayerPed() != nullptr;
    }

    void Process() {
        CPlayerPed* player = FindPlayerPed();
        if (!player) {
            return;
        }

        GameLogic::ProcessInfiniteAmmo(player, MenuState::InfiniteAmmo);
        GameLogic::SetFastReload(player, MenuState::FastReload);
        GameLogic::ProcessWeaponTweaks(player, MenuState::HugeWeaponDamage, MenuState::LongWeaponRange);
    }

    void GiveAll() {
        GameLogic::GiveAllWeapons(FindPlayerPed());
    }

    void ClearAll() {
        GameLogic::ClearWeapons(FindPlayerPed());
    }

    void ResetStats() {
        GameLogic::ResetWeaponStats();
    }
}