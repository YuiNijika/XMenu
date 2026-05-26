<div align="center">

# XMenu for GTA III / Vice City / San Andreas

一个简单但功能比较全面的 GTA 三部曲 ASI 菜单。  
By **鼠子(YuiNijika)**

A simple yet feature-rich ASI menu for GTA III, GTA Vice City, and GTA San Andreas.

[![GitHub](https://img.shields.io/badge/GitHub-XMenu-181717?style=for-the-badge&logo=github)](https://github.com/YuiNijika/XMenu)
[![GTAMODX](https://img.shields.io/badge/GTAMODX-XMenu-2ea44f?style=for-the-badge)](https://gtamodx.com/mods/xmenu)
[![Bilibili](https://img.shields.io/badge/Bilibili-Tomoriゞ-00A1D6?style=for-the-badge&logo=bilibili&logoColor=white)](https://space.bilibili.com/435502585)

</div>

---

## 中文

> 默认快捷键：`M` 呼出 / 关闭菜单

### 这是什么

XMenu 是一个面向 GTA III、GTA Vice City、GTA San Andreas 的 ASI 菜单，目标是安装简单、功能直观、三端共用同一套使用体验。

它不是大型整合包，也不会替换游戏主程序。把文件放到游戏目录后，通过 ASI Loader 加载即可使用。

### 功能概览

| 模块 | 功能 |
| --- | --- |
| 玩家 | 血量、护甲、金钱、通缉等级、无敌、无限奔跑、状态开关 |
| 载具 | 生成载具、修复载具、车速控制、车门、损伤、载具状态 |
| 武器 | 获取武器、弹药、常用武器操作 |
| 世界 | 时间、天气、游戏速度、环境控制 |
| 传送 | 按城市 / 区域选择地点，支持三端独立数据 |
| 行人 | 行人相关功能与数据列表 |
| 场景 | 动画、粒子、过场相关功能；当前主要面向 San Andreas |
| 视觉 | 视觉 / 天气相关选项，支持 III / VC / SA |
| 快捷键 | 菜单快捷键、动作快捷键、Overlay、命令窗口 |
| 设置 | 中文、英文、日文、俄文，本地化切换立即生效 |

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

1. 安装上面的运行依赖。
2. 将 `XMenu.asi` 放入游戏根目录。
3. 将 `XMenu` 文件夹放入游戏根目录。
4. GTA III / Vice City 额外放入 D3D8to9 的 `d3d8.dll`。
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
│  └─ i18n/              # 兼容旧路径，可选
└─ d3d8.dll              # 仅 GTA III / Vice City 需要
```

运行后会在游戏目录下生成：

```text
XMenu/config.json
XMenu/debug.log
```

### 如何增加新语言

XMenu 的当前语言文件位于：

```text
XMenu/data/i18n/<language>/
```

源码内对应目录为：

```text
src/data/i18n/<language>/
```

新增语言示例：假设要增加韩文 `ko`。

1. 复制一个已有语言目录，例如：

```text
src/data/i18n/en/ -> src/data/i18n/ko/
```

2. 修改 `src/data/i18n/ko/index.json`。

通常需要确认：

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

3. 翻译 `ko` 目录下的各个 JSON 文件。

只改右侧文本，不改左侧 key：

```json
{
  "tab.player": "Player"
}
```

应该改成：

```json
{
  "tab.player": "플레이어"
}
```

不要改成：

```json
{
  "탭.플레이어": "플레이어"
}
```

4. 重新构建或把语言目录放到运行目录。

开发环境构建：

```bat
Build.bat Release --no-pause
```

玩家本地临时添加语言：

```text
GameRoot/XMenu/data/i18n/ko/
```

5. 启动游戏，在设置页切换语言。

如果某个 key 没翻译，XMenu 会回退到备用语言或显示原 key，方便定位缺失项。

### 构建

```bat
Build.bat Release --no-pause
```

构建产物：

```text
build/bin/XMenu.asi
build/bin/XMenu/XMenuSA.dll
build/bin/XMenu/XMenuVC.dll
build/bin/XMenu/XMenuIII.dll
build/bin/XMenu/data/**
build/bin/XMenu/data/i18n/**
build/bin/XMenu/i18n/**
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

Author: **鼠子(YuiNijika)**

### Features

- Player: health, armor, money, wanted level, god mode, and state toggles
- Vehicle: spawning, repair, speed, doors, damage, and vehicle state controls
- Weapon: weapon grants, ammo, and common weapon actions
- World: time, weather, game speed, and environment controls
- Teleport: city / area based locations with per-game data
- Ped: ped-related tools and data lists
- Scene: animation, particle, and cutscene tools, mainly for San Andreas
- Visual: visual / weather options for III / VC / SA
- Hotkeys: menu hotkey, action hotkeys, overlay, and command window
- Settings: Chinese, English, Japanese, and Russian localization with instant switching

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
- [SilentPatch](https://gtaforums.com/topic/669045-silentpatch/) recommended, optional for GTA III
- [D3D8to9](https://github.com/crosire/d3d8to9/releases) for GTA III / Vice City only

### Installation

1. Install the required runtimes and loaders.
2. Copy `XMenu.asi` into the game root directory.
3. Copy the `XMenu` folder into the game root directory.
4. For GTA III / Vice City, also copy D3D8to9 `d3d8.dll` into the game root directory.
5. Start the game and press `M` in-game.

### Add a new language

Language files are stored in:

```text
XMenu/data/i18n/<language>/
```

Source files are stored in:

```text
src/data/i18n/<language>/
```

To add a new language, copy an existing language folder, rename it, update its `index.json`, and translate the JSON values. Keep all JSON keys unchanged.

Example:

```json
{
  "tab.player": "Player"
}
```

Only translate the value:

```json
{
  "tab.player": "플레이어"
}
```

Then rebuild:

```bat
Build.bat Release --no-pause
```

### Build

```bat
Build.bat Release --no-pause
```

Output layout:

```text
build/bin/XMenu.asi
build/bin/XMenu/XMenuSA.dll
build/bin/XMenu/XMenuVC.dll
build/bin/XMenu/XMenuIII.dll
build/bin/XMenu/data/**
build/bin/XMenu/data/i18n/**
build/bin/XMenu/i18n/**
```

### Links

- GitHub: [YuiNijika/XMenu](https://github.com/YuiNijika/XMenu)
- GTAMODX: [XMenu page](https://gtamodx.com/mods/xmenu)
- Bilibili: [Tomoriゞ](https://space.bilibili.com/435502585)
- QQ Group: [GTAMODX QQ Group](https://gtamodx.com/qqun)

> XMenu is free. Reselling or commercial redistribution is not allowed.
