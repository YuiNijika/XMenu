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

} // namespace XBase::Cheats
