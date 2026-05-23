#pragma once

class CVehicle;

namespace Controllers::Vehicle {
    CVehicle* GetCurrentVehicle();
    void Process();
    void Repair();
    void Start();
    void Stop();
    void SetEngine(bool enable);
    void Unflip();
    void SetHeavy(bool enable);
    void SetWatertight(bool enable);
    bool Spawn(unsigned int modelId);
}