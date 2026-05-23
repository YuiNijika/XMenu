#pragma once

class CVehicle;

namespace Controllers::Vehicle {
    CVehicle* GetCurrentVehicle();
    void Process();
    void Repair();
    void Stop();
    void SetEngine(bool enable);
}