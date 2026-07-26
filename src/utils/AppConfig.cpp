#include "AppConfig.h"
#include "game/Runtime.h"
#include "ui/GuiTheme.h"
#include "ui/MenuState.h"
#include "utils/JsonLoader.h"
#include "utils/Log.h"
#include "utils/I18n.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

#ifdef GetObject
#undef GetObject
#endif

extern const char* XMENU_VERSION;
extern const char* XMENU_AUTHOR;
extern const char* XMENU_URL;
extern const char* XMENU_GITHUB;

namespace {
    constexpr const char* DefaultMenuKey = "M";

    std::string menuKeyName = DefaultMenuKey;
    std::string fallbackLanguageCode = "zh";
    AppConfig::Hotkey menuHotkey;
    AppConfig::UpdateCache updateCache;
    std::vector<DataManager::LocationData> customLocations;
    std::vector<AppConfig::ActionHotkey> actionHotkeys;
    std::unordered_map<std::string, bool> persistentEnabled;
    std::unordered_map<std::string, bool> persistentValues;
    bool syncingPersistentState = false;
    ULONGLONG persistentSyncReadyTick = 0;
    ULONGLONG lastPersistentSyncTick = 0;
    constexpr ULONGLONG PersistentStartupDelayMs = 15000;
    constexpr ULONGLONG PersistentWriteCooldownMs = 5000;

    struct PersistentStateBinding {
        const char* id;
        const char* name;
        bool* value;
        bool defaultValue;
    };

    const std::vector<PersistentStateBinding>& PersistentStateBindings() {
        static const std::vector<PersistentStateBinding> bindings = {
            {"player.godMode", "player.godMode", &MenuState::GodMode, false},
            {"player.autoHeal", "player.autoHeal", &MenuState::AutoHeal, false},
            {"player.hardMode", "player.hardMode", &MenuState::HardMode, false},
            {"player.infiniteSprint", "player.infiniteSprint", &MenuState::InfiniteSprint, false},
            {"player.respawnAtDeathPosition", "player.respawnAtDeathPosition", &MenuState::RespawnAtDeathPosition, false},
            {"player.freezeWantedLevel", "player.freezeWantedLevel", &MenuState::FreezeWantedLevel, false},
            {"player.keepStuff", "player.keepStuff", &MenuState::KeepStuff, false},
            {"player.autoFlight", "player.autoFlight", &MenuState::FreeFlyEnabled, false},
            {"player.neverWanted", "player.neverWanted", &MenuState::NeverWanted, false},
            {"player.invisible", "player.invisible", &MenuState::InvisiblePlayer, false},
            {"player.megaJump", "player.megaJump", &MenuState::MegaJump, false},
            {"player.megaPunch", "player.megaPunch", &MenuState::MegaPunch, false},
            {"player.cycleJump", "player.cycleJump", &MenuState::CycleJump, false},
            {"player.infiniteOxygen", "player.infiniteOxygen", &MenuState::InfiniteOxygen, false},
            {"player.neverHungry", "player.neverHungry", &MenuState::NeverHungry, false},
            {"player.fastSprint", "player.fastSprint", &MenuState::FastSprint, false},
            {"player.drunkEffect", "player.drunkEffect", &MenuState::DrunkEffect, false},
            {"player.sprintEverywhere", "player.sprintEverywhere", &MenuState::SprintEverywhere, false},
            {"player.aimSkinChanger", "player.aimSkinChanger", &MenuState::AimSkinChanger, false},
            {"weapon.infiniteAmmo", "weapon.infiniteAmmo", &MenuState::InfiniteAmmo, false},
            {"weapon.fastReload", "weapon.fastReload", &MenuState::FastReload, false},
            {"weapon.hugeDamage", "weapon.hugeDamage", &MenuState::HugeWeaponDamage, false},
            {"weapon.longRange", "weapon.longRange", &MenuState::LongWeaponRange, false},
            {"weapon.rapidFire", "weapon.rapidFire", &MenuState::RapidFire, false},
            {"weapon.fireRate", "weapon.fireRate", &MenuState::WeaponFireRateEnabled, false},
            {"weapon.dualWield", "weapon.dualWield", &MenuState::DualWield, false},
            {"weapon.moveAim", "weapon.moveAim", &MenuState::MoveAim, false},
            {"weapon.moveFire", "weapon.moveFire", &MenuState::MoveFire, false},
            {"weapon.noSpread", "weapon.noSpread", &MenuState::NoSpread, false},
            {"weapon.autoAim", "weapon.autoAim", &MenuState::WeaponAutoAim, false},
            {"weapon.pedEsp", "weapon.pedEsp", &MenuState::WeaponPedEsp, false},
            {"weapon.pedColEsp", "weapon.pedColEsp", &MenuState::WeaponPedColEsp, false},
            {"weapon.pedSkeleton", "weapon.pedSkeleton", &MenuState::WeaponPedSkeleton, false},
            {"weapon.vehicleEsp", "weapon.vehicleEsp", &MenuState::WeaponVehicleEsp, false},
            {"weapon.vehicleColEsp", "weapon.vehicleColEsp", &MenuState::WeaponVehicleColEsp, false},
            {"weapon.bulletTrack", "weapon.bulletTrack", &MenuState::WeaponBulletTrack, false},
            {"weapon.bulletThroughWalls", "weapon.bulletThroughWalls", &MenuState::WeaponBulletThroughWalls, false},
            {"vehicle.noDamage", "vehicle.noDamage", &MenuState::VehicleNoDamage, false},
            {"vehicle.autoUnflip", "vehicle.autoUnflip", &MenuState::VehicleAutoUnflip, false},
            {"vehicle.heavy", "vehicle.heavy", &MenuState::VehicleHeavy, false},
            {"vehicle.watertight", "vehicle.watertight", &MenuState::VehicleWatertight, false},
            {"vehicle.lockSpeed", "vehicle.lockSpeed", &MenuState::VehicleSpeedLock, false},
            {"vehicle.flyingCars", "vehicle.flyingCars", &MenuState::VehicleFlyingCars, false},
            {"vehicle.boatFly", "vehicle.boatFly", &MenuState::VehicleBoatFly, false},
            {"vehicle.bikeFly", "vehicle.bikeFly", &MenuState::VehicleBikeFly, false},
            {"vehicle.stayOnBike", "vehicle.stayOnBike", &MenuState::VehicleStayOnBike, false},
            {"vehicle.driveWater", "vehicle.driveWater", &MenuState::VehicleDriveWater, false},
            {"vehicle.perfectHandling", "vehicle.perfectHandling", &MenuState::VehiclePerfectHandling, false},
            {"vehicle.tankMode", "vehicle.tankMode", &MenuState::VehicleTankMode, false},
            {"vehicle.greenLights", "vehicle.greenLights", &MenuState::VehicleGreenLights, false},
            {"vehicle.aimDrive", "vehicle.aimDrive", &MenuState::VehicleAimDrive, false},
            {"vehicle.noDerail", "vehicle.noDerail", &MenuState::VehicleNoDerail, false},
            {"vehicle.flipNoBurn", "vehicle.flipNoBurn", &MenuState::VehicleFlipNoBurn, false},
            {"vehicle.infNitro", "vehicle.infNitro", &MenuState::VehicleInfNitro, false},
            {"ped.bigHeadMode", "ped.bigHeadMode", &MenuState::BigHeadMode, false},
            {"ped.thinBodyMode", "ped.thinBodyMode", &MenuState::ThinBodyMode, false},
            {"ped.elvisEverywhere", "ped.elvisEverywhere", &MenuState::ElvisEverywhere, false},
            {"ped.everyoneArmed", "ped.everyoneArmed", &MenuState::EveryoneArmed, false},
            {"ped.pedsMayhem", "ped.pedsMayhem", &MenuState::PedsMayhem, false},
            {"ped.pedsAtkRocket", "ped.pedsAtkRocket", &MenuState::PedsAtkRocket, false},
            {"ped.pedsRiot", "ped.pedsRiot", &MenuState::PedsRiot, false},
            {"ped.slutMagnet", "ped.slutMagnet", &MenuState::SlutMagnet, false},
            {"ped.gangsControl", "ped.gangsControl", &MenuState::GangsControl, false},
            {"ped.gangsEverywhere", "ped.gangsEverywhere", &MenuState::GangsEverywhere, false},
            {"ped.noProstitutes", "ped.noProstitutes", &MenuState::PedNoProstitutes, false},
            {"ped.nastyLimbs", "ped.nastyLimbs", &MenuState::PedNastyLimbs, false},
            {"ped.gangWarsActive", "ped.gangWarsActive", &MenuState::GangWarsActive, false},
            {"visual.squareRadar", "visual.squareRadar", &MenuState::VisualSquareRadar, false},
            {"visual.noRadarRot", "visual.noRadarRot", &MenuState::VisualNoRadarRot, false},
            {"visual.fullscreenMap", "visual.fullscreenMap", &MenuState::VisualFullscreenMap, false},
            {"visual.unfogMap", "visual.unfogMap", &MenuState::VisualUnfogMap, false},
            {"visual.hideAreaNames", "visual.hideAreaNames", &MenuState::VisualHideAreaNames, false},
            {"visual.hideVehicleNames", "visual.hideVehicleNames", &MenuState::VisualHideVehicleNames, false},
            {"visual.nightVision", "visual.nightVision", &MenuState::VisualNightVision, false},
            {"visual.infrared", "visual.infrared", &MenuState::VisualInfrared, false},
            {"world.lockCurrentWeather", "world.lockCurrentWeather", &MenuState::LockWeather, false},
            {"world.lockCurrentTime", "world.lockCurrentTime", &MenuState::WorldLockTime, false},
            {"world.solidWater", "world.solidWater", &MenuState::SolidWater, false},
            {"world.noWaterPhysics", "world.noWaterPhysics", &MenuState::NoWaterPhysics, false},
            {"world.disableReplay", "world.disableReplay", &MenuState::DisableReplay, false},
            {"world.disableCheats", "world.disableCheats", &MenuState::DisableCheats, false},
            {"world.disableForbiddenAreaWanted", "world.disableForbiddenAreaWanted", &MenuState::ForbiddenAreaWanted, false},
            {"world.freePayNSpray", "world.freePayNSpray", &MenuState::FreePayNSpray, false},
            {"world.fasterClock", "world.fasterClock", &MenuState::FasterClock, false},
            {"world.freezeTime", "world.freezeTime", &MenuState::FreezeTime, false},
            {"overlay.enabled", "overlay.enabled", &MenuState::OverlayEnabled, false},
            {"overlay.showPosition", "overlay.showPosition", &MenuState::OverlayShowPosition, true},
            {"overlay.showVehicle", "overlay.showVehicle", &MenuState::OverlayShowVehicle, false},
            {"overlay.showFps", "overlay.showFps", &MenuState::OverlayShowFps, true},
            {"overlay.showPlayer", "overlay.showPlayer", &MenuState::OverlayShowPlayer, true},
            {"overlay.showTime", "overlay.showTime", &MenuState::OverlayShowTime, true},
            {"overlay.showWorld", "overlay.showWorld", &MenuState::OverlayShowWorld, false},
            {"overlay.showDetails", "overlay.showDetails", &MenuState::OverlayShowDetails, false},
            {"overlay.showFeatures", "overlay.showFeatures", &MenuState::OverlayShowFeatures, false}
        };
        return bindings;
    }

    bool IsSaRuntime() {
        return GameRuntime::Current().target == GameRuntime::Target::SA;
    }

    bool IsPersistentFeatureSupported(const char* id) {
        if (!id) {
            return false;
        }

        const std::string feature = id;
        if (feature == "weapon.dualWield" || feature == "weapon.moveAim" || feature == "weapon.moveFire"
            || feature == "weapon.autoAim"
            || feature == "world.disableForbiddenAreaWanted" || feature == "world.freePayNSpray"
            || feature == "player.megaJump" || feature == "player.megaPunch" || feature == "player.cycleJump"
            || feature == "player.infiniteOxygen" || feature == "player.neverHungry" || feature == "player.fastSprint"
            || feature == "player.drunkEffect" || feature == "player.sprintEverywhere" || feature == "player.aimSkinChanger"
            || feature == "player.invisible" || feature == "player.neverWanted"
            || feature == "vehicle.bikeFly" || feature == "vehicle.stayOnBike" || feature == "vehicle.tankMode"
            || feature == "vehicle.aimDrive" || feature == "vehicle.noDerail" || feature == "vehicle.flipNoBurn"
            || feature == "vehicle.infNitro"
            || feature == "ped.thinBodyMode" || feature == "ped.gangWarsActive"
            || feature == "ped.elvisEverywhere" || feature == "ped.pedsAtkRocket" || feature == "ped.gangsControl"
            || feature == "ped.gangsEverywhere"
            || feature == "visual.squareRadar" || feature == "visual.noRadarRot" || feature == "visual.fullscreenMap"
            || feature == "visual.unfogMap" || feature == "visual.hideAreaNames" || feature == "visual.hideVehicleNames"
            || feature == "visual.nightVision" || feature == "visual.infrared"
            || feature == "world.solidWater" || feature == "world.noWaterPhysics") {
            return IsSaRuntime();
        }

        if (feature == "weapon.pedEsp" || feature == "weapon.pedColEsp" || feature == "weapon.pedSkeleton"
            || feature == "weapon.vehicleEsp" || feature == "weapon.vehicleColEsp"
            || feature == "weapon.bulletTrack" || feature == "weapon.bulletThroughWalls") {
            return IsSaRuntime() || GameRuntime::Current().target == GameRuntime::Target::VC;
        }

        if (feature == "ped.bigHeadMode") {
            // SA 皮肤缩放 + III pre-render 钩子；VC 无实现
            return IsSaRuntime() || GameRuntime::Current().target == GameRuntime::Target::III;
        }
        if (feature == "ped.noProstitutes") {
            return GameRuntime::Current().target == GameRuntime::Target::VC;
        }
        if (feature == "ped.nastyLimbs") {
            return GameRuntime::Current().target == GameRuntime::Target::III;
        }

        return true;
    }

    bool IsActionHotkeySupportedForRuntime(const char* id) {
        if (!id) {
            return false;
        }

        const std::string action = id;
        if (action == "teleport.marker" || action == "teleport.quickMap"
            || action == "player.aimSkinChanger" || action == "player.neverWanted.toggle") {
            return IsSaRuntime();
        }

        return true;
    }

    const PersistentStateBinding* FindPersistentStateBinding(const std::string& id) {
        const std::vector<PersistentStateBinding>& bindings = PersistentStateBindings();
        for (const PersistentStateBinding& binding : bindings) {
            if (binding.id == id) {
                return &binding;
            }
        }
        return nullptr;
    }

    std::string PersistentFeatureGroup(const char* id) {
        const std::string value = id ? id : "";
        const std::size_t dot = value.find('.');
        return dot == std::string::npos ? "misc" : value.substr(0, dot);
    }

    void ResetPersistentStateToDefaults() {
        persistentEnabled.clear();
        persistentValues.clear();
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }
            persistentEnabled[binding.id] = false;
            persistentValues[binding.id] = binding.defaultValue;
        }
    }

    std::string DirectoryFromModule(HMODULE module) {
        if (!module) {
            return "";
        }

        char path[MAX_PATH] = {};
        const DWORD size = GetModuleFileNameA(module, path, MAX_PATH);
        if (size == 0) {
            return "";
        }

        std::string directory(path, size);
        const std::size_t slash = directory.find_last_of("\\/");
        if (slash != std::string::npos) {
            return directory.substr(0, slash + 1);
        }
        return "";
    }

    std::string XMenuModuleDirectory() {
        const std::string asiDirectory = DirectoryFromModule(GetModuleHandleA("XMenu.asi"));
        if (!asiDirectory.empty()) {
            return asiDirectory;
        }

        HMODULE module = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&XMenuModuleDirectory),
            &module
        );
        return DirectoryFromModule(module);
    }

    bool EnsureDirectory(const std::string& path) {
        if (path.empty()) {
            return false;
        }
        if (CreateDirectoryA(path.c_str(), nullptr)) {
            return true;
        }
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    bool FileExists(const std::string& path) {
        const DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    std::string XMenuDataDirectory() {
        const std::string directory = XMenuModuleDirectory();
        if (!directory.empty()) {
            return directory + "XMenu\\";
        }
        return "XMenu\\";
    }

    std::string ConfigPath() {
        return XMenuDataDirectory() + "config.json";
    }

    std::string LegacyConfigPath() {
        const std::string directory = XMenuModuleDirectory();
        if (!directory.empty()) {
            return directory + "XMenu.json";
        }
        return "XMenu.json";
    }

    std::string ReadConfigPath() {
        const std::string path = ConfigPath();
        if (FileExists(path)) {
            return path;
        }

        const std::string legacyPath = LegacyConfigPath();
        if (FileExists(legacyPath)) {
            return legacyPath;
        }

        return path;
    }

    std::string Trim(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        return value;
    }

    std::string Upper(std::string value) {
        for (char& ch : value) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return value;
    }

    std::vector<std::string> SplitKeyExpression(const std::string& rawValue) {
        std::vector<std::string> parts;
        std::string current;

        for (const char ch : rawValue) {
            if (ch == '+') {
                const std::string part = Trim(current);
                if (!part.empty()) {
                    parts.push_back(part);
                }
                current.clear();
                continue;
            }
            current.push_back(ch);
        }

        const std::string part = Trim(current);
        if (!part.empty()) {
            parts.push_back(part);
        }
        return parts;
    }

    const std::unordered_map<std::string, int>& KeyMap() {
        static const std::unordered_map<std::string, int> keys = {
            {"BACKSPACE", VK_BACK}, {"BACK", VK_BACK},
            {"TAB", VK_TAB},
            {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN},
            {"SHIFT", VK_SHIFT}, {"LSHIFT", VK_LSHIFT}, {"RSHIFT", VK_RSHIFT},
            {"CTRL", VK_CONTROL}, {"CONTROL", VK_CONTROL}, {"LCTRL", VK_LCONTROL}, {"RCTRL", VK_RCONTROL},
            {"ALT", VK_MENU}, {"MENU", VK_MENU}, {"LALT", VK_LMENU}, {"RALT", VK_RMENU},
            {"PAUSE", VK_PAUSE}, {"CAPSLOCK", VK_CAPITAL}, {"CAPS", VK_CAPITAL},
            {"ESC", VK_ESCAPE}, {"ESCAPE", VK_ESCAPE},
            {"SPACE", VK_SPACE}, {"SPACEBAR", VK_SPACE},
            {"PAGEUP", VK_PRIOR}, {"PGUP", VK_PRIOR},
            {"PAGEDOWN", VK_NEXT}, {"PGDN", VK_NEXT},
            {"END", VK_END}, {"HOME", VK_HOME},
            {"LEFT", VK_LEFT}, {"UP", VK_UP}, {"RIGHT", VK_RIGHT}, {"DOWN", VK_DOWN},
            {"PRINTSCREEN", VK_SNAPSHOT}, {"PRTSC", VK_SNAPSHOT},
            {"INSERT", VK_INSERT}, {"INS", VK_INSERT},
            {"DELETE", VK_DELETE}, {"DEL", VK_DELETE},
            {"WIN", VK_LWIN}, {"LWIN", VK_LWIN}, {"RWIN", VK_RWIN},
            {"NUM0", VK_NUMPAD0}, {"NUMPAD0", VK_NUMPAD0},
            {"NUM1", VK_NUMPAD1}, {"NUMPAD1", VK_NUMPAD1},
            {"NUM2", VK_NUMPAD2}, {"NUMPAD2", VK_NUMPAD2},
            {"NUM3", VK_NUMPAD3}, {"NUMPAD3", VK_NUMPAD3},
            {"NUM4", VK_NUMPAD4}, {"NUMPAD4", VK_NUMPAD4},
            {"NUM5", VK_NUMPAD5}, {"NUMPAD5", VK_NUMPAD5},
            {"NUM6", VK_NUMPAD6}, {"NUMPAD6", VK_NUMPAD6},
            {"NUM7", VK_NUMPAD7}, {"NUMPAD7", VK_NUMPAD7},
            {"NUM8", VK_NUMPAD8}, {"NUMPAD8", VK_NUMPAD8},
            {"NUM9", VK_NUMPAD9}, {"NUMPAD9", VK_NUMPAD9},
            {"MULTIPLY", VK_MULTIPLY}, {"NUM*", VK_MULTIPLY},
            {"ADD", VK_ADD}, {"NUM+", VK_ADD},
            {"SUBTRACT", VK_SUBTRACT}, {"NUM-", VK_SUBTRACT},
            {"DECIMAL", VK_DECIMAL}, {"NUM.", VK_DECIMAL},
            {"DIVIDE", VK_DIVIDE}, {"NUM/", VK_DIVIDE},
            {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4},
            {"F5", VK_F5}, {"F6", VK_F6}, {"F7", VK_F7}, {"F8", VK_F8},
            {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
            {"F13", VK_F13}, {"F14", VK_F14}, {"F15", VK_F15}, {"F16", VK_F16},
            {"F17", VK_F17}, {"F18", VK_F18}, {"F19", VK_F19}, {"F20", VK_F20},
            {"F21", VK_F21}, {"F22", VK_F22}, {"F23", VK_F23}, {"F24", VK_F24},
            {"NUMLOCK", VK_NUMLOCK}, {"SCROLLLOCK", VK_SCROLL}, {"SCROLL", VK_SCROLL},
            {";", VK_OEM_1}, {":", VK_OEM_1},
            {"=", VK_OEM_PLUS}, {"PLUS", VK_OEM_PLUS},
            {",", VK_OEM_COMMA}, {"COMMA", VK_OEM_COMMA},
            {"-", VK_OEM_MINUS}, {"MINUS", VK_OEM_MINUS},
            {".", VK_OEM_PERIOD}, {"PERIOD", VK_OEM_PERIOD},
            {"/", VK_OEM_2}, {"SLASH", VK_OEM_2},
            {"`", VK_OEM_3}, {"~", VK_OEM_3}, {"BACKQUOTE", VK_OEM_3},
            {"[", VK_OEM_4}, {"LBRACKET", VK_OEM_4},
            {"\\", VK_OEM_5}, {"BACKSLASH", VK_OEM_5},
            {"]", VK_OEM_6}, {"RBRACKET", VK_OEM_6},
            {"'", VK_OEM_7}, {"QUOTE", VK_OEM_7}
        };
        return keys;
    }

    int KeyNameToVirtualKey(const std::string& rawName) {
        const std::string name = Upper(Trim(rawName));
        if (name.empty()) {
            return 0;
        }

        if (name.size() == 1) {
            const char ch = name[0];
            if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
                return ch;
            }
        }

        const auto mapped = KeyMap().find(name);
        if (mapped != KeyMap().end()) {
            return mapped->second;
        }

        if (name.rfind("VK_", 0) == 0) {
            return KeyNameToVirtualKey(name.substr(3));
        }

        if (name.rfind("0X", 0) == 0) {
            return static_cast<int>(std::strtol(name.c_str(), nullptr, 16));
        }

        bool numeric = true;
        for (const char ch : name) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                numeric = false;
                break;
            }
        }
        if (numeric) {
            return std::atoi(name.c_str());
        }

        return 0;
    }

    bool IsKeyDown(int virtualKey) {
        return virtualKey > 0 && (GetKeyState(virtualKey) & 0x8000) != 0;
    }

    bool IsModifierDown(bool required, int genericKey, int leftKey, int rightKey) {
        if (!required) {
            return true;
        }
        return IsKeyDown(genericKey) || IsKeyDown(leftKey) || IsKeyDown(rightKey);
    }

    std::string CanonicalHotkeyName(const AppConfig::Hotkey& hotkey) {
        if (hotkey.key <= 0) {
            return "None";
        }

        std::vector<std::string> parts;
        if (hotkey.ctrl) {
            parts.push_back("Ctrl");
        }
        if (hotkey.alt) {
            parts.push_back("Alt");
        }
        if (hotkey.shift) {
            parts.push_back("Shift");
        }

        std::string keyName;
        for (const auto& item : KeyMap()) {
            if (item.second == hotkey.key && item.first.size() > keyName.size()) {
                keyName = item.first;
            }
        }

        if (hotkey.key >= 'A' && hotkey.key <= 'Z') {
            keyName = static_cast<char>(hotkey.key);
        } else if (hotkey.key >= '0' && hotkey.key <= '9') {
            keyName = static_cast<char>(hotkey.key);
        } else if (keyName.empty()) {
            keyName = "VK_" + std::to_string(hotkey.key);
        }

        parts.push_back(keyName);

        std::ostringstream stream;
        for (std::size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                stream << "+";
            }
            stream << parts[i];
        }
        return stream.str();
    }

    AppConfig::Hotkey ParseHotkey(const std::string& keyName, bool& valid) {
        AppConfig::Hotkey hotkey;
        valid = false;

        const std::string normalized = Upper(Trim(keyName));
        if (normalized.empty() || normalized == "NONE" || normalized == "DISABLED") {
            hotkey.key = 0;
            valid = true;
            return hotkey;
        }

        const std::vector<std::string> parts = SplitKeyExpression(keyName);
        for (const std::string& rawPart : parts) {
            const std::string part = Upper(Trim(rawPart));
            if (part == "CTRL" || part == "CONTROL" || part == "LCTRL" || part == "RCTRL") {
                hotkey.ctrl = true;
                continue;
            }
            if (part == "ALT" || part == "MENU" || part == "LALT" || part == "RALT") {
                hotkey.alt = true;
                continue;
            }
            if (part == "SHIFT" || part == "LSHIFT" || part == "RSHIFT") {
                hotkey.shift = true;
                continue;
            }

            const int virtualKey = KeyNameToVirtualKey(part);
            if (virtualKey > 0) {
                hotkey.key = virtualKey;
                valid = true;
            }
        }

        return hotkey;
    }

    std::string EscapeJson(const std::string& value) {
        std::string escaped;
        escaped.reserve(value.size());
        for (const char ch : value) {
            switch (ch) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped.push_back(ch); break;
            }
        }
        return escaped;
    }

    const JsonLoader::JsonValue& ObjectOrNull(const JsonLoader::JsonValue& value, const std::string& key) {
        static const JsonLoader::JsonValue empty;
        if (value.type != JsonLoader::JsonValue::OBJECT) {
            return empty;
        }

        const auto found = value.object_values.find(key);
        return found == value.object_values.end() ? empty : found->second;
    }

    void ApplyMenuKey(const std::string& keyName);

    void LoadGuiConfig(const JsonLoader::JsonValue& root) {
        const JsonLoader::JsonValue& menu = ObjectOrNull(root, "menu");
        if (menu.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        const int themeIndex = static_cast<int>(JsonLoader::GetNumber(menu, "guiTheme", 0.0));
        if (themeIndex >= 0 && themeIndex < GuiTheme::ThemeCount) {
            MenuState::GuiThemeIndex = themeIndex;
        }
        GuiTheme::SetThemeByIndex(MenuState::GuiThemeIndex);

        const int interactionMode = static_cast<int>(JsonLoader::GetNumber(menu, "interaction", 0.0));
        if (interactionMode >= 0 && interactionMode < GuiTheme::InteractionCount) {
            MenuState::GuiInteractionMode = interactionMode;
        }
        GuiTheme::SetInteractionByIndex(MenuState::GuiInteractionMode);
        
        MenuState::UseNativeMenu = JsonLoader::GetBool(menu, "UseNativeMenu", MenuState::UseNativeMenu);
        MenuState::ListMenuMouseInput = JsonLoader::GetBool(menu, "ListMenuMouseInput", MenuState::ListMenuMouseInput);
    }

    struct DefaultActionHotkey {
        const char* id;
        const char* name;
        const char* key;
    };

    const DefaultActionHotkey* DefaultActions(std::size_t& count) {
        static const DefaultActionHotkey actions[] = {
            {"teleport.marker", "action.teleport.marker", "X"},
            {"teleport.quickMap", "action.teleport.quickMap", "Shift+Q"},
            {"teleport.forward", "action.teleport.forward", "Shift+X"},
            {"player.freeFly.toggle", "action.player.freeFly.toggle", "NUM0"},
            {"player.autoFlight.hold", "action.player.autoFlight.hold", "None"},
            {"command.toggle", "action.command.toggle", "Ctrl+C"},
            {"overlay.toggle", "action.overlay.toggle", "F7"},
            {"player.heal", "action.player.heal", "F8"},
            {"player.armour", "action.player.armour", "Shift+F8"},
            {"player.clearWanted", "action.player.clearWanted", "Ctrl+F8"},
            {"player.aimSkinChanger", "action.player.aimSkinChanger", "None"},
            {"player.neverWanted.toggle", "action.player.neverWanted.toggle", "None"},
            {"vehicle.repair", "action.vehicle.repair", "F6"},
            {"vehicle.unflip", "action.vehicle.unflip", "Shift+F6"},
            {"vehicle.stop", "action.vehicle.stop", "Ctrl+F6"},
            {"weapon.giveAll", "action.weapon.giveAll", "F9"},
            {"world.toggleFreezeTime", "action.world.toggleFreezeTime", "Ctrl+F9"}
        };
        count = sizeof(actions) / sizeof(actions[0]);
        return actions;
    }

    AppConfig::ActionHotkey* FindActionHotkey(const std::string& actionId) {
        for (AppConfig::ActionHotkey& action : actionHotkeys) {
            if (action.id == actionId) {
                return &action;
            }
        }
        return nullptr;
    }

    void ResetActionHotkeysToDefaults() {
        actionHotkeys.clear();
        std::size_t count = 0;
        const DefaultActionHotkey* defaults = DefaultActions(count);
        for (std::size_t i = 0; i < count; ++i) {
            if (!IsActionHotkeySupportedForRuntime(defaults[i].id)) {
                continue;
            }

            bool valid = false;
            AppConfig::ActionHotkey action;
            action.id = defaults[i].id;
            action.name = defaults[i].name;
            action.hotkey = ParseHotkey(defaults[i].key, valid);
            if (!valid) {
                action.hotkey = AppConfig::Hotkey{};
                action.hotkey.key = 0;
            }
            actionHotkeys.push_back(action);
        }
    }

    void LoadActionHotkeys(const JsonLoader::JsonValue& configRoot) {
        ResetActionHotkeysToDefaults();
        const JsonLoader::JsonValue& actions = ObjectOrNull(configRoot, "actions");
        if (actions.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        for (const auto& item : actions.object_values) {
            AppConfig::ActionHotkey* action = FindActionHotkey(item.first);
            if (!action) {
                continue;
            }

            const std::string keyName = item.second.type == JsonLoader::JsonValue::STRING
                ? item.second.string_value
                : JsonLoader::GetString(item.second, "hotkey", "");
            bool valid = false;
            AppConfig::Hotkey parsed = ParseHotkey(keyName, valid);
            if (valid) {
                action->hotkey = parsed;
            }
        }
    }

    void WriteActionHotkeys(std::ostream& file, bool trailingComma) {
        file << "  \"actions\": {\n";
        for (std::size_t i = 0; i < actionHotkeys.size(); ++i) {
            const AppConfig::ActionHotkey& action = actionHotkeys[i];
            file << "    \"" << EscapeJson(action.id) << "\": \"" << EscapeJson(CanonicalHotkeyName(action.hotkey)) << "\"";
            if (i + 1 < actionHotkeys.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << "  }" << (trailingComma ? "," : "") << "\n";
    }

    void ApplyPersistentStateValues() {
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            const auto enabled = persistentEnabled.find(binding.id);
            if (enabled == persistentEnabled.end() || !enabled->second) {
                continue;
            }

            const auto value = persistentValues.find(binding.id);
            *binding.value = value == persistentValues.end() ? binding.defaultValue : value->second;
        }
    }

    void LoadPersistentState(const JsonLoader::JsonValue& root) {
        ResetPersistentStateToDefaults();
        const JsonLoader::JsonValue& persistentState = ObjectOrNull(root, "persistentState");
        if (persistentState.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        for (const auto& item : persistentState.object_values) {
            const PersistentStateBinding* binding = FindPersistentStateBinding(item.first);
            if (!binding || !IsPersistentFeatureSupported(binding->id) || item.second.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }

            persistentEnabled[binding->id] = JsonLoader::GetBool(item.second, "enabled", false);
            persistentValues[binding->id] = JsonLoader::GetBool(item.second, "value", binding->defaultValue);
        }

        ApplyPersistentStateValues();
    }

    void WritePersistentState(std::ostream& file, bool trailingComma) {
        file << "  \"persistentState\": {\n";
        const std::vector<PersistentStateBinding>& bindings = PersistentStateBindings();
        std::vector<const PersistentStateBinding*> supportedBindings;
        for (const PersistentStateBinding& binding : bindings) {
            if (IsPersistentFeatureSupported(binding.id)) {
                supportedBindings.push_back(&binding);
            }
        }

        for (std::size_t i = 0; i < supportedBindings.size(); ++i) {
            const PersistentStateBinding& binding = *supportedBindings[i];
            const bool enabled = persistentEnabled[binding.id];
            const bool value = persistentValues[binding.id];
            file << "    \"" << EscapeJson(binding.id) << "\": {\"enabled\": " << (enabled ? "true" : "false")
                 << ", \"value\": " << (value ? "true" : "false") << "}";
            if (i + 1 < supportedBindings.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << "  }" << (trailingComma ? "," : "") << "\n";
    }

    bool CapturePersistentStateValues() {
        bool changed = false;
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }

            if (!persistentEnabled[binding.id]) {
                continue;
            }

            const bool currentValue = *binding.value;
            if (persistentValues[binding.id] != currentValue) {
                persistentValues[binding.id] = currentValue;
                changed = true;
            }
        }
        return changed;
    }

    const char* GameKey() {
        return GameRuntime::CurrentKey();
    }

    const JsonLoader::JsonValue& GameConfigRoot(const JsonLoader::JsonValue& root, const char* gameKey) {
        const JsonLoader::JsonValue& games = ObjectOrNull(root, "games");
        const JsonLoader::JsonValue& game = ObjectOrNull(games, gameKey ? gameKey : "");
        return game.type == JsonLoader::JsonValue::OBJECT ? game : ObjectOrNull(root, "__missingGameConfig");
    }

    const JsonLoader::JsonValue& CurrentGameConfigOrLegacyRoot(const JsonLoader::JsonValue& root) {
        const JsonLoader::JsonValue& game = GameConfigRoot(root, GameKey());
        return game.type == JsonLoader::JsonValue::OBJECT ? game : root;
    }

    void WriteJsonValue(std::ostream& file, const JsonLoader::JsonValue& value) {
        switch (value.type) {
        case JsonLoader::JsonValue::STRING:
            file << "\"" << EscapeJson(value.string_value) << "\"";
            break;
        case JsonLoader::JsonValue::NUMBER:
            file << value.number_value;
            break;
        case JsonLoader::JsonValue::BOOL:
            file << (value.bool_value ? "true" : "false");
            break;
        case JsonLoader::JsonValue::ARRAY:
            file << "[";
            for (std::size_t i = 0; i < value.array_values.size(); ++i) {
                if (i > 0) {
                    file << ", ";
                }
                WriteJsonValue(file, value.array_values[i]);
            }
            file << "]";
            break;
        case JsonLoader::JsonValue::OBJECT: {
            file << "{";
            std::size_t index = 0;
            for (const auto& item : value.object_values) {
                if (index++ > 0) {
                    file << ", ";
                }
                file << "\"" << EscapeJson(item.first) << "\": ";
                WriteJsonValue(file, item.second);
            }
            file << "}";
            break;
        }
        case JsonLoader::JsonValue::NULL_VALUE:
        default:
            file << "null";
            break;
        }
    }

    void LoadOverlayConfig(const JsonLoader::JsonValue& root) {
        const JsonLoader::JsonValue& overlay = ObjectOrNull(root, "overlay");
        if (overlay.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        MenuState::OverlayEnabled = JsonLoader::GetBool(overlay, "enabled", MenuState::OverlayEnabled);
        MenuState::OverlayShowPosition = JsonLoader::GetBool(overlay, "showPosition", MenuState::OverlayShowPosition);
        MenuState::OverlayShowVehicle = JsonLoader::GetBool(overlay, "showVehicle", MenuState::OverlayShowVehicle);
        MenuState::OverlayShowFps = JsonLoader::GetBool(overlay, "showFps", MenuState::OverlayShowFps);
        MenuState::OverlayShowPlayer = JsonLoader::GetBool(overlay, "showPlayer", MenuState::OverlayShowPlayer);
        MenuState::OverlayShowTime = JsonLoader::GetBool(overlay, "showTime", MenuState::OverlayShowTime);
        MenuState::OverlayShowWorld = JsonLoader::GetBool(overlay, "showWorld", MenuState::OverlayShowWorld);
        MenuState::OverlayShowDetails = JsonLoader::GetBool(overlay, "showDetails", MenuState::OverlayShowDetails);
        MenuState::OverlayShowFeatures = JsonLoader::GetBool(overlay, "showFeatures", MenuState::OverlayShowFeatures);
    }

    void WriteOverlayConfig(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"overlay\": {\n";
        file << indent << "  \"enabled\": " << (MenuState::OverlayEnabled ? "true" : "false") << ",\n";
        file << indent << "  \"showPosition\": " << (MenuState::OverlayShowPosition ? "true" : "false") << ",\n";
        file << indent << "  \"showVehicle\": " << (MenuState::OverlayShowVehicle ? "true" : "false") << ",\n";
        file << indent << "  \"showFps\": " << (MenuState::OverlayShowFps ? "true" : "false") << ",\n";
        file << indent << "  \"showPlayer\": " << (MenuState::OverlayShowPlayer ? "true" : "false") << ",\n";
        file << indent << "  \"showTime\": " << (MenuState::OverlayShowTime ? "true" : "false") << ",\n";
        file << indent << "  \"showWorld\": " << (MenuState::OverlayShowWorld ? "true" : "false") << ",\n";
        file << indent << "  \"showDetails\": " << (MenuState::OverlayShowDetails ? "true" : "false") << ",\n";
        file << indent << "  \"showFeatures\": " << (MenuState::OverlayShowFeatures ? "true" : "false") << "\n";
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void LoadTeleportConfig(const JsonLoader::JsonValue& root) {
        const JsonLoader::JsonValue& teleport = ObjectOrNull(root, "teleport");
        if (teleport.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        MenuState::TeleportForwardDistance = static_cast<float>(JsonLoader::GetNumber(teleport, "forwardDistance", MenuState::TeleportForwardDistance));
        if (MenuState::TeleportForwardDistance < 0.1f) {
            MenuState::TeleportForwardDistance = 0.1f;
        }
        MenuState::TeleportForwardHold = JsonLoader::GetBool(teleport, "forwardHold", MenuState::TeleportForwardHold);
    }

    void WriteTeleportConfig(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"teleport\": {\n";
        file << indent << "  \"forwardDistance\": " << MenuState::TeleportForwardDistance << ",\n";
        file << indent << "  \"forwardHold\": " << (MenuState::TeleportForwardHold ? "true" : "false") << "\n";
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void LoadWeaponConfig(const JsonLoader::JsonValue& root) {
        const JsonLoader::JsonValue& weapon = ObjectOrNull(root, "weapon");
        if (weapon.type != JsonLoader::JsonValue::OBJECT) {
            return;
        }

        MenuState::WeaponBulletLockRange = static_cast<float>(JsonLoader::GetNumber(
            weapon, "bulletLockRange", MenuState::WeaponBulletLockRange));
        if (MenuState::WeaponBulletLockRange < 10.0f) {
            MenuState::WeaponBulletLockRange = 10.0f;
        }
        if (MenuState::WeaponBulletLockRange > 300.0f) {
            MenuState::WeaponBulletLockRange = 300.0f;
        }
    }

    void WriteWeaponConfig(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"weapon\": {\n";
        file << indent << "  \"bulletLockRange\": " << MenuState::WeaponBulletLockRange << "\n";
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void LoadWorldConfig(const JsonLoader::JsonValue& root) {
        // 时间锁定由 features binding 持久化，禁止强制 false
        (void)root;
    }

    void WriteWorldConfig(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"world\": {\n";
        file << indent << "  \"note\": \"lockTime/freezeTime live under features bindings\"\n";
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void WriteMenuConfig(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"menu\": {\n";
        file << indent << "  \"toggleKey\": \"" << EscapeJson(menuKeyName) << "\",\n";
        file << indent << "  \"fallbackLanguage\": \"" << EscapeJson(fallbackLanguageCode) << "\",\n";
        file << indent << "  \"guiTheme\": " << MenuState::GuiThemeIndex << ",\n";
        file << indent << "  \"interaction\": " << MenuState::GuiInteractionMode << ",\n";
        file << indent << "  \"UseNativeMenu\": " << (MenuState::UseNativeMenu ? "true" : "false") << ",\n";
        file << indent << "  \"ListMenuMouseInput\": " << (MenuState::ListMenuMouseInput ? "true" : "false") << ",\n";
        file << indent << "  \"hotkey\": {\n";
        file << indent << "    \"key\": " << menuHotkey.key << ",\n";
        file << indent << "    \"ctrl\": " << (menuHotkey.ctrl ? "true" : "false") << ",\n";
        file << indent << "    \"alt\": " << (menuHotkey.alt ? "true" : "false") << ",\n";
        file << indent << "    \"shift\": " << (menuHotkey.shift ? "true" : "false") << "\n";
        file << indent << "  }\n";
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void WriteActionHotkeys(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"actions\": {\n";
        for (std::size_t i = 0; i < actionHotkeys.size(); ++i) {
            const AppConfig::ActionHotkey& action = actionHotkeys[i];
            file << indent << "  \"" << EscapeJson(action.id) << "\": \"" << EscapeJson(CanonicalHotkeyName(action.hotkey)) << "\"";
            if (i + 1 < actionHotkeys.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void WritePersistentState(std::ostream& file, const std::string& indent, bool trailingComma) {
        file << indent << "\"persistentState\": {\n";
        const std::vector<PersistentStateBinding>& bindings = PersistentStateBindings();
        std::vector<const PersistentStateBinding*> supportedBindings;
        for (const PersistentStateBinding& binding : bindings) {
            if (IsPersistentFeatureSupported(binding.id)) {
                supportedBindings.push_back(&binding);
            }
        }

        for (std::size_t i = 0; i < supportedBindings.size(); ++i) {
            const PersistentStateBinding& binding = *supportedBindings[i];
            const bool enabled = persistentEnabled[binding.id];
            const bool value = persistentValues[binding.id];
            file << indent << "  \"" << EscapeJson(binding.id) << "\": {\"enabled\": " << (enabled ? "true" : "false")
                 << ", \"value\": " << (value ? "true" : "false") << "}";
            if (i + 1 < supportedBindings.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << indent << "}" << (trailingComma ? "," : "") << "\n";
    }

    void WriteLocationArray(std::ostream& file, const std::string& indent, const char* key, const std::vector<DataManager::LocationData>& locations, bool trailingComma) {
        file << indent << "\"" << key << "\": [\n";
        for (std::size_t i = 0; i < locations.size(); ++i) {
            const DataManager::LocationData& location = locations[i];
            file << indent << "  {\"name\": \"" << EscapeJson(location.name) << "\", \"x\": " << location.x
                 << ", \"y\": " << location.y << ", \"z\": " << location.z
                 << ", \"interior\": " << location.interior << "}";
            if (i + 1 < locations.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << indent << "]" << (trailingComma ? "," : "") << "\n";
    }

    void LoadLocationsFromArray(const std::vector<JsonLoader::JsonValue>& locations, std::vector<DataManager::LocationData>& output) {
        output.clear();
        for (const JsonLoader::JsonValue& item : locations) {
            if (item.type != JsonLoader::JsonValue::OBJECT) {
                continue;
            }

            DataManager::LocationData location;
            location.category = "custom";
            location.name = JsonLoader::GetString(item, "name", "");
            location.x = static_cast<float>(JsonLoader::GetNumber(item, "x", 0.0));
            location.y = static_cast<float>(JsonLoader::GetNumber(item, "y", 0.0));
            location.z = static_cast<float>(JsonLoader::GetNumber(item, "z", 0.0));
            location.interior = static_cast<int>(JsonLoader::GetNumber(item, "interior", 0));
            if (!location.name.empty()) {
                output.push_back(location);
            }
        }
    }

    std::vector<DataManager::LocationData> ReadCurrentGameLocations(const JsonLoader::JsonValue& root) {
        std::vector<DataManager::LocationData> locations;
        const JsonLoader::JsonValue& gameConfig = CurrentGameConfigOrLegacyRoot(root);
        LoadLocationsFromArray(JsonLoader::GetArray(gameConfig, "customLocations"), locations);
        if (!locations.empty()) {
            return locations;
        }

        LoadLocationsFromArray(JsonLoader::GetArray(root, GameKey()), locations);
        if (!locations.empty()) {
            return locations;
        }

        LoadLocationsFromArray(JsonLoader::GetArray(root, "customLocations"), locations);
        return locations;
    }

    void AppendLocations(const std::vector<DataManager::LocationData>& locations) {
        customLocations.insert(customLocations.end(), locations.begin(), locations.end());
    }

    void WriteLocationArray(std::ostream& file, const char* key, const std::vector<DataManager::LocationData>& locations, bool trailingComma) {
        file << "  \"" << key << "\": [\n";
        for (std::size_t i = 0; i < locations.size(); ++i) {
            const DataManager::LocationData& location = locations[i];
            file << "    {\"name\": \"" << EscapeJson(location.name) << "\", \"x\": " << location.x
                 << ", \"y\": " << location.y << ", \"z\": " << location.z
                 << ", \"interior\": " << location.interior << "}";
            if (i + 1 < locations.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << "  ]" << (trailingComma ? "," : "") << "\n";
    }

    void LoadUpdateCacheData(const JsonLoader::JsonValue& root) {
        const JsonLoader::JsonValue& cache = ObjectOrNull(root, "updateCache");
        updateCache.timestamp = static_cast<long long>(JsonLoader::GetNumber(cache, "timestamp", 0.0));
        updateCache.tagName = JsonLoader::GetString(cache, "tagName", "");
        updateCache.htmlUrl = JsonLoader::GetString(cache, "htmlUrl", "");
        updateCache.source = JsonLoader::GetString(cache, "source", "");
    }

    void WriteUpdateCache(std::ostream& file, bool trailingComma) {
        file << "  \"updateCache\": {\n";
        file << "    \"timestamp\": " << updateCache.timestamp << ",\n";
        file << "    \"tagName\": \"" << EscapeJson(updateCache.tagName) << "\",\n";
        file << "    \"htmlUrl\": \"" << EscapeJson(updateCache.htmlUrl) << "\",\n";
        file << "    \"source\": \"" << EscapeJson(updateCache.source) << "\"\n";
        file << "  }" << (trailingComma ? "," : "") << "\n";
    }

    void LoadConfigData(const JsonLoader::JsonValue& root) {
        LoadUpdateCacheData(root);

        const JsonLoader::JsonValue& gameConfig = CurrentGameConfigOrLegacyRoot(root);
        LoadActionHotkeys(gameConfig);
        LoadPersistentState(gameConfig);
        LoadOverlayConfig(gameConfig);
        LoadTeleportConfig(gameConfig);
        LoadWeaponConfig(gameConfig);
        LoadWorldConfig(gameConfig);
        LoadGuiConfig(gameConfig);

        const JsonLoader::JsonValue& menu = ObjectOrNull(gameConfig, "menu");
        ApplyMenuKey(JsonLoader::GetString(menu, "toggleKey", DefaultMenuKey));
        fallbackLanguageCode = JsonLoader::GetString(menu, "fallbackLanguage", "zh");
        I18n::SetFallbackLanguage(fallbackLanguageCode);
        fallbackLanguageCode = I18n::GetFallbackLanguageCode();

        LoadLocationsFromArray(JsonLoader::GetArray(gameConfig, "customLocations"), customLocations);
        if (!customLocations.empty()) {
            return;
        }

        const std::vector<JsonLoader::JsonValue>& gameLocations = JsonLoader::GetArray(root, GameKey());
        if (!gameLocations.empty()) {
            LoadLocationsFromArray(gameLocations, customLocations);
            return;
        }

        const std::vector<JsonLoader::JsonValue>& legacyLocations = JsonLoader::GetArray(root, "customLocations");
        LoadLocationsFromArray(legacyLocations, customLocations);
    }

    bool WriteConfigData(std::ostream& file, const JsonLoader::JsonValue* sourceRoot = nullptr, AppConfig::TransferScope scope = AppConfig::TransferScope::All) {
        const JsonLoader::JsonValue existingRoot = sourceRoot ? *sourceRoot : JsonLoader::LoadFromFile(ReadConfigPath());
        const JsonLoader::JsonValue& existingGames = ObjectOrNull(existingRoot, "games");
        const std::string currentGame = GameKey();

        file << "{\n";
        if (scope == AppConfig::TransferScope::CustomLocations) {
            file << "  \"games\": {\n";
            file << "    \"" << EscapeJson(currentGame) << "\": {\n";
            WriteLocationArray(file, "      ", "customLocations", customLocations, false);
            file << "    }\n";
            file << "  }\n";
            file << "}\n";
            return !file.fail();
        }

        file << "  \"XMenu\": {\n";
        file << "    \"version\": \"" << EscapeJson(XMENU_VERSION) << "\",\n";
        file << "    \"XMENU_AUTHOR\": \"" << EscapeJson(XMENU_AUTHOR) << "\",\n";
        file << "    \"XMENU_URL\": \"" << EscapeJson(XMENU_URL) << "\",\n";
        file << "    \"XMENU_GITHUB\": \"" << EscapeJson(XMENU_GITHUB) << "\"\n";
        file << "  },\n";
        WriteUpdateCache(file, true);
        file << "  \"games\": {\n";

        const char* games[] = {"iii", "vc", "sa"};
        for (std::size_t i = 0; i < 3; ++i) {
            const std::string game = games[i];
            file << "    \"" << game << "\": ";

            if (game == currentGame) {
                file << "{\n";
                WriteMenuConfig(file, "      ", true);
                WriteOverlayConfig(file, "      ", true);
                WriteTeleportConfig(file, "      ", true);
                WriteWeaponConfig(file, "      ", true);
                WriteWorldConfig(file, "      ", true);
                WriteActionHotkeys(file, "      ", true);
                WritePersistentState(file, "      ", true);
                WriteLocationArray(file, "      ", "customLocations", customLocations, false);
                file << "    }";
            } else {
                const JsonLoader::JsonValue& existingGame = GameConfigRoot(existingRoot, game.c_str());
                if (existingGame.type == JsonLoader::JsonValue::OBJECT) {
                    WriteJsonValue(file, existingGame);
                } else {
                    std::vector<DataManager::LocationData> legacyLocations;
                    LoadLocationsFromArray(JsonLoader::GetArray(existingRoot, game), legacyLocations);
                    file << "{\n";
                    WriteLocationArray(file, "      ", "customLocations", legacyLocations, false);
                    file << "    }";
                }
            }

            if (i + 1 < 3) {
                file << ",";
            }
            file << "\n";
        }

        file << "  }\n";
        file << "}\n";
        return !file.fail();
    }

    bool SaveToPath(const std::string& path, const JsonLoader::JsonValue* sourceRoot = nullptr, AppConfig::TransferScope scope = AppConfig::TransferScope::All) {
        const JsonLoader::JsonValue existingRoot = sourceRoot ? *sourceRoot : JsonLoader::LoadFromFile(ReadConfigPath());

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            Log::Warn(std::string("配置文件写入失败: ") + path);
            return false;
        }

        return WriteConfigData(file, &existingRoot, scope);
    }

    void ApplyMenuKey(const std::string& keyName) {
        bool valid = false;
        AppConfig::Hotkey parsed = ParseHotkey(keyName, valid);
        if (!valid) {
            Log::Warn(std::string("菜单快捷键无效，已回退到 M: ") + keyName);
            parsed = AppConfig::Hotkey{};
            parsed.key = 'M';
        }

        menuHotkey = parsed;
        menuKeyName = CanonicalHotkeyName(menuHotkey);
    }
}

namespace AppConfig {
    void Init() {
        customLocations.clear();
        ResetActionHotkeysToDefaults();
        ResetPersistentStateToDefaults();
        ApplyMenuKey(DefaultMenuKey);
        fallbackLanguageCode = "zh";
        I18n::SetFallbackLanguage(fallbackLanguageCode);
        fallbackLanguageCode = I18n::GetFallbackLanguageCode();

        const std::string path = ReadConfigPath();
        EnsureDirectory(XMenuDataDirectory());
        const JsonLoader::JsonValue root = JsonLoader::LoadFromFile(path);
        persistentSyncReadyTick = GetTickCount64() + PersistentStartupDelayMs;
        if (root.type != JsonLoader::JsonValue::OBJECT) {
            Log::Info(std::string("配置文件不存在或为空，将创建默认配置: ") + path);
            Save();
            return;
        }

        LoadConfigData(root);
        Log::Info(std::string("配置加载完成：游戏=") + GameKey() + "，快捷键=" + menuKeyName + "，自定义地点=" + std::to_string(customLocations.size()));
    }

    void Save() {
        EnsureDirectory(XMenuDataDirectory());
        SaveToPath(ConfigPath());
    }

    std::string GetConfigPath() {
        return ConfigPath();
    }

    bool LoadUpdateCache(UpdateCache& cache) {
        const JsonLoader::JsonValue root = JsonLoader::LoadFromFile(ReadConfigPath());
        if (root.type != JsonLoader::JsonValue::OBJECT) {
            return false;
        }

        LoadUpdateCacheData(root);
        cache = updateCache;
        return cache.timestamp > 0 && !cache.tagName.empty();
    }

    void SaveUpdateCache(const UpdateCache& cache) {
        updateCache = cache;
        SaveToPath(ConfigPath());
    }

    bool ApplyImportRoot(const JsonLoader::JsonValue& root, TransferScope scope, const std::string& sourceLabel) {
        if (root.type != JsonLoader::JsonValue::OBJECT) {
            Log::Warn(std::string("配置导入失败，内容无效: ") + sourceLabel);
            return false;
        }

        if (scope == TransferScope::CustomLocations) {
            const std::vector<DataManager::LocationData> importedLocations = ReadCurrentGameLocations(root);
            AppendLocations(importedLocations);
            SaveToPath(ConfigPath());
            Log::Info(std::string("自定义地点导入完成: ") + sourceLabel + "，追加数量=" + std::to_string(importedLocations.size()));
            return true;
        }

        customLocations.clear();
        ResetPersistentStateToDefaults();
        ApplyMenuKey(DefaultMenuKey);
        LoadConfigData(root);
        SaveToPath(ConfigPath(), &root);
        Log::Info(std::string("配置导入完成: ") + sourceLabel);
        return true;
    }

    bool ImportFrom(const std::string& path, TransferScope scope) {
        const JsonLoader::JsonValue root = JsonLoader::LoadFromFile(path);
        return ApplyImportRoot(root, scope, path);
    }

    bool ImportFromText(const std::string& text, TransferScope scope) {
        const JsonLoader::JsonValue root = JsonLoader::Parse(text);
        return ApplyImportRoot(root, scope, "text");
    }

    bool ExportTo(const std::string& path, TransferScope scope) {
        if (!SaveToPath(path, nullptr, scope)) {
            return false;
        }
        Log::Info(std::string(scope == TransferScope::CustomLocations ? "自定义地点导出完成: " : "配置导出完成: ") + path);
        return true;
    }

    std::string ExportToText(TransferScope scope) {
        std::ostringstream output;
        if (!WriteConfigData(output, nullptr, scope)) {
            return "";
        }
        Log::Info(scope == TransferScope::CustomLocations ? "自定义地点已导出到文本" : "配置已导出到文本");
        return output.str();
    }

    std::vector<PersistentFeatureState> GetPersistentFeatureStates() {
        std::vector<PersistentFeatureState> features;
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }

            PersistentFeatureState feature;
            feature.id = binding.id;
            feature.name = binding.name;
            feature.group = PersistentFeatureGroup(binding.id);
            feature.enabledNow = *binding.value;
            feature.restoreNextLaunch = persistentEnabled[binding.id];
            feature.restoreValue = persistentValues[binding.id];
            features.push_back(feature);
        }
        return features;
    }

    std::size_t GetPersistentRestoreCount() {
        std::size_t count = 0;
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }

            if (persistentEnabled[binding.id]) {
                ++count;
            }
        }
        return count;
    }

    void CaptureEnabledPersistentFeatures() {
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }

            const bool currentValue = *binding.value;
            persistentEnabled[binding.id] = currentValue;
            persistentValues[binding.id] = currentValue ? true : binding.defaultValue;
        }

        persistentSyncReadyTick = GetTickCount64() + PersistentStartupDelayMs;
        Save();
    }

    void ApplyPersistentRestoreSelection(const std::vector<std::string>& selectedIds) {
        std::unordered_map<std::string, bool> selected;
        for (const std::string& id : selectedIds) {
            selected[id] = true;
        }

        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }

            const bool restore = selected.find(binding.id) != selected.end();
            persistentEnabled[binding.id] = restore;
            persistentValues[binding.id] = restore ? true : binding.defaultValue;
        }

        persistentSyncReadyTick = GetTickCount64() + PersistentStartupDelayMs;
        Save();
    }

    void ClearPersistentRestoreSelection() {
        for (const PersistentStateBinding& binding : PersistentStateBindings()) {
            if (!IsPersistentFeatureSupported(binding.id)) {
                continue;
            }

            persistentEnabled[binding.id] = false;
            persistentValues[binding.id] = binding.defaultValue;
        }

        persistentSyncReadyTick = GetTickCount64() + PersistentStartupDelayMs;
        Save();
    }

    void SyncPersistentState() {
        const ULONGLONG now = GetTickCount64();
        if (syncingPersistentState || now < persistentSyncReadyTick || now - lastPersistentSyncTick < PersistentWriteCooldownMs) {
            return;
        }

        if (!CapturePersistentStateValues()) {
            return;
        }

        syncingPersistentState = true;
        Save();
        lastPersistentSyncTick = now;
        syncingPersistentState = false;
    }

    const Hotkey& GetMenuHotkey() {
        return menuHotkey;
    }

    bool IsMenuHotkeyPressed() {
        return IsModifierDown(menuHotkey.ctrl, VK_CONTROL, VK_LCONTROL, VK_RCONTROL)
            && IsModifierDown(menuHotkey.alt, VK_MENU, VK_LMENU, VK_RMENU)
            && IsModifierDown(menuHotkey.shift, VK_SHIFT, VK_LSHIFT, VK_RSHIFT)
            && IsKeyDown(menuHotkey.key);
    }

    int GetMenuKeyVirtualKey() {
        return menuHotkey.key;
    }

    std::string GetMenuKeyName() {
        return menuKeyName;
    }

    void SetMenuKeyName(const std::string& keyName) {
        ApplyMenuKey(keyName);
        Save();
    }

    std::string GetFallbackLanguageCode() {
        return fallbackLanguageCode;
    }

    void SetFallbackLanguageCode(const std::string& code) {
        I18n::SetFallbackLanguage(code);
        fallbackLanguageCode = I18n::GetFallbackLanguageCode();
        Save();
    }

    const std::vector<ActionHotkey>& GetActionHotkeys() {
        return actionHotkeys;
    }

    bool IsActionHotkeySupportedForRuntime(const std::string& actionId) {
        return ::IsActionHotkeySupportedForRuntime(actionId.c_str());
    }

    const Hotkey* GetActionHotkey(const std::string& actionId) {
        if (!IsActionHotkeySupportedForRuntime(actionId)) {
            return nullptr;
        }

        const ActionHotkey* action = FindActionHotkey(actionId);
        return action ? &action->hotkey : nullptr;
    }

    std::string GetActionHotkeyName(const std::string& actionId) {
        const Hotkey* hotkey = GetActionHotkey(actionId);
        return hotkey ? CanonicalHotkeyName(*hotkey) : "None";
    }

    void SetActionHotkeyName(const std::string& actionId, const std::string& keyName) {
        if (!IsActionHotkeySupportedForRuntime(actionId)) {
            Log::Warn(std::string("当前游戏不支持动作快捷键: ") + actionId);
            return;
        }

        ActionHotkey* action = FindActionHotkey(actionId);
        if (!action) {
            Log::Warn(std::string("动作快捷键不存在: ") + actionId);
            return;
        }

        bool valid = false;
        const Hotkey parsed = ParseHotkey(keyName, valid);
        if (!valid) {
            Log::Warn(std::string("动作快捷键无效: ") + actionId + " = " + keyName);
            return;
        }

        action->hotkey = parsed;
        Save();
    }

    bool IsHotkeyPressed(const Hotkey& hotkey) {
        if (hotkey.key <= 0) {
            return false;
        }

        return IsModifierDown(hotkey.ctrl, VK_CONTROL, VK_LCONTROL, VK_RCONTROL)
            && IsModifierDown(hotkey.alt, VK_MENU, VK_LMENU, VK_RMENU)
            && IsModifierDown(hotkey.shift, VK_SHIFT, VK_LSHIFT, VK_RSHIFT)
            && IsKeyDown(hotkey.key);
    }

    std::string FormatHotkey(const Hotkey& hotkey) {
        return CanonicalHotkeyName(hotkey);
    }

    const std::vector<DataManager::LocationData>& GetCustomLocations() {
        return customLocations;
    }

    bool AddCustomLocation(const std::string& name, float x, float y, float z, int interior) {
        const std::string trimmedName = Trim(name);
        if (trimmedName.empty()) {
            return false;
        }

        DataManager::LocationData location;
        location.category = "custom";
        location.name = trimmedName;
        location.x = x;
        location.y = y;
        location.z = z;
        location.interior = interior;
        customLocations.push_back(location);
        Save();
        Log::Info(std::string("已添加自定义地点: ") + trimmedName);
        return true;
    }

    int GetGuiThemeIndex() {
        return MenuState::GuiThemeIndex;
    }

    void SetGuiThemeIndex(int index) {
        if (index >= 0 && index < GuiTheme::ThemeCount) {
            MenuState::GuiThemeIndex = index;
            GuiTheme::SetThemeByIndex(index);
            GuiTheme::ApplyToStyle();
            Save();
        }
    }

    int GetInteractionMode() {
        return MenuState::GuiInteractionMode;
    }

    void SetInteractionMode(int mode) {
        if (mode >= 0 && mode < GuiTheme::InteractionCount) {
            MenuState::GuiInteractionMode = mode;
            GuiTheme::SetInteractionByIndex(mode);
            GuiTheme::ApplyInteraction();
            Save();
        }
    }
}