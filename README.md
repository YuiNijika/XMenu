<div align="center">

# XMenu for GTA III / Vice City / San Andreas

一个简单但功能比较全面的 GTA 三部曲 ASI 菜单。  
By **鼠子(YuiNijika)**

A simple yet feature-rich ASI menu for GTA III, GTA Vice City, and GTA San Andreas.

[![GitHub](https://img.shields.io/badge/GitHub-XMenu-181717?style=for-the-badge&logo=github)](https://github.com/YuiNijika/XMenu)
[![GTAMODX](https://img.shields.io/badge/GTAMODX-XMenu-2ea44f?style=for-the-badge)](https://gtamodx.com/mods/xmenu)
[![Bilibili](https://img.shields.io/badge/Bilibili-Tomoriゞ-00A1D6?style=for-the-badge&logo=bilibili&logoColor=white)](https://space.bilibili.com/435502585)

![XMenu](./images/poster.webp)

</div>

---

## 中文

> 默认快捷键：`M` 呼出 / 关闭菜单

### 这是什么

XMenu 是一个面向 GTA III、GTA Vice City、GTA San Andreas 的 ASI 菜单，目标是安装简单、功能直观、三端共用同一套使用体验。

它不是大型整合包，也不会替换游戏主程序。把文件放到游戏目录后，通过 ASI Loader 加载即可使用，也可使用仓库附带的安装器一键部署。

当前版本见源码 `XMENU_VERSION`。游戏内与安装器均会检测新版本。

### 功能概览

| 模块 | 功能 |
| --- | --- |
| 玩家 | 血量、护甲、金钱、通缉等级、无敌、无限奔跑、自由飞行、状态开关 |
| 载具 | 生成载具、修复、车速、车门、损伤、霓虹、载具状态 |
| 武器 | 获取武器、弹药、安全模式、滚轮刷武器、自定义射速、线框与骨骼绘制、子弹追踪、子弹穿墙 |
| 世界 | 时间、天气、游戏速度、环境控制、自由相机、俯视相机、随机作弊 |
| 传送 | 城市 / 区域选点、快速地图 / 标记、向前传送、持续向前传送 |
| 行人 | 行人生成与数据列表、无火控制、刷新数量限制 |
| 场景 | 动画、粒子、过场 |
| 视觉 | 视觉 / 天气选项 |
| 界面 | 面板模式、列表模式、数值弹窗输入、全局主题色 |
| 快捷键 | 菜单快捷键、动作快捷键、Overlay、命令窗口 |
| 设置 | 中英日俄本地化、配置导入导出、Overlay、启动状态恢复 |
| 更新 | 双源检测：优先 [GTAMODX](https://gtamodx.com/mods/xmenu)，失败回退 GitHub Releases |

#### 武器辅助

- **绘制**：行人 / 车辆包围盒、ColModel 碰撞线框、行人骨骼
- **子弹追踪**：锁定范围与最大目标数可配，只改子弹落点，**不会转动相机**
- **子弹穿墙**：本地玩家开火时忽略建筑等遮挡
- **自定义射速**：与快速连射相互独立，可写入配置
- **安全模式**：默认开启，拦截无效武器 ID
- GTA III 暂为占位，完整追踪以 SA / VC 为准

#### 界面模式

- **面板模式**：传统窗口布局
- **列表模式**：上下选择，支持键盘、滚轮与鼠标，数值可弹窗输入，长列表可折叠
- 两种模式共用主题色，设置页可切换

#### San Andreas

部分能力主要面向 SA，例如载具霓虹、自由相机、俯视相机、随机作弊、场景粒子与过场。III / VC 以各端实际可用功能为准。

#### 三端能力差异

| 能力 | SA | VC | III |
| --- | :-: | :-: | :-: |
| 玩家基础（血量 / 护甲 / 金钱 / 无敌 / 奔跑等） | ✅ | ✅ | ✅ |
| 武器（获取 / 丢弃 / 射速 / 属性覆盖） | ✅ | ✅ | ✅ |
| 世界（时间 / 天气 / 重力 / 游戏速度 / FPS / 冻结时间 / 快时钟 / 禁用回放与作弊） | ✅ | ✅ | ✅ |
| 传送 | ✅ | ✅ | ✅ |
| 视觉（HUD / 雷达 / 天气滤镜） | ✅ | ✅ | ✅ |
| 行人（生成 / 属性 / 大头） | ✅ | ✅ | ✅ |
| 子弹追踪 / 穿墙 | ✅ | ✅ | ✖ |
| 行人 / 车辆碰撞线框与骨骼绘制 | ◐ | ◐ | ✖ |
| 世界拾取（生成拾取物） | ◐ | ◐ | ◐ |
| 玩家运行时效果 / 载具基础与配色 | ✅ | ◐ | ◐ |
| 载具霓虹、自由 / 俯视相机、随机作弊、场景粒子与过场 | ✅ | ✖ | ✖ |

> GTA III 的子弹追踪为占位；VC / III 中标记 ◐ 的能力页面可用但部分动作受限。

### 支持游戏

| 游戏 | 渲染链路 | 说明 |
| --- | --- | --- |
| GTA III | D3D8to9 + D3D9 | 需要 D3D8to9 |
| GTA Vice City | D3D8to9 + D3D9 | 需要 D3D8to9 |
| GTA San Andreas | D3D9 | 原生 D3D9 |

### 运行依赖

- [DirectX End-User Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=35)
- [Visual C++ Redistributable 2022 x86](https://aka.ms/vs/17/release/vc_redist.x86.exe)
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
- [SilentPatch](https://gtaforums.com/topic/669045-silentpatch/) 推荐
- [D3D8to9](https://github.com/crosire/d3d8to9/releases) 仅 GTA III / Vice City

### 安装

#### 方式 A：安装器

1. 准备好游戏本体。
2. 运行 `XMenuInstaller.exe`，按向导选择目录与组件。
3. 可附带部署 ASI Loader、SilentPatch、D3D8to9，也可只装 XMenu。
4. 版本信息优先读 GTAMODX，安装包来自 GitHub Release。
5. 启动游戏，按 `M` 打开菜单。

#### 方式 B：手动复制

1. 安装运行依赖。
2. 将 `XMenu.asi` 放入游戏根目录。
3. 将 `XMenu` 文件夹放入游戏根目录。
4. GTA III / Vice City 额外放入 `d3d8.dll`。
5. 启动游戏，按 `M` 打开菜单。

推荐目录结构：

```text
GameRoot/
├─ XMenu.asi
├─ XMenu/
│  ├─ XMenuSA.dll
│  ├─ XMenuVC.dll
│  ├─ XMenuIII.dll
│  ├─ data/
│  │  ├─ sa/
│  │  ├─ vc/
│  │  ├─ iii/
│  │  └─ i18n/
│  └─ i18n/
└─ d3d8.dll
```

运行后会生成：

```text
XMenu/config.json
XMenu/debug.log
```

`config.json` 保存菜单状态、快捷键、武器辅助与射速等。首次启动会分帧完成 i18n、配置、逻辑与 D3D Hook；资源 JSON 按需加载。

### 如何增加新语言

语言文件位于：

```text
XMenu/data/i18n/<language>/
```

源码目录：

```text
src/data/i18n/<language>/
```

新增语言示例：韩文 `ko`。

1. 复制已有语言目录：

```text
src/data/i18n/en/ -> src/data/i18n/ko/
```

2. 修改 `src/data/i18n/ko/index.json`。

```json
{
  "name": "한국어",
  "files": [
    "common.json",
    "player.json",
    "vehicle.json",
    "weapon.json",
    "world.json",
    "teleport.json",
    "ped.json",
    "scene.json",
    "visual.json",
    "actions.json",
    "settings.json",
    "update.json",
    "about.json",
    "scene_visual_data.json"
  ]
}
```

3. 翻译 JSON 的**值**，不要改 key。

```json
{
  "tab.player": "플레이어"
}
```

4. 重新构建，或把语言目录放到：

```text
GameRoot/XMenu/data/i18n/ko/
```

5. 启动游戏，在设置页切换语言。

缺失 key 会回退到备用语言或显示原 key。

### 构建

- Visual Studio，MSVC C++，Win32
- [plugin-sdk](https://github.com/DK22Pac/plugin-sdk)，环境变量 `PLUGIN_SDK_DIR` 或上级目录自动探测
- 仓库内 `tools/premake5.exe`

```bat
Build.bat Release --no-pause
```

可选：`Debug` / `Release`、`--toolset v143|v145`、`--no-pause`。  
首次可先运行 `Setup.bat`。

构建产物：

```text
build/bin/XMenu.asi
build/bin/XMenuInstaller.exe
build/bin/XMenu/XMenuSA.dll
build/bin/XMenu/XMenuVC.dll
build/bin/XMenu/XMenuIII.dll
build/bin/XMenu/data/**
build/bin/XMenu/data/i18n/**
build/bin/XMenu/i18n/**
```

| 路径 | 说明 |
| --- | --- |
| `tools/build_i18n_split.py` | 拆分 / 整理 i18n |
| `tools/dataEditor/` | Python 数据编辑工具 |
| `tools/build_plugin_sdk.bat` | 构建 plugin-sdk |
| `tools/resolve_vc_toolset.ps1` | 解析 MSVC 工具集 |

改完 `src/data/` 后重新构建，或拷贝 `data/` 到运行目录。

### 目录结构

```text
XMenu/
├─ src/
│  ├─ controllers/
│  ├─ data/i18n|sa|vc|iii/
│  └─ main.cpp
├─ installer/
├─ include/
├─ tools/
├─ Build.bat / Setup.bat / premake5.lua
└─ images/
```

### 反馈与关注

- GitHub：[YuiNijika/XMenu](https://github.com/YuiNijika/XMenu)
- GTAMODX：[XMenu 发布页](https://gtamodx.com/mods/xmenu)
- Bilibili：[Tomoriゞ](https://space.bilibili.com/435502585)
- QQ 群：[GTAMODX QQ 群](https://gtamodx.com/qqun)

> XMenu 免费发布，禁止倒卖，禁止用于商业用途。

---

## English

> Default hotkey: `M` to open / close the menu

### About

XMenu is a simple yet feature-rich ASI menu for GTA III, GTA Vice City, and GTA San Andreas.  
It is designed to be easy to install, easy to use, and consistent across all three games.

You can install manually or use the bundled installer.  
Version string is `XMENU_VERSION` in source. In-game and installer update checks are available.

Author: **鼠子(YuiNijika)**

### Features

- Player: health, armor, money, wanted level, god mode, free fly, state toggles
- Vehicle: spawn, repair, speed, doors, damage, neon, vehicle state
- Weapon: grants, ammo, safe mode, scroll-wheel cycling, custom fire rate, wireframe and skeleton ESP, bullet tracking, wallhack
- World: time, weather, game speed, environment, freecam, top-down cam, random cheats
- Teleport: city / area locations, quick map / marker, forward teleport, continuous forward teleport
- Ped: spawn tools, data lists, no-fire control, spawn population limits
- Scene: animation, particle, cutscene tools
- Visual: visual / weather options
- UI: panel mode, list mode, popup numeric entry, shared themes
- Hotkeys: menu hotkey, action hotkeys, overlay, command window
- Settings: zh / en / jp / ru, config import/export, overlay, startup restore
- Updates: prefer [GTAMODX](https://gtamodx.com/mods/xmenu), fall back to GitHub Releases

#### Weapon assist

- **Drawing**: ped / vehicle bounds, ColModel wireframes, ped skeleton
- **Bullet tracking**: configurable lock range and max targets; redirects bullet destination only, **does not rotate the camera**
- **Wallhack**: ignores buildings on the local player fire path
- **Custom fire rate**: independent from rapid-fire; can be saved to config
- **Safe mode**: blocks invalid weapon IDs by default
- GTA III is a stub for now; full tracking is for SA / VC

#### UI modes

- **Panel mode**: classic window layout
- **List mode**: list navigation with keyboard, wheel, and mouse; popup numeric entry; collapsible groups
- Shared theme palette in settings

#### San Andreas

Neon, freecam, top-down camera, random cheats, and many scene tools are SA-oriented. III / VC expose what each build implements.

#### Per-game capability differences

| Capability | SA | VC | III |
| --- | :-: | :-: | :-: |
| Player basics (health / armor / money / god mode / sprint, etc.) | ✅ | ✅ | ✅ |
| Weapon (grant / drop / fire rate / stat overrides) | ✅ | ✅ | ✅ |
| World (time / weather / gravity / game speed / FPS / freeze time / faster clock / disable replay & cheats) | ✅ | ✅ | ✅ |
| Teleport | ✅ | ✅ | ✅ |
| Visual (HUD / radar / weather filter) | ✅ | ✅ | ✅ |
| Ped (spawn / attributes / big head) | ✅ | ✅ | ✅ |
| Bullet tracking / wallhack | ✅ | ✅ | ✖ |
| Ped / vehicle collision wireframe & skeleton drawing | ◐ | ◐ | ✖ |
| World pickups (spawn pickup) | ◐ | ◐ | ◐ |
| Player runtime effects / vehicle basics & colors | ✅ | ◐ | ◐ |
| Vehicle neon, freecam / top-down camera, random cheats, scene particles & cutscenes | ✅ | ✖ | ✖ |

> GTA III bullet tracking is a stub; on VC / III, ◐ means the page is available but some actions are limited.

### Supported games

| Game | Rendering path | Notes |
| --- | --- | --- |
| GTA III | D3D8to9 + D3D9 | D3D8to9 required |
| GTA Vice City | D3D8to9 + D3D9 | D3D8to9 required |
| GTA San Andreas | D3D9 | Native D3D9 |

### Requirements

- [DirectX End-User Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=35)
- [Visual C++ Redistributable 2022 x86](https://aka.ms/vs/17/release/vc_redist.x86.exe)
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
- [SilentPatch](https://gtaforums.com/topic/669045-silentpatch/) recommended
- [D3D8to9](https://github.com/crosire/d3d8to9/releases) GTA III / Vice City only

### Installation

#### Option A: Installer

1. Prepare the game install.
2. Run `XMenuInstaller.exe` and follow the wizard.
3. Optional components: ASI Loader, SilentPatch, D3D8to9, or XMenu only.
4. Version info prefers GTAMODX; packages come from GitHub Releases.
5. Start the game and press `M`.

#### Option B: Manual copy

1. Install requirements.
2. Copy `XMenu.asi` into the game root.
3. Copy the `XMenu` folder into the game root.
4. For GTA III / Vice City, also copy `d3d8.dll`.
5. Start the game and press `M`.

Recommended layout:

```text
GameRoot/
├─ XMenu.asi
├─ XMenu/
│  ├─ XMenuSA.dll
│  ├─ XMenuVC.dll
│  ├─ XMenuIII.dll
│  ├─ data/
│  │  ├─ sa/
│  │  ├─ vc/
│  │  ├─ iii/
│  │  └─ i18n/
│  └─ i18n/
└─ d3d8.dll
```

Runtime files:

```text
XMenu/config.json
XMenu/debug.log
```

`config.json` stores menu state, hotkeys, weapon assist, fire rate, and more.  
Startup is staged across frames; resource JSON loads on demand.

### Add a new language

Language files:

```text
XMenu/data/i18n/<language>/
```

Source:

```text
src/data/i18n/<language>/
```

Copy an existing language folder, update `index.json`, and translate JSON **values** only. Keep keys unchanged. Then rebuild or copy into the game folder.

### Build

Visual Studio, MSVC, Win32; [plugin-sdk](https://github.com/DK22Pac/plugin-sdk) via `PLUGIN_SDK_DIR`; `tools/premake5.exe`.

```bat
Build.bat Release --no-pause
```

Optional: `Debug` / `Release`, `--toolset v143|v145`, `--no-pause`.  
`Setup.bat` helps with first-time setup.

Output:

```text
build/bin/XMenu.asi
build/bin/XMenuInstaller.exe
build/bin/XMenu/XMenuSA.dll
build/bin/XMenu/XMenuVC.dll
build/bin/XMenu/XMenuIII.dll
build/bin/XMenu/data/**
build/bin/XMenu/data/i18n/**
build/bin/XMenu/i18n/**
```

| Path | Purpose |
| --- | --- |
| `tools/build_i18n_split.py` | Maintain i18n data |
| `tools/dataEditor/` | Python data editor |
| `tools/build_plugin_sdk.bat` | Build plugin-sdk |
| `tools/resolve_vc_toolset.ps1` | Resolve MSVC toolset |

### Source layout

```text
XMenu/
├─ src/
│  ├─ controllers/
│  ├─ data/i18n|sa|vc|iii/
│  └─ main.cpp
├─ installer/
├─ include/
├─ tools/
├─ Build.bat / Setup.bat / premake5.lua
└─ images/
```

### Links

- GitHub: [YuiNijika/XMenu](https://github.com/YuiNijika/XMenu)
- GTAMODX: [XMenu page](https://gtamodx.com/mods/xmenu)
- Bilibili: [Tomoriゞ](https://space.bilibili.com/435502585)
- QQ Group: [GTAMODX QQ Group](https://gtamodx.com/qqun)

> XMenu is free. Reselling or commercial redistribution is not allowed.