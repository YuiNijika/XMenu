#include "Visual.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"

namespace Controllers::Visual {
    void DisplayHud(bool enable) {
        GameLogic::DisplayHud(enable);
    }

    void DisplayRadar(bool enable) {
        GameLogic::DisplayRadar(enable);
    }

    void SetFilter(int id, float strength) {
        GameLogic::SetVisualFilter(true, id, strength);
    }

    void ApplyHudState() {
        DisplayHud(MenuState::VisualHud);
    }

    void ApplyRadarState() {
        DisplayRadar(MenuState::VisualRadar);
    }

    void ApplyFilterState() {
        GameLogic::SetVisualFilter(MenuState::VisualFilter, MenuState::VisualFilterId, MenuState::VisualTimecycStrength);
    }

    void Process() {
        ApplyHudState();
        ApplyRadarState();
        ApplyFilterState();
        GameLogic::ProcessVisualExtras();
    }
}