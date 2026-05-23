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
        {"室内", "武器店", 1, 286.1490f, -40.6444f, 1001.5156f},
        {"室内", "洛圣都健身房", 5, 772.1120f, -3.8986f, 1000.7288f},
    };

    constexpr VehicleEntry kVehicles[] = {
        {"收藏", "Buffalo", 402},
    };
    constexpr std::size_t kVehicleCount = 0;
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
        {"重武器", "MAC", 283, true},
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
        {"武器店", "华盛顿武器店", 0, -65.0f, -1479.0f, 10.0f},
        {"武器店", "华盛顿海滩武器店", 0, 200.0f, -474.0f, 11.0f},
        {"常用", "维赛迪豪宅", 0, -382.0f, -537.0f, 17.0f},
        {"常用", "阳光车行", 0, -1008.0f, -872.0f, 12.0f},
        {"常用", "马里布俱乐部", 0, 496.0f, -84.0f, 10.0f},
        {"喷漆店", "海景酒店喷漆店", 0, -15.0f, -1258.0f, 10.0f},
        {"喷漆店", "维斯角喷漆店", 0, 330.0f, 429.0f, 11.0f},
    };

    constexpr VehicleEntry kVehicles[] = {
        {"船只", "Rio", 136}, {"船只", "Predator", 160}, {"船只", "Squalo", 176}, {"船只", "Speeder", 182}, {"船只", "Reefer", 183}, {"船只", "Tropic", 184}, {"船只", "Skimmer", 190}, {"船只", "Coastg", 202}, {"船只", "Dinghy", 203}, {"船只", "Marquis", 214}, {"船只", "Jetmax", 223},
        {"摩托", "Angel", 166}, {"摩托", "Pizzaboy", 178}, {"摩托", "PCJ 600", 191}, {"摩托", "Faggio", 192}, {"摩托", "Freeway", 193}, {"摩托", "Sanchez", 198},
        {"直升机", "Maverick", 217}, {"直升机", "VCN Maverick", 218},
        {"运输", "Taxi", 150}, {"运输", "Cabbie", 168}, {"运输", "Caddy", 187}, {"运输", "Zebra", 188}, {"运输", "Bus", 161}, {"运输", "Coach", 167}, {"运输", "Kaufman", 216},
        {"作业车", "Pony", 143}, {"作业车", "Mule", 144}, {"作业车", "Bobcat", 152}, {"作业车", "Yankee", 186}, {"作业车", "Walton", 208}, {"作业车", "Boxville", 228}, {"作业车", "Benson", 229},
        {"高性能", "Stinger", 132}, {"高性能", "Infernus", 141}, {"高性能", "Cheetah", 145}, {"高性能", "BF Injection", 154}, {"高性能", "Banshee", 159},
        {"遥控", "RC Bandit", 171}, {"遥控", "RC Baron", 194}, {"遥控", "RC Raider", 195}, {"遥控", "RC Goblin", 231},
        {"执法", "Police", 156}, {"执法", "Rhino", 162}, {"执法", "Barracks", 163}, {"执法", "Hunter", 155}, {"执法", "FBI Car", 147}, {"执法", "Enforcer", 157}, {"执法", "FBI Ranch", 220},
        {"特殊", "Firetruck", 137}, {"特殊", "Ambulance", 146}, {"特殊", "Mr. Whoopee", 153}, {"特殊", "Trashmaster", 138}, {"特殊", "Top Fun", 189}, {"特殊", "Sandking", 225}, {"特殊", "Love Fist", 201},
        {"赛车", "Hotring", 224}, {"赛车", "Hotring A", 232}, {"赛车", "Hotring B", 233}, {"赛车", "Bloodring A", 234}, {"赛车", "Bloodring B", 235},
        {"其它", "Landstalker", 130}, {"其它", "Idaho", 131}, {"其它", "Linerunner", 133}, {"其它", "Perennial", 134}, {"其它", "Sentinel", 135}, {"其它", "Stretch", 139}, {"其它", "Manana", 140}, {"其它", "Voodoo", 142}, {"其它", "Moonbeam", 148}, {"其它", "Esperanto", 149}, {"其它", "Washington", 151}, {"其它", "Securicar", 158}, {"其它", "Cuban Hermes", 164}, {"其它", "Stallion", 169}, {"其它", "Rumpo", 170}, {"其它", "Romero", 172}, {"其它", "Packer", 173}, {"其它", "Sentinel XS", 174}, {"其它", "Admiral", 175}, {"其它", "Sea Sparrow", 177}, {"其它", "Gang Burrito", 179}, {"其它", "Flatbed", 185}, {"其它", "Glendale", 196}, {"其它", "Oceanic", 197}, {"其它", "Sparrow", 199}, {"其它", "Patriot", 200}, {"其它", "Hermes", 204}, {"其它", "Sabre", 205}, {"其它", "Sabre Turbo", 206}, {"其它", "Phoenix", 207}, {"其它", "Regina", 209}, {"其它", "Comet", 210}, {"其它", "Deluxo", 211}, {"其它", "Burrito", 212}, {"其它", "Spand Express", 213}, {"其它", "Baggage", 215}, {"其它", "Rancher", 219}, {"其它", "Virgo", 221}, {"其它", "Greenwood", 222}, {"其它", "Blista Compact", 226}, {"其它", "Police Maverick", 227}, {"其它", "Mesa Grande", 230}, {"其它", "Vice Cheetah", 236},
    };
    constexpr std::size_t kVehicleCount = sizeof(kVehicles) / sizeof(kVehicles[0]);
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
        {"全部", "导弹发射器", 179, true},
        {"全部", "M16", 180, true},
        {"全部", "火焰喷射器", 181, true},
        {"全部", "炸弹", 182, true},
        {"全部", "手指", 183, true},
    };

    constexpr LocationEntry kLocations[] = {
        {"常用", "波特兰安全屋", 0, 885.97f, -309.27f, 8.64f},
        {"常用", "斯唐顿安全屋", 0, 103.0f, -478.5f, 15.93f},
        {"常用", "海岸之谷安全屋", 0, -666.75f, -1.75f, 18.86f},
        {"任务", "萨尔瓦多宅邸", 0, 1454.77f, -189.59f, 55.46f},
        {"任务", "波特兰 8-Ball 炸弹店", 0, 1274.13f, -95.89f, 14.89f},
    };

    constexpr VehicleEntry kVehicles[] = {
        {"执法", "Police", 116}, {"执法", "Enforcer", 117}, {"执法", "Rhino", 122}, {"执法", "FBI Car", 107}, {"执法", "Barracks", 123},
        {"飞机", "Dodo", 126},
        {"作业车", "Firetruck", 97}, {"作业车", "Ambulance", 106}, {"作业车", "Taxi", 110}, {"作业车", "Mr. Whoopee", 113}, {"作业车", "Bus", 121}, {"作业车", "Cabbie", 128}, {"作业车", "Borgnine", 148}, {"作业车", "Flatbed", 145}, {"作业车", "Yankee", 146}, {"作业车", "Coach", 127}, {"作业车", "Trashmaster", 98},
        {"船只", "Speeder", 142}, {"船只", "Reefer", 143}, {"船只", "Predator", 120},
        {"其它", "Landstalker", 90}, {"其它", "Idaho", 91}, {"其它", "Stinger", 92}, {"其它", "Linerunner", 93}, {"其它", "Perennial", 94}, {"其它", "Sentinel", 95}, {"其它", "Patriot", 96}, {"其它", "Stretch", 99}, {"其它", "Manana", 100}, {"其它", "Infernus", 101}, {"其它", "Blista", 102}, {"其它", "Pony", 103}, {"其它", "Mule", 104}, {"其它", "Cheetah", 105}, {"其它", "Moonbeam", 108}, {"其它", "Esperanto", 109}, {"其它", "Kuruma", 111}, {"其它", "Bobcat", 112}, {"其它", "BF Injection", 114}, {"其它", "Corpse", 115}, {"其它", "Securicar", 118}, {"其它", "Banshee", 119}, {"其它", "Stallion", 129}, {"其它", "Rumpo", 130}, {"其它", "RC Bandit", 131}, {"其它", "Bellyup", 132}, {"其它", "Mr. Wong's", 133}, {"其它", "Mafia Sentinel", 134}, {"其它", "Yardie Lobo", 135}, {"其它", "Yakuza Stinger", 136}, {"其它", "Diablo Stallion", 137}, {"其它", "Cartel Cruiser", 138}, {"其它", "Hoods Rumpo XL", 139}, {"其它", "Panlantic", 144}, {"其它", "Toyz", 149}, {"其它", "Ghost", 150},
    };
    constexpr std::size_t kVehicleCount = sizeof(kVehicles) / sizeof(kVehicles[0]);
#endif
}

WeaponTable GetWeapons() {
    return {kWeapons, sizeof(kWeapons) / sizeof(kWeapons[0])};
}

LocationTable GetLocations() {
    return {kLocations, sizeof(kLocations) / sizeof(kLocations[0])};
}

VehicleTable GetVehicles() {
    return {kVehicles, kVehicleCount};
}
}