#include "I18n.h"
#include "resources/I18nResources.h"
#include "utils/Log.h"
#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <windows.h>

namespace {
    using Dictionary = std::unordered_map<std::string, std::string>;

    constexpr std::size_t LanguageCount = 4;
    I18n::Language currentLanguage = I18n::Language::Zh;
    std::array<Dictionary, LanguageCount> dictionaries;

    int LanguageIndex(I18n::Language language) {
        return static_cast<int>(language);
    }

    std::string DecodeJsonString(const std::string& value) {
        std::string decoded;
        decoded.reserve(value.size());

        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] != '\\' || i + 1 >= value.size()) {
                decoded.push_back(value[i]);
                continue;
            }

            const char escaped = value[++i];
            switch (escaped) {
            case 'n': decoded.push_back('\n'); break;
            case 'r': decoded.push_back('\r'); break;
            case 't': decoded.push_back('\t'); break;
            case '"': decoded.push_back('"'); break;
            case '\\': decoded.push_back('\\'); break;
            case '/': decoded.push_back('/'); break;
            default:
                decoded.push_back(escaped);
                break;
            }
        }

        return decoded;
    }

    void ParseFlatJson(const std::string& content, Dictionary& dictionary) {
        std::size_t cursor = 0;
        while (cursor < content.size()) {
            const std::size_t keyStartQuote = content.find('"', cursor);
            if (keyStartQuote == std::string::npos) {
                break;
            }

            const std::size_t keyEndQuote = content.find('"', keyStartQuote + 1);
            if (keyEndQuote == std::string::npos) {
                break;
            }

            const std::size_t colon = content.find(':', keyEndQuote + 1);
            if (colon == std::string::npos) {
                break;
            }

            const std::size_t valueStartQuote = content.find('"', colon + 1);
            if (valueStartQuote == std::string::npos) {
                break;
            }

            std::size_t valueEndQuote = valueStartQuote + 1;
            bool escaped = false;
            while (valueEndQuote < content.size()) {
                const char current = content[valueEndQuote];
                if (current == '"' && !escaped) {
                    break;
                }
                escaped = current == '\\' && !escaped;
                if (current != '\\') {
                    escaped = false;
                }
                ++valueEndQuote;
            }

            if (valueEndQuote >= content.size()) {
                break;
            }

            const std::string key = DecodeJsonString(content.substr(keyStartQuote + 1, keyEndQuote - keyStartQuote - 1));
            const std::string value = DecodeJsonString(content.substr(valueStartQuote + 1, valueEndQuote - valueStartQuote - 1));
            dictionary[key] = value;
            cursor = valueEndQuote + 1;
        }
    }

    bool LoadDictionary(I18n::Language language, const char* path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        ParseFlatJson(content, dictionaries[LanguageIndex(language)]);
        return !dictionaries[LanguageIndex(language)].empty();
    }

    bool LoadDictionaryFromResource(I18n::Language language, int resourceId) {
        // 尝试从当前DLL模块加载资源
        HMODULE hModule = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCSTR)&LoadDictionaryFromResource, &hModule);
        
        if (!hModule) {
            Log::Warn("无法获取当前模块句柄");
            hModule = GetModuleHandle(NULL);
        }
        
        char moduleName[MAX_PATH];
        GetModuleFileNameA(hModule, moduleName, MAX_PATH);
        Log::Info(std::string("尝试从模块加载资源: ") + moduleName + ", 资源ID: " + std::to_string(resourceId));
        
        HRSRC hResource = FindResourceA(hModule, MAKEINTRESOURCEA(resourceId), RT_RCDATA);
        if (!hResource) {
            DWORD error = GetLastError();
            Log::Warn(std::string("资源加载失败: 找不到资源ID ") + std::to_string(resourceId) + ", 错误码: " + std::to_string(error));
            return false;
        }

        HGLOBAL hLoaded = LoadResource(hModule, hResource);
        if (!hLoaded) {
            Log::Warn(std::string("资源加载失败: 无法加载资源ID ") + std::to_string(resourceId));
            return false;
        }

        const DWORD size = SizeofResource(hModule, hResource);
        const void* data = LockResource(hLoaded);
        if (!data || size == 0) {
            Log::Warn(std::string("资源加载失败: 资源数据为空，ID ") + std::to_string(resourceId));
            return false;
        }

        const std::string content(static_cast<const char*>(data), size);
        ParseFlatJson(content, dictionaries[LanguageIndex(language)]);
        
        bool success = !dictionaries[LanguageIndex(language)].empty();
        if (success) {
            Log::Info(std::string("从资源成功加载语言字典，ID: ") + std::to_string(resourceId) + ", 词条数: " + std::to_string(dictionaries[LanguageIndex(language)].size()));
        } else {
            Log::Warn(std::string("从资源加载语言字典失败，解析后为空，ID: ") + std::to_string(resourceId));
        }
        return success;
    }

    void LoadFallbackZh() {
        Dictionary& zh = dictionaries[LanguageIndex(I18n::Language::Zh)];
        if (!zh.empty()) {
            return;
        }

        const std::pair<const char*, const char*> fallback[] = {
            {"window.title", (const char*)u8"XMenu 作者：%s - GTAMODX"},
            {"tab.player", (const char*)u8"玩家"}, {"tab.vehicle", (const char*)u8"载具"}, {"tab.teleport", (const char*)u8"传送"}, {"tab.weapon", (const char*)u8"武器"}, {"tab.world", (const char*)u8"世界"}, {"tab.settings", (const char*)u8"设置"}, {"tab.about", (const char*)u8"关于"},
            {"settings.language", (const char*)u8"语言"}, {"settings.interfaceLanguage", (const char*)u8"界面语言"}, {"settings.applyImmediately", (const char*)u8"切换后立即生效"},
            {"player.copyCoordinates", (const char*)u8"复制坐标"}, {"player.healFully", (const char*)u8"回满血量"}, {"player.refillArmor", (const char*)u8"补满护甲"}, {"player.addMoney", (const char*)u8"加 25 万现金"}, {"player.kill", (const char*)u8"立即倒地"},
            {"common.toggles", (const char*)u8"功能开关"}, {"player.statusToggles", (const char*)u8"状态开关"}, {"player.valueAdjustments", (const char*)u8"数值调整"}, {"player.godMode", (const char*)u8"无敌"}, {"player.autoHeal", (const char*)u8"自动回血"}, {"player.hardMode", (const char*)u8"50 血量"}, {"player.infiniteSprint", (const char*)u8"无限冲刺"}, {"player.respawnAtDeathPosition", (const char*)u8"死亡后回到原地"}, {"player.freezeWantedLevel", (const char*)u8"冻结通缉"}, {"player.keepStuff", (const char*)u8"住院/被捕保留装备"}, {"player.freeHospital", (const char*)u8"住院不扣钱"}, {"player.freeJail", (const char*)u8"被捕不扣钱"}, {"player.proofFlags", (const char*)u8"单项防护"},
            {"proof.bullet", (const char*)u8"防弹"}, {"proof.collision", (const char*)u8"防撞"}, {"proof.explosion", (const char*)u8"防爆"}, {"proof.fire", (const char*)u8"防火"}, {"proof.melee", (const char*)u8"防近战"},
            {"player.health", (const char*)u8"血量"}, {"player.setHealth", (const char*)u8"设置血量"}, {"player.armor", (const char*)u8"护甲"}, {"player.setArmor", (const char*)u8"设置护甲"}, {"player.money", (const char*)u8"现金"}, {"player.setMoney", (const char*)u8"设置现金"}, {"player.wantedLevel", (const char*)u8"通缉星级"}, {"player.setWanted", (const char*)u8"设置通缉"}, {"player.readCurrentValues", (const char*)u8"读取当前数值"}, {"player.clearWanted", (const char*)u8"消除通缉"},
            {"vehicle.blowUpAll", (const char*)u8"炸毁所有载具"}, {"vehicle.noListData", (const char*)u8"当前版本没有载具列表数据，请用模型 ID 生成。"}, {"vehicle.notInVehicle", (const char*)u8"当前不在载具中，修车和载具状态功能暂不可用。"}, {"vehicle.repair", (const char*)u8"一键修车"}, {"vehicle.stop", (const char*)u8"立即停车"}, {"vehicle.unflip", (const char*)u8"扶正载具"}, {"vehicle.start", (const char*)u8"立即起步"}, {"vehicle.engineOn", (const char*)u8"点火"}, {"vehicle.engineOff", (const char*)u8"熄火"}, {"vehicle.lights", (const char*)u8"车灯"}, {"vehicle.lockDoors", (const char*)u8"车门上锁"}, {"vehicle.invisible", (const char*)u8"隐形载具"}, {"vehicle.alwaysSkidMarks", (const char*)u8"始终留下刹车痕"}, {"vehicle.disableParticles", (const char*)u8"禁用粒子效果"}, {"vehicle.driverTargetable", (const char*)u8"驾驶员可被瞄准"}, {"vehicle.missileTargetable", (const char*)u8"可被导弹锁定"}, {"vehicle.petrolTankWeakness", (const char*)u8"油箱弱点"}, {"vehicle.sirenAlarm", (const char*)u8"警笛/警报"}, {"vehicle.takeLessDamage", (const char*)u8"降低受损"}, {"vehicle.health", (const char*)u8"车身健康值"}, {"vehicle.setHealth", (const char*)u8"设置健康值"}, {"vehicle.readHealth", (const char*)u8"读取健康值"}, {"vehicle.spawnVehicle", (const char*)u8"生成载具"}, {"vehicle.noDamage", (const char*)u8"载具无伤"}, {"vehicle.autoUnflip", (const char*)u8"自动扶正"}, {"vehicle.heavy", (const char*)u8"车身加重"}, {"vehicle.watertight", (const char*)u8"载具防水"}, {"vehicle.lockSpeed", (const char*)u8"锁定车速"}, {"vehicle.targetSpeed", (const char*)u8"目标速度"}, {"vehicle.spawnAsDriver", (const char*)u8"生成后坐上驾驶位"}, {"vehicle.spawnAircraftInAir", (const char*)u8"飞机/直升机生成在空中"}, {"vehicle.spawnIdTip", (const char*)u8"提示：手输 ID 只允许当前游戏已知载具模型，避免不存在的 ID 进入创建流程导致崩溃。"}, {"vehicle.modelId", (const char*)u8"模型 ID"}, {"vehicle.spawnById", (const char*)u8"按 ID 生成"},
            {"weapon.playerNotReady", (const char*)u8"玩家还没准备好，稍等进档后再用。"}, {"weapon.getAll", (const char*)u8"获取所有武器"}, {"weapon.dropWeapon", (const char*)u8"丢出当前武器"}, {"weapon.clearWeapons", (const char*)u8"清空武器"}, {"weapon.removeCurrent", (const char*)u8"移除当前武器"}, {"weapon.highDamage", (const char*)u8"高伤害"}, {"weapon.fastReload", (const char*)u8"快速换弹"}, {"weapon.infiniteAmmo", (const char*)u8"无限弹药"}, {"weapon.longRange", (const char*)u8"远射程"}, {"weapon.moveWhileAiming", (const char*)u8"瞄准时可移动"}, {"weapon.moveWhileFiring", (const char*)u8"开火时可移动"}, {"weapon.noSpread", (const char*)u8"零散射"}, {"weapon.rapidFire", (const char*)u8"快速连射"}, {"weapon.dualWield", (const char*)u8"双持"}, {"weapon.getWeapon", (const char*)u8"获取武器"}, {"weapon.ammo", (const char*)u8"弹药"}, {"weapon.typeId", (const char*)u8"武器类型 ID"}, {"weapon.modelId", (const char*)u8"武器模型 ID"}, {"weapon.getById", (const char*)u8"按 ID 获取"},
            {"teleport.insertCurrentCoordinates", (const char*)u8"插入当前坐标"}, {"teleport.quickMapTeleport", (const char*)u8"快速地图传送"}, {"teleport.allowUnderwaterLanding", (const char*)u8"允许水下落点"}, {"teleport.quickMarkerTeleport", (const char*)u8"标记点快捷传送"}, {"teleport.coordinates", (const char*)u8"坐标"}, {"teleport.toCoordinates", (const char*)u8"传送到坐标"}, {"teleport.byMapPosition", (const char*)u8"按地图位置传送"}, {"teleport.toMarker", (const char*)u8"传送到标记点"}, {"teleport.mapCenter", (const char*)u8"地图中心"}, {"teleport.moveForward5m", (const char*)u8"向前挪 5 米"}, {"teleport.customMapSize", (const char*)u8"自定义地图尺寸"}, {"teleport.customMapSizeHint", (const char*)u8"如果快速地图传送位置偏移，可以在这里调整地图宽高。默认值是 6000 x 6000。"}, {"teleport.width", (const char*)u8"宽度"}, {"teleport.height", (const char*)u8"高度"}, {"teleport.applyMapSize", (const char*)u8"应用地图尺寸"}, {"teleport.restoreDefault", (const char*)u8"恢复默认"}, {"teleport.locations", (const char*)u8"地点"}, {"teleport.locationName", (const char*)u8"地点名称"}, {"teleport.customLocation", (const char*)u8"自定义地点"}, {"teleport.locationCoordinates", (const char*)u8"地点坐标"}, {"teleport.addLocation", (const char*)u8"添加地点"},
            {"world.time", (const char*)u8"时间"}, {"world.hour", (const char*)u8"小时"}, {"world.minute", (const char*)u8"分钟"}, {"world.syncRealTime", (const char*)u8"同步现实时间"}, {"world.weather", (const char*)u8"天气"}, {"world.lockCurrentWeather", (const char*)u8"锁住当前天气"}, {"world.gameRules", (const char*)u8"游戏规则"}, {"world.disableReplay", (const char*)u8"禁用回放"}, {"world.disableCheats", (const char*)u8"禁用作弊码"}, {"world.fasterClock", (const char*)u8"加快时钟"}, {"world.freezeTime", (const char*)u8"冻结时间"}, {"world.disableForbiddenAreaWanted", (const char*)u8"禁止通缉区域"}, {"world.freePayNSpray", (const char*)u8"免费喷漆店"}, {"world.daysPassed", (const char*)u8"经过天数"}, {"world.setDays", (const char*)u8"设置天数"}, {"world.readDays", (const char*)u8"读取天数"}, {"world.gravity", (const char*)u8"重力"}, {"world.fpsLimit", (const char*)u8"FPS 限制"}, {"world.setFps", (const char*)u8"设置 FPS"}, {"world.readFps", (const char*)u8"读取 FPS"}, {"world.gameSpeed", (const char*)u8"游戏节奏"}, {"world.multiplier", (const char*)u8"倍率"}, {"world.restoreSpeed", (const char*)u8"恢复原速"}, {"weather.sunny", (const char*)u8"晴天"}, {"weather.cloudy", (const char*)u8"多云"}, {"weather.rainy", (const char*)u8"下雨"}, {"weather.foggy", (const char*)u8"起雾"},
            {"about.version", (const char*)u8"版本：%s"}, {"about.author", (const char*)u8"作者：%s"}, {"about.testing", (const char*)u8"测试：%s"}, {"about.techStack", (const char*)u8"技术栈：%s"}, {"about.openSourceLibs", (const char*)u8"开源库：%s"}, {"about.notice1", (const char*)u8"1. 永久免费，禁止倒卖，禁止用于商业用途。"}, {"about.notice2", (const char*)u8"2. 遇到问题可以加群或前往项目主页反馈。"}, {"about.joinGroup", (const char*)u8"加群"}, {"about.projectPage", (const char*)u8"项目主页"},
            {"update.availableTitle", (const char*)u8"发现新版本"}, {"update.availableMessage", (const char*)u8"当前版本：%s\n最新版本：%s\n请选择更新来源。"}, {"update.openGitHub", "GitHub"}, {"update.openGTAMODX", "GTAMODX"},
            {"status.language", (const char*)u8"语言"}, {"status.localVersion", (const char*)u8"本地：%s"}, {"status.remoteVersion", (const char*)u8"云端：%s"}, {"status.remoteUnknown", (const char*)u8"未知"}
        };

        for (const auto& item : fallback) {
            zh[item.first] = item.second;
        }
    }

    void TryLoadAllFrom(const std::string& baseDir) {
        LoadDictionary(I18n::Language::Zh, (baseDir + "zh.json").c_str());
        LoadDictionary(I18n::Language::En, (baseDir + "en.json").c_str());
        LoadDictionary(I18n::Language::Jp, (baseDir + "jp.json").c_str());
        LoadDictionary(I18n::Language::Ru, (baseDir + "ru.json").c_str());
    }
}

namespace I18n {
    void Init() {
        Log::Info("开始初始化 i18n 系统...");
        
        for (Dictionary& dictionary : dictionaries) {
            dictionary.clear();
        }

        // 首先尝试从资源加载
        Log::Info("尝试从DLL资源加载语言文件...");
        bool loadedFromResource = true;
        loadedFromResource &= LoadDictionaryFromResource(Language::Zh, IDR_I18N_ZH);
        loadedFromResource &= LoadDictionaryFromResource(Language::En, IDR_I18N_EN);
        loadedFromResource &= LoadDictionaryFromResource(Language::Jp, IDR_I18N_JP);
        loadedFromResource &= LoadDictionaryFromResource(Language::Ru, IDR_I18N_RU);

        if (!loadedFromResource) {
            Log::Warn("从资源加载i18n失败，尝试从文件系统加载");
            // 如果资源加载失败，回退到文件系统
            // 尝试多个可能的路径
            TryLoadAllFrom("data\\i18n\\");
            TryLoadAllFrom("XMenu\\data\\i18n\\");
            TryLoadAllFrom("src\\data\\i18n\\");
            
            // 获取当前模块所在目录
            char modulePath[MAX_PATH];
            HMODULE hModule = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCSTR)&Init, &hModule);
            if (hModule) {
                GetModuleFileNameA(hModule, modulePath, MAX_PATH);
                std::string moduleDir(modulePath);
                size_t lastSlash = moduleDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    moduleDir = moduleDir.substr(0, lastSlash + 1);
                    Log::Info(std::string("尝试从DLL目录加载: ") + moduleDir + "data\\i18n\\");
                    TryLoadAllFrom((moduleDir + "data\\i18n\\").c_str());
                }
            }
        }

        LoadFallbackZh();
        
        // 输出各语言字典的状态
        Log::Info(std::string("i18n 初始化完成 - 中文: ") + std::to_string(dictionaries[LanguageIndex(Language::Zh)].size()) + " 词条, " +
                  "英文: " + std::to_string(dictionaries[LanguageIndex(Language::En)].size()) + " 词条, " +
                  "日文: " + std::to_string(dictionaries[LanguageIndex(Language::Jp)].size()) + " 词条, " +
                  "俄文: " + std::to_string(dictionaries[LanguageIndex(Language::Ru)].size()) + " 词条");
    }

    void SetLanguage(Language language) {
        currentLanguage = language;
        Log::Info(std::string("语言已切换为: ") + GetLanguageName(language));
    }

    Language GetLanguage() {
        return currentLanguage;
    }

    const char* GetLanguageCode(Language language) {
        switch (language) {
        case Language::En: return "en";
        case Language::Jp: return "jp";
        case Language::Ru: return "ru";
        case Language::Zh:
        default: return "zh";
        }
    }

    const char* GetLanguageName(Language language) {
        switch (language) {
        case Language::En: return "English";
        case Language::Jp: return (const char*)u8"日本語";
        case Language::Ru: return (const char*)u8"Русский";
        case Language::Zh:
        default: return (const char*)u8"简体中文";
        }
    }

    const char* T(Language language, const char* key) {
        const Dictionary& dictionary = dictionaries[LanguageIndex(language)];
        const auto translated = dictionary.find(key);
        if (translated != dictionary.end()) {
            return translated->second.c_str();
        }

        const Dictionary& zh = dictionaries[LanguageIndex(Language::Zh)];
        const auto fallback = zh.find(key);
        if (fallback != zh.end()) {
            return fallback->second.c_str();
        }

        return key;
    }

    const char* T(const char* key) {
        const Dictionary& current = dictionaries[LanguageIndex(currentLanguage)];
        const auto translated = current.find(key);
        if (translated != current.end()) {
            return translated->second.c_str();
        }

        const Dictionary& zh = dictionaries[LanguageIndex(Language::Zh)];
        const auto fallback = zh.find(key);
        if (fallback != zh.end()) {
            return fallback->second.c_str();
        }

        // 如果找不到翻译，记录警告
        static std::unordered_map<std::string, bool> loggedKeys;
        if (loggedKeys.find(key) == loggedKeys.end()) {
            Log::Warn(std::string("未找到翻译键: ") + key + "，当前语言: " + GetLanguageName(currentLanguage));
            loggedKeys[key] = true;
        }
        return key;
    }
}