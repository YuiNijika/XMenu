#include "ResourceData.h"

#ifdef GTASA
#include "eWeaponType.h"
#endif

namespace Resources {
namespace {
#ifdef GTASA
    const WeaponEntry kWeapons[] = {
        {"近战", "指虎", WEAPONTYPE_BRASSKNUCKLE, false},
        {"近战", "高尔夫球杆", WEAPONTYPE_GOLFCLUB, false},
        {"近战", "警棍", WEAPONTYPE_NIGHTSTICK, false},
        {"近战", "小刀", WEAPONTYPE_KNIFE, false},
        {"近战", "棒球棍", WEAPONTYPE_BASEBALLBAT, false},
        {"近战", "铁铲", WEAPONTYPE_SHOVEL, false},
        {"近战", "台球杆", WEAPONTYPE_POOLCUE, false},
        {"近战", "武士刀", WEAPONTYPE_KATANA, false},
        {"近战", "电锯", WEAPONTYPE_CHAINSAW, false},
        {"近战", "紫色玩具", WEAPONTYPE_DILDO1, false},
        {"近战", "白色玩具", WEAPONTYPE_DILDO2, false},
        {"近战", "白色震动棒", WEAPONTYPE_VIBE1, false},
        {"近战", "银色震动棒", WEAPONTYPE_VIBE2, false},
        {"近战", "鲜花", WEAPONTYPE_FLOWERS, false},
        {"近战", "手杖", WEAPONTYPE_CANE, false},
        {"投掷", "手雷", WEAPONTYPE_GRENADE, false},
        {"投掷", "催泪瓦斯", WEAPONTYPE_TEARGAS, false},
        {"投掷", "燃烧瓶", WEAPONTYPE_MOLOTOV, false},
        {"手枪", "手枪", WEAPONTYPE_PISTOL, false},
        {"手枪", "消音手枪", WEAPONTYPE_PISTOL_SILENCED, false},
        {"手枪", "沙漠之鹰", WEAPONTYPE_DESERT_EAGLE, false},
        {"霰弹枪", "霰弹枪", WEAPONTYPE_SHOTGUN, false},
        {"霰弹枪", "短管霰弹枪", WEAPONTYPE_SAWNOFF, false},
        {"霰弹枪", "战斗霰弹枪", WEAPONTYPE_SPAS12, false},
        {"冲锋枪", "Uzi", WEAPONTYPE_MICRO_UZI, false},
        {"冲锋枪", "MP5", WEAPONTYPE_MP5, false},
        {"步枪", "AK47", WEAPONTYPE_AK47, false},
        {"步枪", "M4", WEAPONTYPE_M4, false},
        {"步枪", "Tec-9", WEAPONTYPE_TEC9, false},
        {"步枪", "步枪", WEAPONTYPE_COUNTRYRIFLE, false},
        {"步枪", "狙击枪", WEAPONTYPE_SNIPERRIFLE, false},
        {"重武器", "火箭筒", WEAPONTYPE_RLAUNCHER, false},
        {"重武器", "热追踪火箭筒", WEAPONTYPE_RLAUNCHER_HS, false},
        {"重武器", "火焰喷射器", WEAPONTYPE_FTHROWER, false},
        {"重武器", "加特林", WEAPONTYPE_MINIGUN, false},
        {"工具", "遥控炸弹", WEAPONTYPE_SATCHEL_CHARGE, false},
        {"工具", "引爆器", WEAPONTYPE_DETONATOR, false},
        {"工具", "喷漆罐", WEAPONTYPE_SPRAYCAN, false},
        {"工具", "灭火器", WEAPONTYPE_EXTINGUISHER, false},
        {"工具", "相机", WEAPONTYPE_CAMERA, false},
        {"工具", "夜视仪", WEAPONTYPE_NIGHTVISION, false},
        {"工具", "热成像", WEAPONTYPE_INFRARED, false},
        {"工具", "降落伞", WEAPONTYPE_PARACHUTE, false},
        {"特殊", "手机", static_cast<unsigned int>(-2), false},
        {"特殊", "喷气背包", static_cast<unsigned int>(-1), false},
    };

    constexpr LocationEntry kLocations[] = {
        {"常用", "葛洛夫街", 0, 2493.0f, -1667.0f, 13.34f},
        {"常用", "废弃机场", 0, 424.0f, 2533.0f, 16.6f},
        {"室内", "CJ 的家", 3, 2496.0498f, -1695.2382f, 1014.7422f},
        {"室内", "Ammunation 1", 1, 286.1490f, -40.6444f, 1001.5156f},
        {"室内", "洛圣都健身房", 5, 772.1120f, -3.8986f, 1000.7288f},
    };
#elif GTAVC
    const WeaponEntry kWeapons[] = {
        {"近战", "指虎", 259, true},
        {"近战", "螺丝刀", 260, true},
        {"近战", "高尔夫球杆", 261, true},
        {"近战", "警棍", 262, true},
        {"近战", "小刀", 263, true},
        {"近战", "棒球棍", 264, true},
        {"近战", "锤子", 265, true},
        {"近战", "菜刀", 266, true},
        {"近战", "砍刀", 267, true},
        {"近战", "武士刀", 268, true},
        {"近战", "电锯", 269, true},
        {"投掷", "手雷", 270, true},
        {"投掷", "催泪瓦斯", 271, true},
        {"投掷", "燃烧瓶", 272, true},
        {"投掷", "火箭筒", 287, true},
        {"投掷", "火焰喷射器", 288, true},
        {"霰弹枪", "短管霰弹枪", 279, true},
        {"霰弹枪", "战斗霰弹枪", 278, true},
        {"机枪", "M4", 280, true},
        {"机枪", "Tec-9", 281, true},
        {"机枪", "Uzi", 282, true},
        {"机枪", "手枪", 274, true},
        {"狙击", "狙击枪", 285, true},
        {"狙击", "激光狙击枪", 286, true},
        {"重武器", "消音 Ingram", 283, true},
        {"重武器", "MP5", 284, true},
        {"重武器", "加特林", 290, true},
        {"重武器", "M60", 289, true},
        {"杂项", "手机", 258, true},
        {"杂项", "左轮", 275, true},
        {"杂项", "Ruger", 276, true},
        {"杂项", "霰弹枪", 277, true},
        {"杂项", "炸弹", 291, true},
        {"杂项", "相机", 292, true},
        {"杂项", "手指", 293, true},
    };

    constexpr LocationEntry kLocations[] = {
        {"Ammunation", "Washington Ammunation", 0, -65.0f, -1479.0f, 10.0f},
        {"Ammunation", "Washington Beach Ammunation", 0, 200.0f, -474.0f, 11.0f},
        {"常用", "维赛迪豪宅", 0, -382.0f, -537.0f, 17.0f},
        {"常用", "阳光车行", 0, -1008.0f, -872.0f, 12.0f},
        {"常用", "马里布俱乐部", 0, 496.0f, -84.0f, 10.0f},
        {"喷漆店", "Oceanview Pay n Spray", 0, -15.0f, -1258.0f, 10.0f},
        {"喷漆店", "Vice Point Pay n Spray", 0, 330.0f, 429.0f, 11.0f},
    };
#else
    const WeaponEntry kWeapons[] = {
        {"全部", "手雷", 170, true},
        {"全部", "AK47", 171, true},
        {"全部", "棒球棍", 172, true},
        {"全部", "手枪", 173, true},
        {"全部", "燃烧瓶", 174, true},
        {"全部", "火箭筒", 175, true},
        {"全部", "霰弹枪", 176, true},
        {"全部", "狙击枪", 177, true},
        {"全部", "Uzi", 178, true},
        {"全部", "导弹", 179, true},
        {"全部", "M16", 180, true},
        {"全部", "火焰喷射器", 181, true},
        {"全部", "炸弹", 182, true},
        {"全部", "手指", 183, true},
    };

    constexpr LocationEntry kLocations[] = {
        {"常用", "Portland - Save House", 0, 885.97f, -309.27f, 8.64f},
        {"常用", "Staunton - Save House", 0, 103.0f, -478.5f, 15.93f},
        {"常用", "Shoreside Vale - Save House", 0, -666.75f, -1.75f, 18.86f},
        {"任务", "Salvatore Mansion", 0, 1454.77f, -189.59f, 55.46f},
        {"任务", "8Ball Bomb Shop Portland", 0, 1274.13f, -95.89f, 14.89f},
    };
#endif
}

WeaponTable GetWeapons() {
    return {kWeapons, sizeof(kWeapons) / sizeof(kWeapons[0])};
}

LocationTable GetLocations() {
    return {kLocations, sizeof(kLocations) / sizeof(kLocations[0])};
}
}