#pragma once

namespace XBase::Weapon {

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

} // namespace XBase::Weapon
