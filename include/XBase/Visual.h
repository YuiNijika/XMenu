#pragma once

namespace XBase::Visual {

struct RadarOptions {
    bool square = false;
    bool noRadarRot = false;
    bool fullscreenMap = false;
    bool unfogMap = false;
    bool hideAreaNames = false;
    bool hideVehicleNames = false;
    bool nightVision = false;
    bool infrared = false;
};

bool DisplayHud(bool enable);
bool DisplayRadar(bool enable);
bool SetFilter(int id, float strength);
void SetRadarOptions(const RadarOptions& options);
void NotifyGameInit();
void Shutdown();
void Process();

} // namespace XBase::Visual
