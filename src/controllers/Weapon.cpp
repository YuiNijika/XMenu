#include "Weapon.h"
#include "features/GameLogic.h"
#include "resources/ResourceData.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "plugin.h"
#include "CPlayerPed.h"
#include "CMessages.h"
#include <unordered_set>

namespace {
    void ShowWeaponMessage(const char* hudText, const char* noticeText) {
        CMessages::AddMessageJumpQ(hudText, 1200, 0);
        MenuState::ShowNotice(noticeText, 2.0);
    }

    std::unordered_set<unsigned int> s_validTypeIds;
    std::unordered_set<unsigned int> s_validModelIds;
    bool s_validationCacheBuilt = false;

    void BuildValidationCache() {
        if (s_validationCacheBuilt) return;
        const Resources::WeaponTable table = Resources::GetWeapons();
        for (std::size_t i = 0; i < table.count; ++i) {
            const Resources::WeaponEntry& w = table.entries->at(i);
            if (w.id > 0 || w.id == -1 || w.id == -2) {
                s_validTypeIds.insert(static_cast<unsigned int>(w.id));
            }
            if (w.modelId > 0) {
                s_validModelIds.insert(static_cast<unsigned int>(w.modelId));
            }
        }
        s_validationCacheBuilt = true;
    }
}

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
        CPlayerPed* player = FindPlayerPed();
        if (!player) {
            return;
        }

        GameLogic::GiveAllWeapons(player);
        ShowWeaponMessage("XMenu: All weapons added", I18n::T("weapon.allGiven"));
    }

    void ClearAll() {
        GameLogic::ClearWeapons(FindPlayerPed());
        ShowWeaponMessage("XMenu: Weapons cleared", I18n::T("weapon.weaponsCleared"));
    }

    void DropWeapon() {
        GameLogic::DropWeapon(FindPlayerPed());
        ShowWeaponMessage("XMenu: Weapon dropped", I18n::T("weapon.weaponDropped"));
    }

    void DropCurrent() {
        GameLogic::DropCurrentWeapon(FindPlayerPed());
        ShowWeaponMessage("XMenu: Current weapon removed", I18n::T("weapon.currentRemoved"));
    }

    void RemovePickups() {
        const int removed = GameLogic::RemoveTrackedPickups();
        ShowWeaponMessage(
            removed > 0 ? "XMenu: Pickups removed" : "XMenu: No pickups to remove",
            removed > 0 ? I18n::T("weapon.pickupsRemoved") : I18n::T("weapon.noPickupsToRemove")
        );
    }

    void Give(unsigned int weaponType, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponTypeId(weaponType)) {
            ShowWeaponMessage("XMenu: Invalid weapon ID", I18n::T("weapon.invalidId"));
            return;
        }
        GameLogic::GiveWeapon(FindPlayerPed(), weaponType, ammo);
        ShowWeaponMessage("XMenu: Weapon added", I18n::T("weapon.weaponGiven"));
    }

    void GiveModel(unsigned int weaponModel, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponModelId(weaponModel)) {
            ShowWeaponMessage("XMenu: Invalid weapon model ID", I18n::T("weapon.invalidId"));
            return;
        }
        GameLogic::GiveWeaponModel(FindPlayerPed(), weaponModel, ammo);
        ShowWeaponMessage("XMenu: Weapon added", I18n::T("weapon.weaponGiven"));
    }

    void GiveSilent(unsigned int weaponType, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponTypeId(weaponType)) return;
        GameLogic::GiveWeapon(FindPlayerPed(), weaponType, ammo);
    }

    void GiveModelSilent(unsigned int weaponModel, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponModelId(weaponModel)) return;
        GameLogic::GiveWeaponModel(FindPlayerPed(), weaponModel, ammo);
    }

    bool IsValidWeaponTypeId(unsigned int weaponType) {
        BuildValidationCache();
        return s_validTypeIds.count(weaponType) > 0;
    }

    bool IsValidWeaponModelId(unsigned int weaponModel) {
        BuildValidationCache();
        return s_validModelIds.count(weaponModel) > 0;
    }

    void ResetStats() {
        GameLogic::ResetWeaponStats();
    }
}