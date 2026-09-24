#include "UiSchema.h"

#include "controllers/Ped.h"
#include "controllers/Player.h"
#include "controllers/ReactUi.h"
#include "controllers/Scene.h"
#include "controllers/Vehicle.h"
#include "controllers/Weapon.h"
#include "controllers/World.h"
#include "integration/XBaseBridge.h"
#include "ui/GuiTheme.h"
#include "ui/Menu.h"
#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"
#include "utils/Log.h"

#include <XBase/Capabilities.h>
#include <XBase/Hooks.h>
#include <XBase/Json.h>
#include <XBase/Platform.h>
#include <XBase/UI.h>
#include <XBase/Visual.h>
#include <XBase/WebView.h>

#include <unordered_map>
#include <variant>
#include <vector>

namespace UiSchema {
namespace {

using Value = XBase::Json::Value;

Value s_schema;
bool s_loaded = false;

const char* CurrentGame() {
#if defined(GTASA)
    return "sa";
#elif defined(GTAVC)
    return "vc";
#elif defined(GTA3)
    return "iii";
#else
    return "";
#endif
}

std::size_t ArraySize(const Value& value) {
    if (!value.IsArray()) return 0;
    return std::get<std::vector<Value>>(value.data).size();
}

const Value& FindById(const Value& list, const char* id) {
    static const Value empty;
    if (!list.IsArray()) return empty;
    for (std::size_t index = 0; index < ArraySize(list); ++index) {
        const Value& item = list[index];
        if (item["id"].AsString() == id) return item;
    }
    return empty;
}

bool* BoolState(const std::string& name) {
    static const std::unordered_map<std::string, bool*> map = {
        {"VehicleNoDamage", &MenuState::VehicleNoDamage},
        {"VehicleAutoUnflip", &MenuState::VehicleAutoUnflip},
        {"VehicleHeavy", &MenuState::VehicleHeavy},
        {"VehicleWatertight", &MenuState::VehicleWatertight},
        {"VehicleFlyingCars", &MenuState::VehicleFlyingCars},
        {"VehicleBoatFly", &MenuState::VehicleBoatFly},
        {"VehicleDriveWater", &MenuState::VehicleDriveWater},
        {"VehicleGreenLights", &MenuState::VehicleGreenLights},
        {"VehiclePerfectHandling", &MenuState::VehiclePerfectHandling},
        {"VehicleBikeFly", &MenuState::VehicleBikeFly},
        {"VehicleStayOnBike", &MenuState::VehicleStayOnBike},
        {"VehicleTankMode", &MenuState::VehicleTankMode},
        {"VehicleAimDrive", &MenuState::VehicleAimDrive},
        {"VehicleNoDerail", &MenuState::VehicleNoDerail},
        {"VehicleFlipNoBurn", &MenuState::VehicleFlipNoBurn},
        {"VehicleInfNitro", &MenuState::VehicleInfNitro},
        {"VehicleNeon", &MenuState::VehicleNeon},
        {"VehicleAutoDrive", &MenuState::VehicleAutoDrive},
        {"VehicleSpeedLock", &MenuState::VehicleSpeedLock},
        {"VehicleSpawnAsDriver", &MenuState::VehicleSpawnAsDriver},
        {"VehicleSpawnAircraftInAir", &MenuState::VehicleSpawnAircraftInAir},
        {"VehicleCleanupAfterSpawn", &MenuState::VehicleCleanupAfterSpawn},
        {"WorldLockTime", &MenuState::WorldLockTime},
        {"DisableReplay", &MenuState::DisableReplay},
        {"DisableCheats", &MenuState::DisableCheats},
        {"FasterClock", &MenuState::FasterClock},
        {"FreezeTime", &MenuState::FreezeTime},
        {"ForbiddenAreaWanted", &MenuState::ForbiddenAreaWanted},
        {"FreePayNSpray", &MenuState::FreePayNSpray},
        {"NoWaterPhysics", &MenuState::NoWaterPhysics},
        {"GodMode", &MenuState::GodMode},
        {"AutoHeal", &MenuState::AutoHeal},
        {"HardMode", &MenuState::HardMode},
        {"InfiniteSprint", &MenuState::InfiniteSprint},
        {"RespawnAtDeathPosition", &MenuState::RespawnAtDeathPosition},
        {"FreezeWantedLevel", &MenuState::FreezeWantedLevel},
        {"KeepStuff", &MenuState::KeepStuff},
        {"FreeFlyEnabled", &MenuState::FreeFlyEnabled},
        {"NeverWanted", &MenuState::NeverWanted},
        {"MegaJump", &MenuState::MegaJump},
        {"MegaPunch", &MenuState::MegaPunch},
        {"CycleJump", &MenuState::CycleJump},
        {"InfiniteOxygen", &MenuState::InfiniteOxygen},
        {"NeverHungry", &MenuState::NeverHungry},
        {"FastSprint", &MenuState::FastSprint},
        {"DrunkEffect", &MenuState::DrunkEffect},
        {"SprintEverywhere", &MenuState::SprintEverywhere},
        {"InvisiblePlayer", &MenuState::InvisiblePlayer},
        {"BigHeadMode", &MenuState::BigHeadMode},
        {"ThinBodyMode", &MenuState::ThinBodyMode},
        {"ElvisEverywhere", &MenuState::ElvisEverywhere},
        {"EveryoneArmed", &MenuState::EveryoneArmed},
        {"PedsMayhem", &MenuState::PedsMayhem},
        {"PedsAtkRocket", &MenuState::PedsAtkRocket},
        {"PedsRiot", &MenuState::PedsRiot},
        {"SlutMagnet", &MenuState::SlutMagnet},
        {"GangsControl", &MenuState::GangsControl},
        {"GangsEverywhere", &MenuState::GangsEverywhere},
        {"PedNoProstitutes", &MenuState::PedNoProstitutes},
        {"PedNastyLimbs", &MenuState::PedNastyLimbs},
        {"PedsNoFire", &MenuState::PedsNoFire},
        {"PedsNoFireCivilians", &MenuState::PedsNoFireCivilians},
        {"PedsNoFireGangs", &MenuState::PedsNoFireGangs},
        {"PedsNoFirePolice", &MenuState::PedsNoFirePolice},
        {"PedsNoFireMission", &MenuState::PedsNoFireMission},
        {"PedsLimitPolice", &MenuState::PedsLimitPolice},
        {"PedsLimitGangs", &MenuState::PedsLimitGangs},
        {"InfiniteAmmo", &MenuState::InfiniteAmmo},
        {"FastReload", &MenuState::FastReload},
        {"HugeWeaponDamage", &MenuState::HugeWeaponDamage},
        {"LongWeaponRange", &MenuState::LongWeaponRange},
        {"WeaponAutoAim", &MenuState::WeaponAutoAim},
        {"MoveAim", &MenuState::MoveAim},
        {"MoveFire", &MenuState::MoveFire},
        {"NoSpread", &MenuState::NoSpread},
        {"RapidFire", &MenuState::RapidFire},
        {"DualWield", &MenuState::DualWield},
        {"WeaponFireRateEnabled", &MenuState::WeaponFireRateEnabled},
        {"WeaponPedEsp", &MenuState::WeaponPedEsp},
        {"WeaponPedColEsp", &MenuState::WeaponPedColEsp},
        {"WeaponPedSkeleton", &MenuState::WeaponPedSkeleton},
        {"WeaponVehicleEsp", &MenuState::WeaponVehicleEsp},
        {"WeaponVehicleColEsp", &MenuState::WeaponVehicleColEsp},
        {"WeaponBulletTrack", &MenuState::WeaponBulletTrack},
        {"WeaponBulletThroughWalls", &MenuState::WeaponBulletThroughWalls},
        {"WeaponTrackCivilian", &MenuState::WeaponTrackCivilian},
        {"WeaponTrackFriend", &MenuState::WeaponTrackFriend},
        {"WeaponTrackHostile", &MenuState::WeaponTrackHostile},
        {"WeaponTrackNeutral", &MenuState::WeaponTrackNeutral},
        {"WeaponBulletHardLock", &MenuState::WeaponBulletHardLock},
        {"WeaponSafeMode", &MenuState::WeaponSafeMode},
        {"WeaponCyclerEnabled", &MenuState::WeaponCyclerEnabled},
        {"VisualHud", &MenuState::VisualHud},
        {"VisualRadar", &MenuState::VisualRadar},
        {"VisualSquareRadar", &MenuState::VisualSquareRadar},
        {"VisualNoRadarRot", &MenuState::VisualNoRadarRot},
        {"VisualFullscreenMap", &MenuState::VisualFullscreenMap},
        {"VisualUnfogMap", &MenuState::VisualUnfogMap},
        {"VisualHideAreaNames", &MenuState::VisualHideAreaNames},
        {"VisualHideVehicleNames", &MenuState::VisualHideVehicleNames},
        {"VisualNightVision", &MenuState::VisualNightVision},
        {"VisualInfrared", &MenuState::VisualInfrared},
        {"VisualFilter", &MenuState::VisualFilter},
        {"QuickTeleport", &MenuState::QuickTeleport},
        {"SpawnUnderwater", &MenuState::SpawnUnderwater},
        {"TeleportMarker", &MenuState::TeleportMarker},
        {"TeleportForwardHold", &MenuState::TeleportForwardHold},
        {"SceneAnimLoop", &MenuState::SceneAnimLoop},
        {"SceneAnimSecondary", &MenuState::SceneAnimSecondary},
        {"SceneAnimOnPed", &MenuState::SceneAnimOnPed},
        {"PedSpawnAsGang", &MenuState::PedSpawnAsGang},
        {"PedFreeze", &MenuState::PedFreeze},
        {"PedHostile", &MenuState::PedHostile},
        {"SmokingEffect", &MenuState::SmokingEffect},
        {"FliesEffect", &MenuState::FliesEffect},
        {"GangWarsActive", &MenuState::GangWarsActive},
        {"CommandWindowEnabled", &MenuState::CommandWindowEnabled},
        {"OverlayEnabled", &MenuState::OverlayEnabled},
        {"OverlayShowDetails", &MenuState::OverlayShowDetails},
        {"OverlayShowFeatures", &MenuState::OverlayShowFeatures},
        {"OverlayShowPosition", &MenuState::OverlayShowPosition},
        {"OverlayShowPlayer", &MenuState::OverlayShowPlayer},
        {"OverlayShowVehicle", &MenuState::OverlayShowVehicle},
        {"OverlayShowTime", &MenuState::OverlayShowTime},
        {"OverlayShowWorld", &MenuState::OverlayShowWorld},
        {"OverlayShowFps", &MenuState::OverlayShowFps},
        {"UseNativeMenu", &MenuState::UseNativeMenu},
        {"ListMenuMouseInput", &MenuState::ListMenuMouseInput},
    };
    const auto found = map.find(name);
    return found == map.end() ? nullptr : found->second;
}

float* FloatState(const std::string& name) {
    static const std::unordered_map<std::string, float*> map = {
        {"VehicleSpeed", &MenuState::VehicleSpeed},
        {"VehicleAutoDriveSpeed", &MenuState::VehicleAutoDriveSpeed},
        {"VehicleTrafficDensity", &MenuState::VehicleTrafficDensity},
        {"FreeFlySpeed", &MenuState::FreeFlySpeed},
        {"WeaponFireRate", &MenuState::WeaponFireRate},
        {"WeaponBulletLockRange", &MenuState::WeaponBulletLockRange},
        {"TeleportForwardDistance", &MenuState::TeleportForwardDistance},
        {"WebViewZoom", &MenuState::WebViewZoom},
    };
    const auto found = map.find(name);
    return found == map.end() ? nullptr : found->second;
}

int* IntState(const std::string& name) {
    static const std::unordered_map<std::string, int*> map = {
        {"VehicleNeonColorR", &MenuState::VehicleNeonColorR},
        {"VehicleNeonColorG", &MenuState::VehicleNeonColorG},
        {"VehicleNeonColorB", &MenuState::VehicleNeonColorB},
        {"PedsMaxNearbyPolice", &MenuState::PedsMaxNearbyPolice},
        {"PedsMaxNearbyGangs", &MenuState::PedsMaxNearbyGangs},
        {"WeaponBulletMaxTargets", &MenuState::WeaponBulletMaxTargets},
        {"SceneFightStyle", &MenuState::SceneFightStyle},
        {"SceneWalkStyle", &MenuState::SceneWalkStyle},
    };
    const auto found = map.find(name);
    return found == map.end() ? nullptr : found->second;
}

bool CapabilityUsable(const std::string& name) {
    // 网页视图这类是域级能力，没有对应的特性枚举，单独走一张表
    static const std::unordered_map<std::string, XBase::Capability> domains = {
        {"WebView", XBase::Capability::WebView},
    };
    const auto domain = domains.find(name);
    if (domain != domains.end()) {
        return XBase::HasCapability(domain->second);
    }

    static const std::unordered_map<std::string, XBase::FeatureCapability> map = {
        {"VehicleBasic", XBase::FeatureCapability::VehicleBasic},
        {"VehicleCheats", XBase::FeatureCapability::VehicleCheats},
        {"VehicleEffectsNeon", XBase::FeatureCapability::VehicleEffectsNeon},
        {"VehicleAutoDrive", XBase::FeatureCapability::VehicleAutoDrive},
        {"VehicleTrafficDensity", XBase::FeatureCapability::VehicleTrafficDensity},
        {"VehicleTakeLessDamage", XBase::FeatureCapability::VehicleTakeLessDamage},
        {"VehicleAlwaysSkidMarks", XBase::FeatureCapability::VehicleAlwaysSkidMarks},
        {"VehicleDisableParticles", XBase::FeatureCapability::VehicleDisableParticles},
        {"VehicleDriverTargetable", XBase::FeatureCapability::VehicleDriverTargetable},
        {"VehicleHeatSeekingTargetable", XBase::FeatureCapability::VehicleHeatSeekingTargetable},
        {"VehiclePetrolTankWeakPoint", XBase::FeatureCapability::VehiclePetrolTankWeakPoint},
        {"VehicleSirenOrAlarm", XBase::FeatureCapability::VehicleSirenOrAlarm},
        {"WorldDisableReplay", XBase::FeatureCapability::WorldDisableReplay},
        {"WorldDisableCheats", XBase::FeatureCapability::WorldDisableCheats},
        {"WorldFasterClock", XBase::FeatureCapability::WorldFasterClock},
        {"WorldFreezeTime", XBase::FeatureCapability::WorldFreezeTime},
        {"WorldForbiddenAreaWanted", XBase::FeatureCapability::WorldForbiddenAreaWanted},
        {"WorldFreePayNSpray", XBase::FeatureCapability::WorldFreePayNSpray},
        {"WorldNoWaterPhysics", XBase::FeatureCapability::WorldNoWaterPhysics},
        {"PlayerKeepStuff", XBase::FeatureCapability::PlayerKeepStuff},
        {"PlayerSaveGame", XBase::FeatureCapability::PlayerSaveGame},
        {"PlayerRuntimeEffects", XBase::FeatureCapability::PlayerRuntimeEffects},
        {"PlayerNeverWanted", XBase::FeatureCapability::PlayerNeverWanted},
        {"PlayerSuperJump", XBase::FeatureCapability::PlayerSuperJump},
        {"PlayerSuperPunch", XBase::FeatureCapability::PlayerSuperPunch},
        {"PlayerCycleJump", XBase::FeatureCapability::PlayerCycleJump},
        {"PlayerUnderwaterBreathing", XBase::FeatureCapability::PlayerUnderwaterBreathing},
        {"PlayerNeverHungry", XBase::FeatureCapability::PlayerNeverHungry},
        {"PlayerFastSprint", XBase::FeatureCapability::PlayerFastSprint},
        {"PlayerSprintEverywhere", XBase::FeatureCapability::PlayerSprintEverywhere},
        {"PlayerDrunkEffect", XBase::FeatureCapability::PlayerDrunkEffect},
        {"PedBigHead", XBase::FeatureCapability::PedBigHead},
        {"PedThinBody", XBase::FeatureCapability::PedThinBody},
        {"PedSmokeFlies", XBase::FeatureCapability::PedSmokeFlies},
        {"BulletAssistFireSuppression", XBase::FeatureCapability::BulletAssistFireSuppression},
        {"WeaponRuntimeEffects", XBase::FeatureCapability::WeaponRuntimeEffects},
        {"WeaponStatOverrides", XBase::FeatureCapability::WeaponStatOverrides},
        {"BulletAssistTracking", XBase::FeatureCapability::BulletAssistTracking},
        {"BulletAssistThroughWalls", XBase::FeatureCapability::BulletAssistThroughWalls},
        {"BulletAssistHardLock", XBase::FeatureCapability::BulletAssistHardLock},
        {"BulletAssistPedBounds", XBase::FeatureCapability::BulletAssistPedBounds},
        {"BulletAssistPedCollision", XBase::FeatureCapability::BulletAssistPedCollision},
        {"BulletAssistPedSkeleton", XBase::FeatureCapability::BulletAssistPedSkeleton},
        {"BulletAssistVehicleBounds", XBase::FeatureCapability::BulletAssistVehicleBounds},
        {"BulletAssistVehicleCollision", XBase::FeatureCapability::BulletAssistVehicleCollision},
        {"VisualHudRadar", XBase::FeatureCapability::VisualHudRadar},
        {"VisualFilter", XBase::FeatureCapability::VisualFilter},
        {"VisualRadarOptions", XBase::FeatureCapability::VisualRadarOptions},
        {"SceneAnimation", XBase::FeatureCapability::SceneAnimation},
    };
    const auto found = map.find(name);
    if (found == map.end()) return true;
    return XBaseBridge::HasCapability(found->second);
}

struct LiveControl {
    std::function<bool()> readToggle;
    std::function<void(bool)> writeToggle;
    std::function<float()> readNumber;
    std::function<void(float)> writeNumber;
};

struct SelectSource {
    std::function<std::vector<SelectOption>()> options;
    std::function<std::string()> read;
    std::function<void(const std::string&)> write;
};

std::unordered_map<std::string, SelectSource>& SelectSources() {
    static std::unordered_map<std::string, SelectSource> sources;
    return sources;
}

std::unordered_map<std::string, LiveControl>& LiveControls() {
    static std::unordered_map<std::string, LiveControl> controls;
    return controls;
}

// 变更回调按名字派发，目前只有车速一处用到
void RunChange(const std::string& name) {
    if (name == "ApplySpeedLock") {
        Controllers::Vehicle::ApplySpeedLock();
    } else if (name == "World.LockTime") {
        Controllers::World::SetLockTime(MenuState::WorldLockTime);
    } else if (name == "World.DisableReplay") {
        Controllers::World::SetDisableReplay(MenuState::DisableReplay);
    } else if (name == "World.DisableCheats") {
        Controllers::World::SetDisableCheats(MenuState::DisableCheats);
    } else if (name == "World.FasterClock") {
        Controllers::World::SetFasterClock(MenuState::FasterClock);
    } else if (name == "World.FreezeTime") {
        Controllers::World::SetFreezeTime(MenuState::FreezeTime);
    } else if (name == "World.ForbiddenAreaWanted") {
        Controllers::World::SetForbiddenAreaWanted(MenuState::ForbiddenAreaWanted);
    } else if (name == "World.FreePayNSpray") {
        Controllers::World::SetFreePayNSpray(MenuState::FreePayNSpray);
    } else if (name == "World.NoWaterPhysics") {
        Controllers::World::SetNoWaterPhysics(MenuState::NoWaterPhysics);
    } else if (name == "Ped.BigHead") {
        Controllers::Ped::SetBigHead(MenuState::BigHeadMode);
    } else if (name == "Ped.ThinBody") {
        Controllers::Ped::SetThinBody(MenuState::ThinBodyMode);
    } else if (name == "Ped.Elvis") {
        Controllers::Ped::SetElvisEverywhere(MenuState::ElvisEverywhere);
    } else if (name == "Ped.EveryoneArmed") {
        Controllers::Ped::SetEveryoneArmed(MenuState::EveryoneArmed);
    } else if (name == "Ped.Mayhem") {
        Controllers::Ped::SetPedsMayhem(MenuState::PedsMayhem);
    } else if (name == "Ped.AtkRocket") {
        Controllers::Ped::SetPedsAtkRocket(MenuState::PedsAtkRocket);
    } else if (name == "Ped.Riot") {
        Controllers::Ped::SetPedsRiot(MenuState::PedsRiot);
    } else if (name == "Ped.SlutMagnet") {
        Controllers::Ped::SetSlutMagnet(MenuState::SlutMagnet);
    } else if (name == "Ped.GangsControl") {
        Controllers::Ped::SetGangsControl(MenuState::GangsControl);
    } else if (name == "Ped.GangsEverywhere") {
        Controllers::Ped::SetGangsEverywhere(MenuState::GangsEverywhere);
    } else if (name == "Ped.NoProstitutes") {
        Controllers::Ped::SetNoProstitutes(MenuState::PedNoProstitutes);
    } else if (name == "Ped.NastyLimbs") {
        Controllers::Ped::SetNastyLimbs(MenuState::PedNastyLimbs);
    } else if (name == "Ped.GangWars") {
        Controllers::Ped::SetGangWarsActive(MenuState::GangWarsActive);
    } else if (name == "Ped.NoFire") {
        Controllers::Ped::SetPedsNoFire(MenuState::PedsNoFire);
    } else if (name == "AppConfig.Save") {
        // 传送与叠加层这类选项是纯配置，改完立刻落盘，与 ImGui 里保存配置的行为一致
        AppConfig::Save();
    } else if (name == "Menu.SyncGuiTheme") {
        // 换成列表界面要立刻换皮，列表里的选中项也一并复位，否则会停在上一页的序号上
        GuiTheme::Sync();
        Menu::NotifySurfaceChanged();
        AppConfig::Save();
    } else if (name == "Web.SetZoom") {
        XBase::WebView::SetZoom(MenuState::WebViewZoom);
    } else if (name == "Visual.DisplayHud") {
        XBase::Visual::DisplayHud(MenuState::VisualHud);
    } else if (name == "Visual.DisplayRadar") {
        XBase::Visual::DisplayRadar(MenuState::VisualRadar);
    } else if (name == "Weapon.ResetStats") {
        // 属性改写改完必须重刷一次武器数据，否则这一帧的改动不会生效
        Controllers::Weapon::ResetStats();
    } else if (name == "Ped.SpawnLimits") {
        // 刷新限制是一次性整组下发，四个值必须一起送，分次调用后面的会覆盖前面的
        Controllers::Ped::SetSpawnLimits(
            MenuState::PedsLimitPolice,
            MenuState::PedsLimitGangs,
            MenuState::PedsMaxNearbyPolice,
            MenuState::PedsMaxNearbyGangs);
    }
}

void RunAction(const std::string& id) {
    if (id == "vehicle.applyTargetSpeed") {
        Controllers::Vehicle::ApplyTargetSpeed();
    } else if (id == "vehicle.restoreDefaultSpeed") {
        Controllers::Vehicle::RestoreDefaultTargetSpeed();
    } else if (id == "world.syncClock") {
        Controllers::World::SyncTimeWithSystemClock();
    } else if (id == "player.copyCoordinates") {
        Controllers::Player::CopyCoordinates();
    } else if (id == "player.heal") {
        Controllers::Player::Heal();
    } else if (id == "player.armour") {
        Controllers::Player::GiveArmour();
    } else if (id == "player.saveGame") {
        // 存档会唤起游戏自己的存档界面，不能从界面线程直接触发，挂到游戏线程那一帧执行
        Controllers::ReactUi::QueueSaveGame();
    } else if (id == "player.addMoney") {
        Controllers::Player::GiveMoney();
    } else if (id == "player.kill") {
        Controllers::Player::Kill();
    } else if (id == "weapon.giveAll") {
        Controllers::Weapon::GiveAll();
    } else if (id == "ped.startGangWar") {
        Controllers::Ped::StartGangWar(true);
    } else if (id == "ped.endGangWar") {
        Controllers::Ped::EndGangWar();
    } else if (id == "ped.resetGangModels") {
        Controllers::Ped::ResetGangModels();
    } else if (id == "vehicle.repair") {
        Controllers::Vehicle::Repair();
    } else if (id == "vehicle.stop") {
        Controllers::Vehicle::Stop();
    } else if (id == "vehicle.unflip") {
        Controllers::Vehicle::Unflip();
    } else if (id == "vehicle.start") {
        Controllers::Vehicle::Start();
    } else if (id == "vehicle.engineOn") {
        Controllers::Vehicle::SetEngine(true);
    } else if (id == "vehicle.engineOff") {
        Controllers::Vehicle::SetEngine(false);
    } else if (id == "vehicle.blowUpAll") {
        Controllers::Vehicle::BlowUpAll();
    } else if (id == "scene.applyFightStyle") {
        Controllers::Scene::SetFightingStyle(MenuState::SceneFightStyle);
    } else if (id == "scene.applyWalkStyle") {
        Controllers::Scene::SetWalkingStyle(MenuState::SceneWalkStyle);
    }
}

bool GamesAllowed(const Value& node) {
    const Value& games = node["games"];
    if (!games.IsArray()) return true;
    const std::string current = CurrentGame();
    for (std::size_t index = 0; index < ArraySize(games); ++index) {
        if (games[index].AsString() == current) return true;
    }
    return false;
}

bool Visible(const Value& control, const Value& section) {
    const std::string expression = control["visibleWhen"].AsString();
    if (expression.empty()) return true;

    // 表达式前面带叹号表示反过来，例如「不用列表界面时才显示交互模式」
    const bool inverted = expression[0] == '!';
    const std::string dependsOn = inverted ? expression.substr(1) : expression;

    const Value& controls = section["controls"];
    for (std::size_t index = 0; index < ArraySize(controls); ++index) {
        const Value& other = controls[index];
        if (other["id"].AsString() != dependsOn) continue;
        bool* state = BoolState(other["state"].AsString());
        if (!state) return true;
        return inverted ? !*state : *state;
    }
    return true;
}

void DrawControl(const Value& control, const Value& section, bool inlineRow) {
    const std::string kind = control["kind"].AsString();
    const std::string state = control["state"].AsString();
    const std::string label = control["labelKey"].AsString();

    // 内联分区把控件排成一行放不下就换行，按钮宽度按行内个数折算
    if (inlineRow) {
        UI::SameLine();
    }

    if (kind == "action") {
        if (UI::Button(I18n::T(label.c_str()), static_cast<short>(section["columns"].AsInt(6)))) {
            RunAction(control["id"].AsString());
        }
        return;
    }

    // 没有 MenuState 可挂的控件（血量、灯光、防护这类读实时状态的）走宿主登记的读写函数，
    // 读到的就是当前真实状态，写下去立刻生效
    const auto live = LiveControls().find(control["id"].AsString());
    const bool hasLive = live != LiveControls().end();

    if (kind == "toggle") {
        if (bool* value = BoolState(state)) {
            if (UI::Checkbox(I18n::T(label.c_str()), value)) {
                RunChange(control["onChange"].AsString());
            }
            return;
        }
        if (!hasLive || !live->second.readToggle || !live->second.writeToggle) return;
        bool current = live->second.readToggle();
        if (UI::Checkbox(I18n::T(label.c_str()), &current)) {
            live->second.writeToggle(current);
        }
        return;
    }

    if (kind == "float") {
        if (float* value = FloatState(state)) {
            const std::string format = control["format"].AsString();
            UI::PushItemWidth(200);
            const bool changed = UI::SliderFloat(
                I18n::T(label.c_str()),
                value,
                static_cast<float>(control["min"].AsNumber(0.0)),
                static_cast<float>(control["max"].AsNumber(1.0)),
                format.empty() ? "%.0f" : format.c_str());
            UI::PopItemWidth();
            if (changed) RunChange(control["onChange"].AsString());
            return;
        }
        if (!hasLive || !live->second.readNumber || !live->second.writeNumber) return;
        float current = live->second.readNumber();
        const std::string format = control["format"].AsString();
        UI::PushItemWidth(200);
        const bool changed = UI::SliderFloat(
            I18n::T(label.c_str()),
            &current,
            static_cast<float>(control["min"].AsNumber(0.0)),
            static_cast<float>(control["max"].AsNumber(1.0)),
            format.empty() ? "%.0f" : format.c_str());
        UI::PopItemWidth();
        if (changed) live->second.writeNumber(current);
        return;
    }

    if (kind == "int") {
        int* value = IntState(state);
        if (!value) return;
        UI::PushItemWidth(200);
        const bool changed = UI::SliderInt(
            I18n::T(label.c_str()),
            value,
            control["min"].AsInt(0),
            control["max"].AsInt(255));
        UI::PopItemWidth();
        if (changed) RunChange(control["onChange"].AsString());
        return;
    }

    // 下拉选择：选项与当前值都从宿主登记的来源取，改完立刻写回去
    if (kind == "select") {
        const auto source = SelectSources().find(control["source"].AsString());
        if (source == SelectSources().end() || !source->second.options || !source->second.read) return;

        const std::vector<SelectOption> options = source->second.options();
        const std::string current = source->second.read();
        std::string preview;
        for (const SelectOption& option : options) {
            if (option.value != current) continue;
            preview = option.translated ? I18n::T(option.label.c_str()) : option.label;
            break;
        }

        XBase::UI::Text(I18n::T(label.c_str()));
        const std::string comboId = "##" + control["id"].AsString();
        XBase::UI::Combo(comboId.c_str(), preview.c_str(), [&] {
            for (const SelectOption& option : options) {
                const std::string text = option.translated ? I18n::T(option.label.c_str()) : option.label;
                const bool selected = option.value == current;
                if (XBase::UI::Selectable(text.c_str(), selected)) {
                    source->second.write(option.value);
                }
                if (selected) XBase::UI::FocusLastItemByDefault();
            }
        });
    }
}

} // namespace

const Value& Schema() {
    Load();
    return s_schema;
}

namespace {
    // 控件按 id 线性找一次，只在网页端取初值与点击时调用，不在每帧路径上
    const Value* FindControl(const char* id) {
        if (!id || !*id) return nullptr;
        const Value& tabs = s_schema["tabs"];
        for (std::size_t tabIndex = 0; tabIndex < ArraySize(tabs); ++tabIndex) {
            const Value& pages = tabs[tabIndex]["pages"];
            for (std::size_t pageIndex = 0; pageIndex < ArraySize(pages); ++pageIndex) {
                const Value& sections = pages[pageIndex]["sections"];
                for (std::size_t sectionIndex = 0; sectionIndex < ArraySize(sections); ++sectionIndex) {
                    const Value& controls = sections[sectionIndex]["controls"];
                    for (std::size_t controlIndex = 0; controlIndex < ArraySize(controls); ++controlIndex) {
                        if (controls[controlIndex]["id"].AsString() == id) {
                            return &controls[controlIndex];
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    double ClampNumber(const Value& control, double raw) {
        const double low = control["min"].AsNumber(0.0);
        const double high = control["max"].AsNumber(0.0);
        if (raw < low) return low;
        if (high > low && raw > high) return high;
        return raw;
    }
}

XBase::Json::Value ControlValue(const char* id) {
    Load();
    const Value* control = FindControl(id);
    if (!control) return XBase::Json::Value();

    const std::string kind = (*control)["kind"].AsString();
    const std::string stateName = (*control)["state"].AsString();

    if (kind == "toggle") {
        if (bool* state = BoolState(stateName)) {
            return XBase::Json::Value(*state);
        }
        const auto live = LiveControls().find(id);
        if (live != LiveControls().end() && live->second.readToggle) {
            return XBase::Json::Value(live->second.readToggle());
        }
        return XBase::Json::Value();
    }

    if (kind == "float" || kind == "int") {
        if (kind == "float") {
            if (float* state = FloatState(stateName)) {
                return XBase::Json::Value(static_cast<double>(*state));
            }
        } else if (int* state = IntState(stateName)) {
            return XBase::Json::Value(*state);
        }
        const auto live = LiveControls().find(id);
        if (live != LiveControls().end() && live->second.readNumber) {
            return XBase::Json::Value(static_cast<double>(live->second.readNumber()));
        }
    }

    if (kind == "select") {
        const auto source = SelectSources().find((*control)["source"].AsString());
        if (source != SelectSources().end() && source->second.read) {
            return XBase::Json::Value(source->second.read());
        }
    }

    return XBase::Json::Value();
}

XBase::Json::Value ControlOptions(const char* id) {
    Load();
    const Value* control = FindControl(id);
    if (!control || (*control)["kind"].AsString() != "select") return XBase::Json::Value();

    const auto source = SelectSources().find((*control)["source"].AsString());
    if (source == SelectSources().end() || !source->second.options) return XBase::Json::Value();

    XBase::Json::Value items;
    for (const SelectOption& option : source->second.options()) {
        XBase::Json::Value item;
        item.Set("value", XBase::Json::Value(option.value));
        item.Set("label", XBase::Json::Value(option.label));
        item.Set("translated", XBase::Json::Value(option.translated));
        items.Push(item);
    }

    XBase::Json::Value result;
    result.Set("items", items);
    result.Set("value", ControlValue(id));
    return result;
}

bool SetControlValue(const char* id, const XBase::Json::Value& value) {
    Load();
    const Value* control = FindControl(id);
    if (!control || value.IsNull()) return false;

    const std::string kind = (*control)["kind"].AsString();
    const std::string stateName = (*control)["state"].AsString();
    const std::string changeName = (*control)["onChange"].AsString();

    if (kind == "toggle") {
        const bool next = value.AsBool();
        if (bool* state = BoolState(stateName)) {
            *state = next;
            RunChange(changeName);
            return true;
        }
        const auto live = LiveControls().find(id);
        if (live != LiveControls().end() && live->second.writeToggle) {
            live->second.writeToggle(next);
            return true;
        }
        return false;
    }

    if (kind == "select") {
        const auto source = SelectSources().find((*control)["source"].AsString());
        if (source == SelectSources().end() || !source->second.write) return false;
        source->second.write(value.AsString());
        return true;
    }

    if (kind == "float" || kind == "int") {
        const double clamped = ClampNumber(*control, value.AsNumber(0.0));
        if (kind == "float") {
            if (float* state = FloatState(stateName)) {
                *state = static_cast<float>(clamped);
                RunChange(changeName);
                return true;
            }
        } else if (int* state = IntState(stateName)) {
            *state = static_cast<int>(clamped);
            RunChange(changeName);
            return true;
        }
        const auto live = LiveControls().find(id);
        if (live != LiveControls().end() && live->second.writeNumber) {
            live->second.writeNumber(static_cast<float>(clamped));
            return true;
        }
    }

    return false;
}

bool RunControl(const char* id) {
    Load();
    const Value* control = FindControl(id);
    if (!control || (*control)["kind"].AsString() != "action") return false;
    RunAction(id);
    return true;
}

bool RegisterLiveToggle(const char* id, std::function<bool()> read, std::function<void(bool)> write) {
    if (!id || !read || !write) return false;
    LiveControl& control = LiveControls()[id];
    control.readToggle = std::move(read);
    control.writeToggle = std::move(write);
    return true;
}

bool RegisterSelectSource(const char* name,
                          std::function<std::vector<SelectOption>()> options,
                          std::function<std::string()> read,
                          std::function<void(const std::string&)> write) {
    if (!name || !*name || !options || !read || !write) return false;
    SelectSource& source = SelectSources()[name];
    source.options = std::move(options);
    source.read = std::move(read);
    source.write = std::move(write);
    return true;
}

bool RegisterLiveNumber(const char* id, std::function<float()> read, std::function<void(float)> write) {
    if (!id || !read || !write) return false;
    LiveControl& control = LiveControls()[id];
    control.readNumber = std::move(read);
    control.writeNumber = std::move(write);
    return true;
}

bool HasLiveControl(const char* id) {
    return id && LiveControls().find(id) != LiveControls().end();
}

Value SchemaPayload() {
    Load();

    static const char* const names[] = {
        "VehicleBasic", "VehicleCheats", "VehicleEffectsNeon", "VehicleAutoDrive",
        "VehicleTrafficDensity", "VehicleTakeLessDamage", "VehicleAlwaysSkidMarks",
        "VehicleDisableParticles", "VehicleDriverTargetable", "VehicleHeatSeekingTargetable",
        "VehiclePetrolTankWeakPoint", "VehicleSirenOrAlarm",
        "WorldDisableReplay", "WorldDisableCheats", "WorldFasterClock", "WorldFreezeTime",
        "WorldForbiddenAreaWanted", "WorldFreePayNSpray", "WorldNoWaterPhysics",
        "PlayerKeepStuff", "PlayerSaveGame", "PlayerRuntimeEffects", "PlayerNeverWanted",
        "PlayerSuperJump", "PlayerSuperPunch", "PlayerCycleJump", "PlayerUnderwaterBreathing",
        "PlayerNeverHungry", "PlayerFastSprint", "PlayerSprintEverywhere", "PlayerDrunkEffect",
        "PedBigHead", "PedThinBody", "PedSmokeFlies", "BulletAssistFireSuppression",
        "WeaponRuntimeEffects", "WeaponStatOverrides", "BulletAssistTracking",
        "BulletAssistThroughWalls", "BulletAssistHardLock", "BulletAssistPedBounds",
        "BulletAssistPedCollision", "BulletAssistPedSkeleton", "BulletAssistVehicleBounds",
        "BulletAssistVehicleCollision", "VisualHudRadar", "VisualFilter", "VisualRadarOptions",
        "SceneAnimation", "WebView",
    };

    Value capabilities;
    for (const char* name : names) {
        const bool usable = CapabilityUsable(name);
        capabilities.Set(name, Value(usable ? "supported" : "unsupported"));
    }

    Value payload;
    payload.Set("schema", s_schema);
    payload.Set("capabilities", capabilities);
    return payload;
}

bool Load() {
    if (s_loaded) return !s_schema.IsNull();

    const std::string path = XBase::Platform::ModDirectory("XMenu") + "data\\ui-schema.json";
    s_schema = Value::Load(path);
    s_loaded = true;

    if (s_schema.IsNull()) {
        Log::Error("界面注册表加载失败，退回原生绘制");
        return false;
    }
    return true;
}

bool HasSection(const char* tabId, const char* pageId, const char* sectionId) {
    if (!Load()) return false;
    const Value& tab = FindById(s_schema["tabs"], tabId);
    const Value& page = FindById(tab["pages"], pageId);
    return !FindById(page["sections"], sectionId).IsNull();
}

bool DrawSection(const char* tabId, const char* pageId, const char* sectionId) {
    if (!Load()) return false;

    const Value& tab = FindById(s_schema["tabs"], tabId);
    const Value& page = FindById(tab["pages"], pageId);
    const Value& section = FindById(page["sections"], sectionId);
    if (section.IsNull() || !GamesAllowed(section)) return false;
    // 只对当前载具生效的分区在没有载具时直接算作已接管，交给页面自己提示去坐车
    if (section["requiresVehicle"].AsBool(false) && !Controllers::Vehicle::GetCurrentVehicleId()) return true;

    const Value& controls = section["controls"];
    if (ArraySize(controls) == 0) return false;

    const std::string title = section["labelKey"].AsString();
    if (!title.empty()) {
        XBase::UI::SeparatorText(I18n::T(title.c_str()));
    }

    // 有的分区带一段操作提示，原生菜单已经把按键写进这里，注册表侧同样免不了要显示
    const std::string hint = section["hintKey"].AsString();
    if (!hint.empty() && !MenuState::UseNativeMenu) {
        XBase::UI::TextWrapped(I18n::T(hint.c_str()));
    }

    const bool sectionUsable = CapabilityUsable(section["capability"].AsString());
    const int columns = section["columns"].AsInt(1);
    const bool inlineRow = section["inline"].AsBool(false) && columns > 1;
    if (columns > 1 && !inlineRow) UI::Columns(columns, nullptr, false);

    std::size_t drawn = 0;
    XBase::UI::Disabled(!sectionUsable, [&] {
        for (std::size_t index = 0; index < ArraySize(controls); ++index) {
            const Value& control = controls[index];
            if (!GamesAllowed(control) || !Visible(control, section)) continue;
            if (!CapabilityUsable(control["capability"].AsString())) continue;
            DrawControl(control, section, inlineRow && drawn > 0 && drawn % columns != 0);
            ++drawn;
            if (columns > 1 && !inlineRow) UI::NextColumn();
        }
    });

    if (columns > 1 && !inlineRow) UI::Columns(1);
    // 返回真表示这段分区已由注册表接管，控件被能力或版本筛掉也算接管，
    // 调用方据此跳过原生兜底，避免把注册表故意隐藏的控件又画出来
    return true;
}

}
