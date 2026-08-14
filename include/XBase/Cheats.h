#pragma once

#include <cstddef>

namespace XBase::Cheats {

struct RandomSettings {
    bool enabled = false;
    bool showProgress = false;
    int intervalSeconds = 5;
};

void SetRandomSettings(const RandomSettings& settings);
RandomSettings GetRandomSettings();
std::size_t GetRandomCheatCount();
const char* GetRandomCheatName(std::size_t index);
bool IsRandomCheatEnabled(std::size_t index);
bool SetRandomCheatEnabled(std::size_t index, bool enabled);
void Init();
void NotifyGameInit();
void Process();
void Shutdown();

void FlyingCars(bool enable);
bool IsFlyingCars();

void AllCarsHaveNitro(bool enable);
bool IsAllCarsHaveNitro();

void PerfectHandling(bool enable);
bool IsPerfectHandling();

void GreenLights(bool enable);
bool IsGreenLights();

void Riot(bool enable);
bool IsRiot();

void BoatFly(bool enable);
bool IsBoatFly();

void DriveWater(bool enable);
bool IsDriveWater();

void TankMode(bool enable);
bool IsTankMode();

void AimDrive(bool enable);
bool IsAimDrive();

void NoDerail(bool enable);
bool IsNoDerail();

void FlipNoBurn(bool enable);
bool IsFlipNoBurn();

void StayOnBike(bool enable);
bool IsStayOnBike();

void BikeFly(bool enable);
bool IsBikeFly();

} // namespace XBase::Cheats
