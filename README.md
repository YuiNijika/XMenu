# XMenu for GTA III / Vice City / San Andreas

A lightweight ASI menu for GTA III, GTA Vice City, and GTA San Andreas.  
基于 ASI 的轻量菜单，支持 GTA III、GTA Vice City 与 GTA San Andreas。

---

## 中文

### 功能

- 玩家：血量、护甲、金钱、通缉、无敌与状态开关
- 载具：生成、修复、车速、车门、损伤与载具状态
- 武器：武器获取、弹药与常用武器操作
- 世界：时间、天气、游戏速度与环境控制
- 传送：按城市/区域选择地点，支持多游戏数据
- 设置：内置中文、英文、日文、俄文本地化，切换后立即生效

### 支持游戏

| 游戏 | 渲染链路 | 说明 |
| --- | --- | --- |
| GTA III | D3D8to9 + D3D9 | 原生 D3D8，需要 D3D8to9 承接菜单渲染 |
| GTA Vice City | D3D8to9 + D3D9 | 原生 D3D8，需要 D3D8to9 承接菜单渲染 |
| GTA San Andreas | D3D9 | 原生可使用当前 D3D9 菜单链路 |

### 运行依赖

- [DirectX End-User Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=35)
- [Visual C++ Redistributable 2022 x86](https://aka.ms/vs/17/release/vc_redist.x86.exe)
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
- [SilentPatch](https://gtaforums.com/topic/669045-silentpatch/)（推荐；GTA III 可不装）
- [D3D8to9](https://github.com/crosire/d3d8to9/releases)（仅 GTA III / Vice City 需要）

### 安装

1. 安装运行依赖。
2. 将 `XMenu.asi` 放入游戏根目录。
3. GTA III / Vice City 额外放入 D3D8to9 的 `d3d8.dll`。
4. 启动游戏，在游戏内打开菜单。

### 构建

```bat
Build.bat Release --no-pause
```

构建产物为 x86 ASI 插件。项目数据和本地化 JSON 会被打包进 DLL 资源，文件系统数据仅作为回退。

---

## English

### Features

- Player: health, armor, money, wanted level, god mode, and state toggles
- Vehicle: spawn, repair, speed, doors, damage, and vehicle state controls
- Weapon: weapon grants, ammo, and common weapon actions
- World: time, weather, game speed, and environment controls
- Teleport: city/area based locations with per-game data
- Settings: built-in Chinese, English, Japanese, and Russian localization with instant switching

### Supported games

| Game | Rendering path | Notes |
| --- | --- | --- |
| GTA III | D3D8to9 + D3D9 | Native D3D8 game; D3D8to9 is required for the D3D9 menu renderer |
| GTA Vice City | D3D8to9 + D3D9 | Native D3D8 game; D3D8to9 is required for the D3D9 menu renderer |
| GTA San Andreas | D3D9 | Works with the current D3D9 menu renderer directly |

### Requirements

- [DirectX End-User Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=35)
- [Visual C++ Redistributable 2022 x86](https://aka.ms/vs/17/release/vc_redist.x86.exe)
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)
- [SilentPatch](https://gtaforums.com/topic/669045-silentpatch/) (recommended; optional for GTA III)
- [D3D8to9](https://github.com/crosire/d3d8to9/releases) (GTA III / Vice City only)

### Installation

1. Install the required runtimes and loaders.
2. Copy `XMenu.asi` into the game root directory.
3. For GTA III / Vice City, also copy D3D8to9 `d3d8.dll` into the game root directory.
4. Start the game and open the menu in-game.

### Build

```bat
Build.bat Release --no-pause
```

The output is an x86 ASI plugin. Data and localization JSON files are embedded into DLL resources, with loose files used only as fallback.
