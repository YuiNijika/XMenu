#pragma once

// 共享运行时与 mod 侧共用这份契约。
// 双方都只把它当作纯数据结构使用，不需要导入导出宏，
// mod 侧按名字取函数地址拿到函数表，共享库以 C 链接方式暴露唯一的入口函数

#include <cstdint>

#define XBASE_ABI_VERSION 2u
#define XBASE_GET_RUNTIME_NAME "xbaseGetRuntime"

// 只有共享运行时工程会定义 XBASE_RUNTIME_DLL，其余工程把入口当成普通声明
#if defined(XBASE_RUNTIME_DLL)
#define XBASE_ABI_EXPORT __declspec(dllexport)
#else
#define XBASE_ABI_EXPORT
#endif

extern "C" {

struct XBaseRuntime;

using XBaseGetRuntimeFn = const XBaseRuntime* (*)(std::uint32_t abiVersion);

// 版本不符或共享库未就绪时返回空指针，调用方据此提示用户，不要继续调用表内函数
XBASE_ABI_EXPORT const XBaseRuntime* xbaseGetRuntime(std::uint32_t abiVersion);

// 字段只能追加，禁止重排或改类型。新增一组能力时同时提升 XBASE_ABI_VERSION。
struct XBaseRuntime {
    std::uint32_t size;
    std::uint32_t abiVersion;

    // 生命周期，获取时递增引用计数，释放时递减，归零后不立即卸载，
    // 避免 mod 反复启停时重复创建 D3D 钩子
    int (*acquire)(const char* modName);
    int (*release)(const char* modName);

    // 路径查询，调用方提供缓冲区，返回写入字节数，缓冲区不足时返回需要的长度
    int (*gameDirectory)(char* buffer, std::uint32_t capacity);
    int (*xbaseDirectory)(char* buffer, std::uint32_t capacity);
    int (*modDirectory)(const char* modName, char* buffer, std::uint32_t capacity);
    int (*appDataDirectory)(char* buffer, std::uint32_t capacity);

    // 日志。level 取 0 信息 1 警告 2 错误
    void (*logInitForMod)(const char* modName);
    void (*logWrite)(int level, const char* message);
    void (*logShutdown)();

    // 配置。传入的键值由共享库复制，调用方可随时释放
    void (*configInitForMod)(const char* modName);
    int (*configGetString)(const char* key, char* buffer, std::uint32_t capacity);
    void (*configSetString)(const char* key, const char* value);
    int (*configGetInt)(const char* key, int defaultValue);
    void (*configSetInt)(const char* key, int value);
    int (*configGetBool)(const char* key, int defaultValue);
    void (*configSetBool)(const char* key, int value);
    void (*configSave)();

    // 宿主事件。回调在游戏逻辑线程触发，渲染回调里不要调用
    int (*hostInstall)(void (*onGameInit)(void*), void (*onProcess)(void*), void* userData);
    void (*hostShutdown)();
    void (*hostShowMessage)(const char* message);
    void (*hostQueueMessage)(const char* message);

    // 渲染与输入。共享库只钩一次 D3D，所有 mod 共用同一套键态
    int (*hooksInit)();
    void (*hooksShutdown)();
    std::uint64_t (*registerDrawCallback)(void (*draw)(void*), void* userData);
    void (*unregisterDrawCallback)(std::uint64_t callbackId);
    void (*setMenuVisible)(int visible);
    int (*isMenuVisible)();
    void (*maintainInputState)();
    float (*getFrameDeltaSeconds)();

    // 键码沿用 Input::Key 的数值，避免 mod 侧再映射一次
    int (*isKeyDown)(int key);
    int (*wasKeyPressed)(int key);
    void (*pollSystemKeys)();

    // 世界就绪。Core 的分发由共享库内部驱动，mod 不要再自己调 Process
    int (*isWorldReady)();

    // 界面。v1 只导出显示型 mod 需要的子集，其余界面能力后续追加。
    // 颜色统一按 0xAARRGGBB 打包
    void (*setNextWindowPosition)(float x, float y);
    void (*getDisplaySize)(float* width, float* height);
    void (*text)(const char* value);
    void (*textFormatted)(const char* format, float value);
    void (*textDisabled)(const char* value);
    void (*separator)();
    void (*sameLine)();
    void (*drawLine)(float x1, float y1, float x2, float y2, std::uint32_t color, float thickness);
    void (*drawRect)(float x1, float y1, float x2, float y2, std::uint32_t color, float thickness);
    void (*drawRectFilled)(float x1, float y1, float x2, float y2, std::uint32_t color);
    void (*drawText)(float x, float y, std::uint32_t color, const char* value);

    // v2 追加。版本查询。字符串写入调用方缓冲区并返回需要的长度，
    // 缓冲区不足时只报长度不写入；数字编码与 Version.h 的 kVersionNumber 同一约定。
    // mod 判断最低运行环境用 versionNumber，不要解析字符串
    int (*versionString)(char* buffer, std::uint32_t capacity);
    std::uint32_t (*versionNumber)();
};

} // extern "C"
