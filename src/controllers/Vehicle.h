#pragma once

class CVehicle;

namespace Controllers::Vehicle {
    CVehicle* GetCurrentVehicle();
    void Process();
    void Repair();
    void Stop();
    void SetEngine(bool enable);
    void SetHeavy(bool enable);
    void SetWatertight(bool enable);
}