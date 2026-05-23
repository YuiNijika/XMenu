#pragma once

class CPlayerPed;

namespace Controllers::Player {
    CPlayerPed* GetPlayer();
    void Process();
    void Heal();
    void GiveArmour();
    void GiveMoney();
    void Kill();
    int GetWantedLevel();
    void SetWantedLevel(int level);
    void ClearWantedLevel();
    void SetInfiniteSprint(bool enable);
    bool GetFreeHealthcare();
    void SetFreeHealthcare(bool enable);
    bool GetFreeJail();
    void SetFreeJail(bool enable);
}