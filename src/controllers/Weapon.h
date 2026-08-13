#pragma once

namespace Controllers::Weapon {
    void Process();
    bool HasPlayer();
    bool GiveAll();
    bool ClearAll();
    bool DropWeapon();
    bool DropCurrent();
    int RemovePickups();
    bool Give(unsigned int weaponType, unsigned int ammo);
    bool GiveModel(unsigned int weaponModel, unsigned int ammo);
    bool GiveSilent(unsigned int weaponType, unsigned int ammo);
    bool GiveModelSilent(unsigned int weaponModel, unsigned int ammo);
    bool IsValidWeaponTypeId(unsigned int weaponType);
    bool IsValidWeaponModelId(unsigned int weaponModel);
    bool ResetStats();
}