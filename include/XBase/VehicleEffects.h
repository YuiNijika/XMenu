#pragma once

namespace XBase::VehicleEffects {

struct NeonSettings {
    bool enabled = false;
    int red = 255;
    int green = 0;
    int blue = 0;
    bool pulsing = false;
};

bool ApplyCurrentNeon(const NeonSettings& settings);
void Init();
void NotifyGameInit();
void Process();
void Shutdown();

} // namespace XBase::VehicleEffects