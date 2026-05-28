#pragma once

namespace Controllers::Weapon {
    void Process();
    bool HasPlayer();
    void GiveAll();
    void ClearAll();
    void DropWeapon();
    void DropCurrent();
    void RemovePickups();
    void Give(unsigned int weaponType, unsigned int ammo);
    void GiveModel(unsigned int weaponModel, unsigned int ammo);
    void GiveSilent(unsigned int weaponType, unsigned int ammo);
    void GiveModelSilent(unsigned int weaponModel, unsigned int ammo);
    void ResetStats();
}