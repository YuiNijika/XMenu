#include "Visual.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"

namespace Controllers::Visual {
    void ApplyHudState() {
        GameLogic::DisplayHud(MenuState::VisualHud);
    }

    void ApplyRadarState() {
        GameLogic::DisplayRadar(MenuState::VisualRadar);
    }

    void ApplyFilterState() {
        GameLogic::SetVisualFilter(MenuState::VisualFilter, MenuState::VisualFilterId, MenuState::VisualTimecycStrength);
    }

    void Process() {
        ApplyHudState();
        ApplyRadarState();
        ApplyFilterState();
    }
}