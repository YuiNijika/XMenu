#pragma once

namespace Controllers::Visual {
    void Process();
    void DisplayHud(bool enable);
    void DisplayRadar(bool enable);
    void SetFilter(int id, float strength);
    void ApplyHudState();
    void ApplyRadarState();
    void ApplyFilterState();
}