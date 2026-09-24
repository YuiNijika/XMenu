# 界面特性注册表（UI Registry）接手清单

> 给接手者的最短路径：先读「2. 架构」和「5. 接入新页面的 SOP」，再照「9. 剩余任务」挑一个页面开工。已经登记过的写法见「8. 已登记页面」，所有关键坑都在「6. 已踩过的坑」里，照做可以省掉一整轮试错。

---

## 0. 一句话现状

XMenu 有两套界面：**ImGui（C++）** 和 **WebUI（React + WebView2）**。因为老游戏用户仍有 Windows 7 / XP 跑不了 WebView2，两套必须长期并存。为避免功能漂移，正在把界面控件抽象成一份 **JSON 注册表**，两边读同一份。九个页面 **全部登记完毕**：玩家、行人、武器、载具、世界、场景、视觉、传送、设置，共 39 个分区、186 个控件
（142 个开关、9 个浮点、8 个整数、4 个下拉、23 个动作）。
载具那五个原本没挂载的分区也补齐了，读实时状态的控件走宿主登记的读写函数，不再要求挂在 MenuState 上。
**网页端到宿主只有三个入口**（`ui.get` / `ui.set` / `ui.run`），控件落到哪个状态、哪个动作全由注册表决定，
所以加控件只改 JSON，不用再往桥接表里补方法。每个分区都保留原生兜底。剩下的收尾工作见第 9 节。

---

## 1. 目标与背景

| 项 | 说明 |
|---|---|
| 为什么要两套 UI | XP / Win7 不支持 WebView2，ImGui 不能废弃 |
| 基准 | **以 ImGui 为基准登记**（ImGui 的控件是事实标准） |
| 抽象层级 | **特性层，不是布局层**。简单控件走注册表；复杂控件（车型生成列表、地图点选、颜色选择）保留原生实现作为逃生舱 |
| 门控 | 分区 / 控件两级，按 `capability` + `games` |

---

## 2. 架构

```
XMenu/src/data/ui-schema.json        ← 单一数据源（随构建部署到 Mods\XMenu\data\）
        │
        ├── ImGui 侧
        │     XMenu/src/ui/UiSchema.cpp  读表 + 渲染 + 解析（state/capability/action）
        │     XMenu/src/ui/pages/*.cpp   if (!DrawSection(...)) { 原生兜底 }
        │
        └── WebUI 侧
              XBase::WebBridge::RegisterMethod("ui.schema")  返回 { schema, capabilities }
              XMenu/react/src/lib/bridge.ts        fetchUiSchema()
              XMenu/react/src/components/menu/schema-section.tsx   SchemaSection 组件
              XMenu/react/src/pages/*.tsx          挂载 <SchemaSection .../>
```

**关键点**：`ui.schema` 返回的 `capabilities` 是按 `FeatureCapability` 名解析好的能力表。网页端因此**不需要**依赖桥接方法表，两边门控完全一致。

**控件怎么落到状态上**（网页端只有三个入口，全部按控件 id 分发）：

| 入口 | 用途 | 落到哪 |
|---|---|---|
| `ui.get { id }` | 取当前值 | `state` → MenuState 字段；无 `state` → 宿主登记的实时读写函数；`select` → 下拉来源 |
| `ui.set { id, value }` | 写值并生效 | 同上，写完再按 `onChange` 触发一次生效回调 |
| `ui.run { id }` | 触发动作 | `RunAction` 的分支 |
| `ui.options { id }` | 下拉的选项与当前值 | 宿主登记的下拉来源 |

五类控件对宿主的依赖：

| kind | 渲染 | 宿主侧要准备什么 |
|---|---|---|
| `toggle` / `float` / `int` | 开关、滑块 | 有 `state` 就只用进解析表；没有 `state` 才要登记实时读写函数 |
| `action` | 按钮 | `RunAction` 里加一条分支 |
| `select` | 下拉（ImGui 走 Combo） | `RegisterSelectSource` 登记选项、读、写三项 |

`ui.get` / `ui.set` / `ui.run` 的实现在 `ReactUi.cpp`（三个薄壳），分发在 `UiSchema.cpp` 的
`ControlValue` / `SetControlValue` / `RunControl`。**所以加一个开关类控件通常一行 C++ 都不用写**，
前提是它的状态已经在 MenuState 里且有 `state` 名可对。

---

## 3. 文件清单

| 文件 | 作用 |
|---|---|
| `XMenu/src/data/ui-schema.json` | 注册表本体，已登记 player / vehicle / world 三个页签 |
| `XMenu/src/ui/UiSchema.h` / `.cpp` | 加载、绘制、三类解析器 |
| `XMenu/src/ui/pages/Vehicle.cpp` | 已接入（6 个分区） |
| `XMenu/src/ui/pages/World.cpp` | 已接入（gameRules） |
| `XMenu/src/ui/pages/Player.cpp` | 已接入（actions / statusToggles / flight） |
| `XMenu/src/ui/pages/Ped.cpp` | 已接入（strategies / noFire / spawnLimits） |
| `XMenu/src/ui/pages/Weapon.cpp` | 已接入（runtime / statOverrides / bulletAssist / giveOptions） |
| `XMenu/src/ui/pages/Visual.cpp` | 已接入（display / radarOptions / filter） |
| `XMenu/src/ui/pages/Teleport.cpp` | 已接入（quickOptions / forward） |
| `XMenu/src/ui/pages/Scene.cpp` | 已接入（animation / styles） |
| `XMenu/src/ui/Menu.cpp` | 已接入设置页（runtime / overlay / guiStyle / web） |
| `XMenu/src/controllers/ReactUi.cpp` | 桥接方法注册处（含 XMenu 自有方法） |
| `XMenu/react/src/lib/bridge.ts` | 类型定义 + `fetchUiSchema()` |
| `XMenu/react/src/components/menu/schema-section.tsx` | 网页端通用渲染组件 |
| `XMenu/react/src/pages/*.tsx` | vehicle / world / player 已挂载 |
| `XMenu/Build.bat` | 已加一步把 `ui-schema.json` 拷到产物（**原先只拷子目录**） |
| `XMenu/tools/check_ui_registry.py` | 开发期检查工具，见第 6.5 节 |

---

## 4. schema 字段规范

```json
{
  "version": 1,
  "tabs": [{
    "id": "player", "labelKey": "tab.player",
    "pages": [{
      "id": "playerMain", "labelKey": "tab.player",
      "sections": [{
        "id": "statusToggles", "labelKey": "player.statusToggles",
        "columns": 3,                    // 可选，多列排版
        "inline": false,                 // 可选，为真的分区排成一排，适合动作按钮
        "capability": "PlayerNeverWanted", // 可选，分区级门控
        "games": ["sa"],                 // 可选，版本白名单，省略=全部
        "hintKey": "player.autoFlightOptions",  // 可选，分区标题下的一段说明文字
        "controls": [{
          "id": "player.neverWanted",    // 必须等于桥接方法名，网页端按它取初值与下发
          "kind": "toggle",              // toggle | float | int | action
          "labelKey": "player.neverWanted",
          "state": "NeverWanted",        // MenuState 字段名
          "capability": "PlayerNeverWanted", // 可选，控件级门控
          "games": ["sa"],
          "min": 0, "max": 255, "step": 1,        // float / int 用
          "format": "%.1f",              // 可选，float 的显示格式，默认 %.0f
          "visibleWhen": "vehicle.neon",  // 条件显隐，引用另一个控件 id
          "onChange": "World.FreezeTime"  // 变更回调名
        }]
      }],
      "custom": [{ "id": "vehicle.spawnList", "renderer": "native" }]
    }]
  }]
}
```

**两个排版字段**：`inline` 为真的分区按行排列并自动换行，适合一排动作按钮，按钮宽度按 `columns` 折算；`hintKey` 在分区标题下渲染一段说明文字，ImGui 与网页端都会显示。

**`requiresVehicle`**：为真的分区只在玩家坐在载具里时绘制。ImGui 侧由 `DrawSection` 自己判断并算作已接管，
网页侧由页面把「当前有没有载具」通过 `visible` 属性传给 `SchemaSection`。

**没有 `state` 的控件 = 读实时状态的控件**：宿主在注册桥接方法时顺带调一次 `UiSchema::RegisterLiveToggle` /
`RegisterLiveNumber` 登记一对读写函数，控件 id 与桥接方法名保持同名。
ImGui 每帧读一次真实状态、改了就写回去；网页端走同名方法，两边看到的是同一份状态。
灯光、车门、载具防护、血量与七项载具属性就是这么做的；纯开关类的 `state` 写法仍然更省事，能用就用。

**门控顺序**：分区 `games` → 分区 `capability` → 控件 `games` → `visibleWhen` → 控件 `capability`。

**控件 id 与桥接方法同名**：网页端读初值发的是不带参数的同名调用，下发发的是带 `enable` 或 `value` 的同名调用，所以 id 不能随便起别的名字。

---

## 5. 接入新页面的 SOP

1. **盘点**：`grep` 目标页面的 `UI::Checkbox` / `SliderFloat` / `SliderInt` / `Button` / `HasCapability` / `#ifdef`，列出控件清单。
2. **区分来源**：只登记 **`MenuState` 支撑的控件** 和 **无需参数的动作**；读实时世界/载具状态的控件（小时、重力、天气 ID、生命值…）**先不纳入**，保持原生。
3. **写 schema**：新增 tab / page / section，填 `id`、`kind`、`labelKey`、`state`、`capability`、`games`、`onChange`。`games` 要严格对齐原生 `#ifdef`。`id` 用已有的桥接方法名，缺的话第 5 步补。
4. **加解析器**（`UiSchema.cpp`）：
   - `BoolState` / `FloatState` / `IntState` 三张表加 `state` 名 → 字段指针
   - `CapabilityUsable` 表加 `capability` 名 → `FeatureCapability`
   - `RunChange` / `RunAction` 加分支
   - `SchemaPayload` 的 `names` 数组加新 capability 名（否则网页端查不到）
   - 新标题键记得补进四种语言的 `src/data/i18n/*/player.json` 之类词条文件
5. **补宿主侧（多数情况不用补）**：
   - 开关与数值：`state` 名进了三张解析表就完事，网页端走 `ui.get` / `ui.set`，不用写桥接方法。
   - 动作：在 `RunAction` 里加一条分支，网页端走 `ui.run`。
   - 读实时状态：在 `ReactUi.cpp` 里 `registerLiveToggle` / `RegisterLiveNumber` 登记一对读写函数（见第 4 节）。
   - 只有页面级领域接口（数据列表、快照、外观设置这类）才需要单独 `RegisterMethod`。
6. **接线**：
   - ImGui：`if (!UiSchema::DrawSection(tab, page, section)) { 原样原生代码 }`；分区一旦登记就视为接管，没进注册表的本地控件要单独画
   - React：页面里加 `fetchUiSchema` 的 state/effect，再挂 `<SchemaSection .../>`
7. **去重复**：同一个控件在网页端既有手写实现又有注册表实现的，删掉手写那份，只留没进注册表的，避免两边各自为政。
8. **跑一遍检查工具**：`python tools/check_ui_registry.py`，退出码非零就是有问题。
   它核五类引用加两项一致性，见第 6.5 节。
9. **构建验证**：`npm run build` + `XMenu/Build.bat Release`（见第 7 节）。

---

## 6. 已踩过的坑（务必先看）

| # | 现象 | 原因 | 解法 |
|---|---|---|---|
| 1 | 编译报找不到 `ePedStates.h` | III 没有这个头，`PEDSTATE_DEAD` 直接在 `CPed.h` 里 | 去掉该 include，只依赖 `CPed.h` |
| 2 | `XBase::Json::Value` 没有 `size()` | 数组存在 variant 里 | `std::get<std::vector<Value>>(v.data).size()` |
| 3 | 两套 `Checkbox` 签名不同 | `XBase::UI::Checkbox` 用**引用**，XMenu 的 `UI::`（Widget.h）用**指针** | 渲染器统一用 XMenu 的 `UI::`（指针） |
| 4 | 运行时 DLL 报 `LNK2005` | `PortableStubs` 的空桩和新实现重复定义 | 接入一个域就删掉对应的空桩 |
| 5 | 网页端开关不动 / 发了没反应 | `ToggleGrid` 发 `{ enable }`，而服务端读 `value` | 桥接同时接受 `enable` 与 `value` |
| 6 | `tsc -b` 一直红 | `ToggleItem.method` 在 `ParamToggles` 场景不该必填 | 已改为可选；用 `item.method ?? item.label` 兜底 |
| 7 | 网页端改了 ImGui 不跟着变 | XBase 的 `world.*` 桥接**不写 MenuState** | XMenu 侧覆盖注册，写 MenuState 再下发 |
| 8 | 改 CRLF 文件匹配不到 | `Capabilities.cpp` 是 CRLF | python 处理前先 `.replace("\r\n","\n")`，写完按原行尾还原 |
| 9 | 网页端所有注册表开关初始都是关的 | `SchemaSection` 读的是应答里的 `value`，XMenu 的开关桥接当时只回 `enabled` | 桥接同时给 `enabled` 与 `value` |
| 10 | 网页端一进页面开关全变开 | 不带参数的同名调用是在读初值，而旧桥接写的是 `params["enable"].AsBool(true)`，缺参数时被默认成真 | 只在参数存在时才赋值，默认值改用当前状态 |
| 11 | ImGui 又画出了注册表刻意隐藏的控件 | 曾把 DrawSection 的返回值定义成「是否画了控件」，控件被能力筛掉时落到原生兜底 | 返回值只表示「这段分区是否已由注册表接管」，被筛掉也算接管 |
| 12 | 一排动作按钮被画成竖着一列 | 注册表渲染器默认按分区分栏排控件 | 分区加 `inline` 加 `columns`，ImGui 走 SameLine、网页端走 flex 换行 |
| 13 | 浮点滑块小数位显示不出来 | float 格式写死成 `%.0f` | 控件加 `format` 字段，缺省仍用 `%.0f` |
| 14 | 网页端条件显隐的控件一直显示 | `SchemaSection` 只按 games 与 capability 过滤，`visibleWhen` 没实现 | 分区统一拉一次初值存成表，按被依赖项的值过滤，取不到值时先显示避免闪烁 |
| 15 | 网页端开关点击就弹「失败」 | 控件 id 起成了界面语义的名字，而桥接方法叫别的名字（如 `vehicle.lockSpeed` 对 `vehicle.speedLock`） | 写完 schema 跑一遍 id 与桥接方法名的比对，缺的在 `ReactUi.cpp` 注册同名方法 |
| 16 | 编译报找不到 `ApplySpeedLock` | 在 `ReactUi.cpp` 里用了载具控制器却没包含它的头 | 用了哪个控制器就包含哪个头，别指望间接包含 |
| 17 | 网页端改了子弹辅助却没效果 | 网页端直接调 XBase 的配置接口，而 `Controllers::BulletAssist::Process` 每帧拿 MenuState 整组覆盖 | 桥接改成写 MenuState，让每帧同步去下发 |
| 18 | 网页端改了属性改写下一帧才生效 | 属性改写改完需要重刷武器数据，桥接只写了状态 | 桥接里带上 `ResetStats`，与 ImGui 的 `weaponStatsChanged` 同一条路 |
| 19 | 网页端改了雷达选项没效果 | 雷达选项由 `Pages::Visual::Process` 每帧整组下发，网页端直接调 XBase 的分组接口会被覆盖 | 八个选项各注册一个方法写 MenuState，让每帧同步去下发 |
| 20 | 想复用 `visual.filter` 当开关 | 这个名字在 XBase 里是「按 id 与强度应用滤镜」的接口，网页端的应用按钮正用它 | 语义冲突时另起名字，这里叫 `visual.filterEnable` |
| 21 | 往 `RunChange` 里插入新分支时把上一个分支顶掉了 | 编辑时拿末尾分支当锚点整段替换，忘了把锚点本身写回去 | 改完跑一遍比对，确认 schema 里用到的 `onChange` 在实现里都还在 |
| 22 | 快速地图传送之类的开关改了没落盘 | 这类选项只在热键那边按需读取，改完要靠显式保存 | 注册表用 `AppConfig.Save` 回调，桥接同样在改完时保存 |
| 23 | 动作 id 与 `RunAction` 分支的名字对不上 | 注册表的动作 id 用的是桥接方法名，写分支时顺手写了词条名（如 `world.syncClock` 对 `world.syncRealTime`） | 动作 id 一律抄桥接方法名，写完跑一遍动作名比对 |
| 24 | 网页端有个开关组既走参数又走状态，两边打架 | 网页端把开关当参数随动作一起发，宿主却读 MenuState，参数赢了但状态没同步 | 把宿主侧方法改成统一读 MenuState，参数只保留真正需要即时传的那几个 |
| 25 | 分区挂在域级能力上却查不到 | `WebView` 这类是 `Capability` 而不是 `FeatureCapability`，能力表里没有它 | `CapabilityUsable` 里补一张域级能力表，两类名字都能解析 |
| 26 | 读实时状态的控件在 ImGui 里根本不画 | 渲染器只认 `state`，没有 `state` 的开关直接跳过，而分区又算作已接管，于是控件整个消失 | 给这类控件加一套宿主登记的读写函数，渲染器在没有 `state` 时改走它 |
| 27 | 网页端把开关当参数随动作发出去，宿主却读状态 | 生成选项这类开关被当作 `vehicle.spawn` 的参数传，宿主侧读的是 MenuState | 覆盖宿主侧方法统一读 MenuState，参数只保留真正需要即时传的 |
| 28 | 网页端在没坐进载具时也画出载具分区 | 分区带了 `requiresVehicle` 但两侧都没实现 | ImGui 在 `DrawSection` 里判断并算作已接管，网页端由页面传 `visible` |
| 29 | 改了控件 id，ImGui 侧控件不画了 | 只改了 schema 与桥接方法名，忘了同步实时读写函数的登记名，两边对不上就找不到读写函数 | 比对要加上一类：没有 `state` 的控件必须有实时读写函数，且登记名与控件 id 一致 |
| 30 | 存档这类动作从界面线程直触发 | 收敛成 `ui.run` 之后顺手改成直调控制器，而存档原本特意挂起到游戏线程 | 存档走 `Controllers::ReactUi::QueueSaveGame()`，不进 `RunAction` 的直调分支 |
| 31 | 桥接表面随控件数量线性膨胀 | 每个控件一个方法，一轮下来加到 157 个 | 控件统一走 `ui.get` / `ui.set` / `ui.run`，控件方法从 114 个降到 0，桥接只剩领域接口 |
| 32 | 网页端换主题后重启又回到旧主题 | 网页的 `settings.setTheme` 只调了 `SetThemeByIndex`，它只改内存索引，不落配置 | 下拉来源的写函数按 ImGui 的步骤走：先 `AppConfig::SetGuiThemeIndex` 再换皮再保存 |

---

## 7. 构建与验证

```bash
# React（tsc 类型检查 + vite 打包）
cd XMenu/react
export PATH="/c/Users/Administrator/.workbuddy/binaries/node/versions/22.22.2-3:$PATH"
npm run build

# XMenu（三个 asi + 网页产物 + 注册表）
cd XMenu && Build.bat Release --no-pause
```

**已知干扰项**（不是错误）：
- XBase 构建末尾 `[Error] Build failed` + `[Warning] XBase viewer build failed` —— 是可选的 viewer 子工程缺 `windows.h`，主产物 OK，看到 `Build completed successfully` 即可。
- 沙箱报 `reg.exe` 被拦截 —— 环境策略，不影响构建。

**注意**：改了注册表要重新构建，产物里的那份副本与源码不一致时检查工具会直接报错。

---

## 6.5 检查工具

```bash
cd XMenu && python tools/check_ui_registry.py
```

退出码为 0 表示通过，非零表示有错误。它拉平了这几条：

| 类别 | 核什么 |
|---|---|
| state | 有 `state` 的控件，名字必须在解析表里 |
| 实时控件 | 没有 `state` 的控件，必须在 `src/controllers/*.cpp` 里查到同名实时读写函数 |
| capability | 分区级与控件级能力都要在映射表里，且进了下发给网页的名单 |
| onChange | 名字必须在 `RunChange` 里有分支 |
| action | 动作 id 必须在 `RunAction` 里有分支 |
| select | 下拉的 `source` 必须真的登记过 |
| 显隐 | `visibleWhen` 指向的控件要在同一个分区里 |
| 一致性 | 源码与产物里的注册表要一致 |
| 词条 | 分区标题与控件标签在四种语言里是否存在（只提示，不判失败） |

这几类恰好就是第 6 节坑表里那些编译期看不出来、运行时才炸的问题。工具本身做过反向验证：故意注入
不存在的 state、没登记的动作、没实现的下拉来源、没备份的实时控件，四类全部被抓出来并且退出码为 1。

---

## 8. 已登记页面

| 页面 | 登记分区 | 备注 |
|---|---|---|
| Vehicle | actions / runtime / status / proof / health / special / cheat / effect / autoDrive / traffic / speed / spawnOptions | 后四个分区读实时状态，见字段规范里的实时控件一节 |
| World | timeLock / gameRules | 小时、重力、天气等读实时状态的控件未纳入 |
| Player | actions / statusToggles / flight | 见下方未纳入清单 |
| Ped | strategies / noFire / spawnOptions / gangWars / spawnLimits | 见下方未纳入清单 |
| Weapon | runtime / statOverrides / bulletAssist / giveOptions | 见下方未纳入清单 |
| Visual | display / radarOptions / filter | 见下方未纳入清单 |
| Teleport | quickOptions / forward | 见下方未纳入清单 |
| Scene | animation / styles | 见下方未纳入清单 |
| Settings | runtime / overlay / guiStyle / web | 见下方未纳入清单 |

**Player 页未纳入的控件**（保持原生，理由都写在代码注释里）：
- `player_values` 页的血量 / 护甲 / 现金 / 通缉：读的是实时玩家状态。
- 单项防护五个开关：读的是实时 proof 状态。
- 住院免费、被捕免费：运行时读写，没有对应的 MenuState 字段。
- 瞄准换肤：`MenuState::AimSkinChanger` 只被配置和界面读写，**没有任何逻辑消费它**，ImGui 的开关本身也是空转；要登记的话先把它接进 `Controllers::Player::Process`。
- 外观 / 属性页与自定义皮肤列表：整页是输入框加列表，属于 custom 逃生舱。

**Ped 页未纳入的控件**（保持原生）：
- 生成页的模型 / 类型 / 帮派 / 武器 / 血量 / 护甲输入框，以及生成、删除三个按钮：整组是输入框加动作，属于 custom 逃生舱。
- 帮派页的帮派 ID / 密度 / 成员槽 / 成员模型 / 武器：读的是实时帮派数据。

**两条 Ped 页特有的注意**：
- 全局策略开关在 ImGui 里大部分没有能力门控（`PedGlobalStrategies` 在 VC/III 是未支持，但 ImGui 照样画）。注册表照旧没加门控，避免把 ImGui 看得见的控件藏掉；要收紧得先决定要不要改 ImGui。
- 不开枪那组在 III 上会因为 `BulletAssistFireSuppression` 未支持而整组隐藏，ImGui 旧行为是置灰显示，这是注册表与原生兜底的已知差异。

---

## 9. 收尾工作（九个页面、载具五个分区的补齐都已完成）

**已经做完的**：载具的 actions / status / proof / health / special 五个分区两侧都挂上了，
Ped 的生成选项与帮派战争也登记完毕。这轮把「读实时状态的控件」这个缺口补上了，
以后再有类似的控件，照 `RegisterLiveToggle` 的写法登记即可。

**已经全部做完**：设置页的语言、回退语言、主题、交互模式四个下拉也登记进去了（`select` 控件类型）。
交互模式在列表界面下不显示，靠 `visibleWhen` 支持前置叹号（`!settings.useListMenu`）表达取反。

**明确留在原生、且不打算动的东西**：
- 显示模式（全屏 / 窗口 / 无边框）：改完要提示重启并显示「当前 / 待生效」两行状态，不是单纯设值，留在两侧原生。
- 各页的文本输入、按钮加输入的组合、按分类的按钮列表：注册表的设计逃生舱，不是欠债。

**维护建议**：以后每次改完注册表，跑一遍第 5 节 SOP 第 8 步的比对。收敛之后要核的是四类：
控件的 `state` 在解析表里、`capability` 在映射与下发名单里、`onChange` 在 `RunChange` 分支里、
动作在 `RunAction` 分支里；没有 `state` 的控件要在 `ReactUi.cpp` 里查到同名实时读写函数。
一次性脚本的做法在改动频繁时值得固化成开发期工具。

**Weapon 页未纳入的控件**（保持原生）：
- 弹药数量、武器类型或模型 ID 两个输入框与按 ID 获取按钮：输入框加动作，属于 custom 逃生舱。
- 滚轮刷出开关与输入 ID 刷出：与后面的刷出输入是一组，留在原生代码里没拆。
- 优先锁定部位：四个单选，注册表没有单选控件类型，网页端用 `weapon.aimPart` 单独下发。
- 武器列表：按分类的生成按钮，属于 custom 逃生舱。

**Weapon 页两条重要的运行时约定**：
- 属性改写类的开关改完必须 `ResetStats`，网页端桥接里也是这么做的，少了这一步改动下一帧前不会生效。
- 子弹辅助与优先锁定部位由 `Controllers::BulletAssist::Process` 每帧整组下发，
  网页端此前直接调 XBase 的配置接口，写完就会被下一帧覆盖，等于没生效；现在改成写 MenuState。

**Visual 页未纳入的控件**（保持原生）：
- 滤镜 ID 输入框与 Timecyc 强度滑块：强度在 ImGui 里本来就是禁用的占位项，没有实装；滤镜 ID 是自由输入，注册表的数值控件是滑块，套不上。
- 应用滤镜按钮：紧跟着滤镜 ID 输入才有意义，留在原生代码里。
- 滤镜列表：按分类的按钮网格，属于 custom 逃生舱。

**Visual 页两条约定**：
- HUD 与雷达只在点击那一刻下发一次，桥接写完 MenuState 还要立刻调一次；雷达选项则是每帧整组下发，写状态就够了。
- 滤镜开关的桥接叫 `visual.filterEnable` 而不是 `visual.filter`，因为后者在 XBase 里是「按 id 与强度应用滤镜」的接口，语义不一样，改名免得把网页端现有的应用按钮搞坏。

**Teleport 页未纳入的控件**（保持原生）：
- 坐标输入框与读取当前坐标按钮：文本输入加动作，属于 custom 逃生舱。
- 传送到坐标、按地图坐标、传送到标记、回到城市中心、向前传送五个按钮：都要先解析坐标输入框，不是无参动作。
- 自定义地图尺寸的宽高输入与应用、恢复默认：折叠区里带草稿状态，属于 custom 逃生舱。
- 地点页的名字输入、添加按钮与地点列表。

**Teleport 页一条约定**：快捷传送与向前传送的选项只在热键和快速地图那边按需读取，没有每帧推送，
改完除了写 MenuState 还要立刻落盘，注册表用 `AppConfig.Save` 回调，桥接里也是同样的处理。

**Scene 页未纳入的控件**（保持原生）：
- 动画分组名、动画名、粒子名、过场名、室内 ID、任务序号这些文本输入：注册表没有文本控件类型。
- 播放、停止、生成粒子、移除粒子、开始与停止过场、开始与失败任务这些按钮：都要配合上面的文本输入，不是无参动作。
- 动画、粒子、过场、任务四个列表：按分类的按钮网格，属于 custom 逃生舱。

**Scene 页一条约定**：循环、副任务动作、目标行人三个开关与格斗、走路两个风格值都只在播放或应用那一刻读取。
网页端的 `scene.animation` 已改成把分组与动画名写进 MenuState 再走控制器播放，那三个开关统一读 MenuState，
参数里再带这三个键会被忽略 —— 与 ImGui 的播放按钮同一条路。

**Settings 页未纳入的控件**（保持原生）：
- 界面语言、回退语言、显示模式、主题、交互模式：都是下拉选择，注册表目前只有开关、数值与动作三类控件。
  网页端这几个走的是 `settings.appearance` / `settings.setTheme` / `settings.setLanguage` / `settings.windowMode` 那套自有桥接。
- 菜单热键输入框、按 ID 刷出的输入、配置文件导入导出文本框。
- 状态持久化、动作热键、更新、日志、调试几块：按钮与列表，属于 custom 逃生舱。

**Settings 页一条约定**：换成列表界面或改交互模式要立刻换皮并复位列表选中项，
注册表用 `Menu.SyncGuiTheme` 回调做这三件事，`Menu::NotifySurfaceChanged` 是为此新开的出口。
网页视图缩放挂在**域级能力** `WebView` 上，`CapabilityUsable` 因此多了一张域级能力表。

**顺手可以做的小活**：Ped 生成页的五个选项开关、帮派页的帮派战争开关还没登记；注册表里 `vehicle` 的 actions / status / proof / health / special 五个分区目前没有任何一侧挂载，它们的 id 还没对齐桥接方法名，挂载之前必须先补上。

---

## 10. 本次会话相关的其他改动（接手前建议了解）

| 改动 | 状态 |
|---|---|
| 目录定稿：`XBase\Library\`（公用二进制）与 `XBase\Mods\<mod>\`（模组数据） | 已落地并部署 |
| 单文件 asi（XMenu 只产 asi，无 payload dll） | 已实现并验证 |
| `XBase::Overlay` 重写为 ImGui 画布，三版本共用一份 | 已实现（编译验证） |
| 能力矩阵核实：VC/III 的 Cheats 实为 Partial（文档曾误写未支持） | 文档已修正 |
| Scene 粗粒度与细粒度 `SceneMission` 对齐 | 已改 |
| III 的 BulletAssist 边界框（只读显示） | 已实现（编译验证） |
| 载具速度隐形锁定 bug 修复 | 已修复 |
| VC/III 的 Camera | **已回退**（实机无效，不留假声明） |
| 存量注释按 yuinijika 规范清理（69 处） | 已完成，剩余 0 |

---

## 11. 参考资料

- 项目记忆：`F:/GTA/DEV/.workbuddy/memory/2026-09-24.md`（本轮全部细节）
- 长期记忆：`F:/GTA/DEV/.workbuddy/memory/MEMORY.md`（XBase 架构约定）
- 线上文档：https://blog.miomoe.cn/docs （`xbase/index`、`xbase/data-layout`、`xbase/shared-runtime`）
- 规范：`.agents/skills/yuinijika/coding-style.md`（代码）、`docs-style.md`（文档）
  - **注释硬性要求**：纯散文，禁止括号 / 冒号 / 分号 / 箭头 / 破折号 / 等号 / 引号 / 反引号，禁止裸标识符

---

## 12. 接手时的第一件事

跑一次双构建确认基线是绿的（第 7 节）。然后挑一个页面按「5. SOP」开工，每步都对照「6. 已踩过的坑」。

**不要**一次登记多个页面 —— 改动的是用户实际在用的界面，且无法在本地实机验证（没有 III/VC 运行环境），必须小步、可回退（每个分区都保留原生兜底）。
