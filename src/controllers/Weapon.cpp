#include "Weapon.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
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
        GameLogic::ProcessWeaponTweaks(
            player,
            MenuState::HugeWeaponDamage,
            MenuState::LongWeaponRange,
            MenuState::RapidFire,
            MenuState::DualWield,
            MenuState::MoveAim,
            MenuState::MoveFire,
            MenuState::NoSpread
        );
    }

    void GiveAll() {
        GameLogic::GiveAllWeapons(FindPlayerPed());
    }

    void ClearAll() {
        GameLogic::ClearWeapons(FindPlayerPed());
    }

    void DropWeapon() {
        GameLogic::DropWeapon(FindPlayerPed());
    }

    void DropCurrent() {
        GameLogic::DropCurrentWeapon(FindPlayerPed());
    }

    void RemovePickups() {
        const int removed = GameLogic::RemoveTrackedPickups();
        MenuState::ShowNotice(removed > 0 ? I18n::T("weapon.pickupsRemoved") : I18n::T("weapon.noPickupsToRemove"), 1.5);
    }

    void Give(unsigned int weaponType, unsigned int ammo) {
        GameLogic::GiveWeapon(FindPlayerPed(), weaponType, ammo);
    }

    void GiveModel(unsigned int weaponModel, unsigned int ammo) {
        GameLogic::GiveWeaponModel(FindPlayerPed(), weaponModel, ammo);
    }

    void ResetStats() {
        GameLogic::ResetWeaponStats();
    }
}