#pragma once

namespace XBase {

enum class CapabilitySupport {
    Unsupported,
    Partial,
    Supported,
};

enum class Capability {
    Player,
    Ped,
    Vehicle,
    World,
    Weapon,
    Scene,
    Visual,
    Teleport,
    Overlay,
    BulletAssist,
    Camera,
    Cheats,
    VehicleEffects,
    Hooks,
    Ui,
};

enum class FeatureCapability {
    PlayerBasicState,
    PlayerRuntimeEffects,
    PlayerProofs,
    PlayerMovement,
    PlayerAppearance,
    PlayerClothes,
    PlayerStats,
    PlayerCheats,
    PlayerKeepStuff,
    PlayerSaveGame,
    PedBasic,
    PedSpawn,
    PedDelete,
    PedAttributes,
    PedClassification,
    PedMarkerSpawn,
    PedGlobalStrategies,
    VehicleBasic,
    VehicleColors,
    VehicleDoors,
    VehicleSpawn,
    VehicleSpawnSession,
    VehicleDelete,
    VehicleEvents,
    VehiclePopDoors,
    VehicleSpecialAttributes,
    VehiclePaintjob,
    VehicleUpgrades,
    WorldTime,
    WorldWeather,
    WorldGravity,
    WorldPickups,
    WeaponBasic,
    WeaponGive,
    WeaponDrop,
    WeaponSkills,
    WeaponRuntimeEffects,
    BulletAssistFireSuppression,
    CameraFreecam,
    CameraTopDown,
    CheatsRandom,
    VehicleEffectsNeon,
    SceneAnimation,
    SceneParticle,
    SceneCutscene,
    SceneMission,
    TeleportBasic,
    VisualHudRadar,
    VisualFilter,
};

// Domain queries are retained for compatibility. New integrations should use
// feature queries so a partially implemented backend is never over-reported.
CapabilitySupport GetCapabilitySupport(Capability capability);
CapabilitySupport GetCapabilitySupport(FeatureCapability capability);
bool HasCapability(Capability capability);
bool HasCapability(FeatureCapability capability);

} // namespace XBase