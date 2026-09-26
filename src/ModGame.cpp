// 单文件 asi 导出目标游戏与载荷基名，供 Bootstrap 做非本游戏的静默跳过守卫与目录推导
// 多个 asi 放同一目录时，另外两个游戏的 asi 不会误初始化，也不会弹出错误框
// 目标游戏由 premake 在编译期注入，基名固定为 XMenu 使三个版本共用同一份数据与清单

#if defined(GTASA)
#define XBASE_MOD_GAME "SA"
#elif defined(GTAVC)
#define XBASE_MOD_GAME "VC"
#elif defined(GTA3)
#define XBASE_MOD_GAME "III"
#else
#define XBASE_MOD_GAME ""
#endif

// 入口必须导出，否则 Bootstrap 读不到目标游戏，静默跳过守卫会失效
extern "C" __declspec(dllexport) const char* XBaseModTargetGame() {
    return XBASE_MOD_GAME;
}

// 基名必须导出，否则 Bootstrap 按 asi 文件名推导成 XMenuVC 这类带版本后缀的名字，
// 清单校验会去 XBase\Mods\XMenuVC\ 找 package.json 并顺手建出一个空目录
extern "C" __declspec(dllexport) const char* XBasePayloadBaseName() {
    return "XMenu";
}
