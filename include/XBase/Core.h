#pragma once

#include <cstdint>

namespace XBase::Core {

enum class Domain : std::uint32_t {
    Player = 1u << 0,
    Ped = 1u << 1,
    Vehicle = 1u << 2,
    World = 1u << 3,
    Weapon = 1u << 4,
    Teleport = 1u << 5,
    Scene = 1u << 6,
    Visual = 1u << 7,
    BulletAssist = 1u << 8,
    Overlay = 1u << 9,
    Camera = 1u << 10,
    Cheats = 1u << 11,
    VehicleEffects = 1u << 12,
};

using DomainMask = std::uint32_t;

constexpr DomainMask DomainBit(Domain domain) {
    return static_cast<DomainMask>(domain);
}

constexpr DomainMask AllDomains =
    DomainBit(Domain::Player) |
    DomainBit(Domain::Ped) |
    DomainBit(Domain::Vehicle) |
    DomainBit(Domain::World) |
    DomainBit(Domain::Weapon) |
    DomainBit(Domain::Teleport) |
    DomainBit(Domain::Scene) |
    DomainBit(Domain::Visual) |
    DomainBit(Domain::BulletAssist) |
    DomainBit(Domain::Overlay) |
    DomainBit(Domain::Camera) |
    DomainBit(Domain::Cheats) |
    DomainBit(Domain::VehicleEffects);

extern bool s_gameInitialized;

void Init(DomainMask enabledDomains = 0);
void Process();
void Shutdown();
bool IsWorldReady();
void NotifyGameInit();
void SetEnabledDomains(DomainMask enabledDomains);
DomainMask GetEnabledDomains();
bool IsDomainEnabled(Domain domain);

} // namespace XBase::Core
