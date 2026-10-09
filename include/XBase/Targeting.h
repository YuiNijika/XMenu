#pragma once

#include <cstddef>
#include <array>
#include <string>
#include <vector>

#include "ValueTypes.h"

namespace XBase::Targeting {

enum class Kind {
    Ped,
    Vehicle,
};

enum class Action {
    None, Restore, Kill, Ignite, Armour, Disarm, Upright, Unlock, Bring, Teleport,
    Health, MaxHealth, Colors, Explode, Lock, Engine, Lights, Stop, Speed,
    Visible, Proofs, BulletProof, FireProof, ExplosionProof, CollisionProof, MeleeProof,
    Weapon, OpenDoor, WarpToSeat, Heavy, Watertight, PopDoor, SkidMarks,
    Particles, DriverTargetable, HeatSeekingTargetable, PetrolTankWeakPoint,
    Siren, TakeLessDamage, Paintjob, AddUpgrade, RemoveUpgrade, RemoveAllUpgrades, Freeze, Delete,
    EnterVehicle, Count
};

enum class Parameter { None, Number, Toggle, Colors, Weapon, Door, Seat };

struct ActionInfo {
    Action action = Action::None;
    const char* labelKey = "";
    Parameter parameter = Parameter::None;
    float minimum = 0.0f;
    float maximum = 0.0f;
    float defaultValue = 0.0f;
    bool supported = false;
};

struct Binding {
    Action action = Action::None;
    float value = 0.0f;
    int secondary = 0;
    int tertiary = 0;
    int quaternary = 0;
    bool enabled = true;
};

constexpr std::size_t SlotCount = 6;
using Menu = std::array<Binding, SlotCount>;

struct Config {
    bool enabled = false;
    bool drawLinks = true;
    bool includePeds = true;
    bool includeVehicles = true;
    // 看门狗式锁定：保持游戏鼠标与镜头输入，按准星方向的鼠标中键锁定目标。
    bool mouseSelect = true;
    float radius = 80.0f;
    float hitRadius = 160.0f;
    int maxTargets = 16;
    Menu pedMenu{{
        {Action::Restore}, {Action::Armour, 100.0f}, {Action::Disarm},
        {Action::Kill}, {Action::Bring}, {Action::Teleport}
    }};
    Menu vehicleMenu{{
        {Action::Restore}, {Action::Upright}, {Action::Unlock},
        {Action::Ignite}, {Action::Bring}, {Action::Teleport}
    }};
};

struct Target {
    Kind kind = Kind::Ped;
    EntityId id;
    unsigned int modelId = 0;
    Vec3 position;
    float distance = 0.0f;
    float health = 0.0f;
    bool selected = false;
};

struct Labels {
    std::string ped = "NPC";
    std::string vehicle = "VEHICLE";
    std::string heal = "HEAL";
    std::string restore = "RESTORE";
    std::string neutralize = "NEUTRALIZE";
    std::string armour = "ARMOUR";
    std::string disarm = "DISARM";
    std::string upright = "UPRIGHT";
    std::string unlock = "UNLOCK";
    std::string bring = "BRING HERE";
    std::string teleport = "TELEPORT";
    std::array<std::string, static_cast<std::size_t>(Action::Count)> actions;
};

std::vector<ActionInfo> GetActions(Kind kind);
void SetConfig(const Config& config);
Config GetConfig();
void SetLabels(const Labels& labels);
void Process();
void Draw();
void Shutdown();
void NotifyGameInit();

std::vector<Target> GetTargets();
bool Select(Kind kind, EntityId id);
void ClearSelection();
bool GetSelected(Target& target);
bool SetSelectedHealth(float health);
bool DeleteSelected();

} // namespace XBase::Targeting
