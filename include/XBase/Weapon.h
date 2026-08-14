#pragma once

namespace XBase::Weapon {

struct StatOverrides {
    bool hugeDamage = false;
    bool longRange = false;
    bool rapidFire = false;
    bool dualWield = false;
    bool moveAim = false;
    bool moveFire = false;
    bool noSpread = false;
    bool customFireRate = false;
    float fireRate = 1.0f;
    bool autoAim = false;
};

void NotifyGameInit();
void Shutdown();
void Process();
bool GiveAll();
bool ClearAll();
bool DropWeapon();
bool DropCurrent();
int RemoveTrackedPickups();
bool Give(unsigned int weaponType, unsigned int ammo);
bool GiveModel(unsigned int weaponModel, unsigned int ammo);
bool MaxWeaponSkills();
bool SetInfiniteAmmo(bool enable);
bool SetFastReload(bool enable);
bool ResetStats();
void SetStatOverrides(const StatOverrides& overrides);

} // namespace XBase::Weapon
