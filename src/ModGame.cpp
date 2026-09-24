// 单文件 asi 导出目标游戏，供 Bootstrap 做非本游戏的静默跳过守卫
// 多个 asi 放同一目录时，另外两个游戏的 asi 不会误初始化，也不会弹出错误框
// 目标游戏由 premake 在编译期注入

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
