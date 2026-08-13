#include "Weapon.h"
#include <XBase/Weapon.h>
#include <XBase/Capabilities.h>
#include <XBase/Player.h>
#include <XBase/Host.h>
#include "resources/ResourceData.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include <unordered_set>

namespace {
    void ShowWeaponMessage(const char* hudText, const char* noticeText) {
        XBase::Host::ShowMessage(hudText);
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
        return XBase::Player::IsAvailable();
    }

    void Process() {
        const bool hasRuntimeEffects = XBase::HasCapability(XBase::FeatureCapability::WeaponRuntimeEffects);
        const bool hasStatOverrides = XBase::HasCapability(XBase::FeatureCapability::WeaponStatOverrides);
        const bool hasGive = XBase::HasCapability(XBase::FeatureCapability::WeaponGive);

        if (!hasRuntimeEffects) {
            MenuState::InfiniteAmmo = false;
            MenuState::FastReload = false;
        }
        if (!hasStatOverrides) {
            MenuState::HugeWeaponDamage = false;
            MenuState::LongWeaponRange = false;
            MenuState::WeaponAutoAim = false;
            MenuState::MoveAim = false;
            MenuState::MoveFire = false;
            MenuState::NoSpread = false;
            MenuState::RapidFire = false;
            MenuState::DualWield = false;
            MenuState::WeaponFireRateEnabled = false;
        }
        if (!hasGive) {
            MenuState::WeaponCyclerEnabled = false;
        }

        if (hasRuntimeEffects) {
            XBase::Weapon::SetInfiniteAmmo(MenuState::InfiniteAmmo);
            XBase::Weapon::SetFastReload(MenuState::FastReload);
        }
    }

    bool GiveAll() {
        if (!XBase::Weapon::GiveAll()) {
            ShowWeaponMessage("XMenu: Weapon operation failed", I18n::T("weapon.operationFailed"));
            return false;
        }
        ShowWeaponMessage("XMenu: All weapons added", I18n::T("weapon.allGiven"));
        return true;
    }

    bool ClearAll() {
        if (!XBase::Weapon::ClearAll()) {
            ShowWeaponMessage("XMenu: Weapon operation failed", I18n::T("weapon.operationFailed"));
            return false;
        }
        ShowWeaponMessage("XMenu: Weapons cleared", I18n::T("weapon.weaponsCleared"));
        return true;
    }

    bool DropWeapon() {
        if (!XBase::Weapon::DropWeapon()) {
            ShowWeaponMessage("XMenu: Weapon operation failed", I18n::T("weapon.operationFailed"));
            return false;
        }
        ShowWeaponMessage("XMenu: Weapon dropped", I18n::T("weapon.weaponDropped"));
        return true;
    }

    bool DropCurrent() {
        if (!XBase::Weapon::DropCurrent()) {
            ShowWeaponMessage("XMenu: Weapon operation failed", I18n::T("weapon.operationFailed"));
            return false;
        }
        ShowWeaponMessage("XMenu: Current weapon removed", I18n::T("weapon.currentRemoved"));
        return true;
    }

    int RemovePickups() {
        const int removed = XBase::Weapon::RemoveTrackedPickups();
        ShowWeaponMessage(
            removed > 0 ? "XMenu: Pickups removed" : "XMenu: No pickups to remove",
            removed > 0 ? I18n::T("weapon.pickupsRemoved") : I18n::T("weapon.noPickupsToRemove")
        );
        return removed;
    }

    bool Give(unsigned int weaponType, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponTypeId(weaponType)) {
            ShowWeaponMessage("XMenu: Invalid weapon ID", I18n::T("weapon.invalidId"));
            return false;
        }
        if (!XBase::Weapon::Give(weaponType, ammo)) {
            ShowWeaponMessage("XMenu: Weapon operation failed", I18n::T("weapon.operationFailed"));
            return false;
        }
        ShowWeaponMessage("XMenu: Weapon added", I18n::T("weapon.weaponGiven"));
        return true;
    }

    bool GiveModel(unsigned int weaponModel, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponModelId(weaponModel)) {
            ShowWeaponMessage("XMenu: Invalid weapon model ID", I18n::T("weapon.invalidId"));
            return false;
        }
        if (!XBase::Weapon::GiveModel(weaponModel, ammo)) {
            ShowWeaponMessage("XMenu: Weapon operation failed", I18n::T("weapon.operationFailed"));
            return false;
        }
        ShowWeaponMessage("XMenu: Weapon added", I18n::T("weapon.weaponGiven"));
        return true;
    }

    bool GiveSilent(unsigned int weaponType, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponTypeId(weaponType)) return false;
        return XBase::Weapon::Give(weaponType, ammo);
    }

    bool GiveModelSilent(unsigned int weaponModel, unsigned int ammo) {
        if (MenuState::WeaponSafeMode && !IsValidWeaponModelId(weaponModel)) return false;
        return XBase::Weapon::GiveModel(weaponModel, ammo);
    }

    bool IsValidWeaponTypeId(unsigned int weaponType) {
        BuildValidationCache();
        return s_validTypeIds.count(weaponType) > 0;
    }

    bool IsValidWeaponModelId(unsigned int weaponModel) {
        BuildValidationCache();
        return s_validModelIds.count(weaponModel) > 0;
    }

    bool ResetStats() {
        return XBase::Weapon::ResetStats();
    }
}