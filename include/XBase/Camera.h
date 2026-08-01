#pragma once

namespace XBase::Camera {

enum class Mode {
    Disabled,
    Freecam,
    TopDown,
};

struct Settings {
    float freecamFov = 70.0f;
    int freecamSpeed = 1;
    int topDownZoom = 40;
};

bool SetMode(Mode mode);
Mode GetMode();
bool IsActive();
void SetSettings(const Settings& settings);
Settings GetSettings();
void NotifyGameInit();
void Process();
void Shutdown();

} // namespace XBase::Camera