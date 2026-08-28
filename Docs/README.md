# Lyra 项目知识库

> UE5 Lyra 示例项目复刻的知识库。按框架子系统组织，帮助在开发时快速查阅类职责、继承关系和架构设计。

> **当前代码基线：** 当前工作区以提交 `19e6961`（2026-07-13）为基线，
> 并包含 2026-08-28 尚未提交的 LocalPlayer（本地玩家）、Settings（设置）、
> GameInstance（游戏实例）和 Experience（体验）改动。当前项目与 Lyra
> 参考项目均声明 UE 5.7，底层解释以 Unreal Engine 5.7.4 源码为准。
> 文档中的“已复刻”表示当前代码已有核心结构与主要行为证据，并与 Lyra
> 参考实现基本一致；它不自动代表已经完成编译、PIE、多人联网、打包或资产
> 验证。

---

## 快速导航

### 架构概览

| 文档 | 内容 | 适合场景 |
|------|------|---------|
| [01-Architecture-Overview.md](01-Architecture-Overview.md) | 项目结构、核心架构原则、框架关系全景图 | 新人入门、快速建立心智模型 |
| [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md) | 引擎启动 → PIE → Experience 加载 → 网络复制的完整调用链 | 调试、理解执行顺序 |
| [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) | 按来源汇总主要 TODO、注释路径、结构占位和验证任务 | 规划下一步开发方向 |
| [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md) | 所有引擎重写函数的调用时机、适合写的逻辑、注意事项 | 开发时选择正确生命周期函数 |
| [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) | 提交 `19e6961` 与 Lyra、UE 5.7.4 的 Controller / Camera / Replay / Cheat / External RPC 对照快照 | 回顾该次提交的调用链、差异与验证边界 |

### 运行时框架

| 文档 | 包含的类 | 框架职责 |
|------|---------|---------|
| [03-System-Framework.md](03-System-Framework.md) | `ULyraGameEngine`, `ULyraGameInstance`, `ULyraReplaySubsystem`, `ULyraAssetManager`, `ULyraGameData` | 引擎入口、用户登录设置桥接、实例管理、资产加载与回放能力判断 |
| [04-Game-Framework.md](04-Game-Framework.md) | `ALyraGameMode`, `ALyraGameState`, `ALyraGameSession`, `ALyraWorldSettings`, `ULyraUserFacingExperienceDefinition`, `ULyraAbilitySystemComponent`, `FLyraVerbMessage`, `ULyraVerbMessageHelpers` | 比赛管理层、前端 Playlist 入口、GameState GAS 与客户端消息桥接 |
| [05-Player-Framework.md](05-Player-Framework.md) | `ULyraLocalPlayer`, `ALyraPlayerController`, `ALyraReplayPlayerController`, `ILyraCameraAssistInterface`, `ALyraPlayerCameraManager`, `ALyraPlayerState`, `ALyraPlayerStart`, `ULyraPlayerSpawningManagerComponent` | LocalPlayer 队伍/设置桥接、Controller/Camera/Replay 协作、PlayerState ASC/Team/StatTag 与出生/重生管理 |
| [06-Character-Framework.md](06-Character-Framework.md) | `ALyraCharacter`, `ALyraCharacterWithAbilities`, `ULyraPawnExtensionComponent`, `ULyraPawnData` | 角色 Pawn 与 Pawn 生成配置 |
| [07-Experience-Framework.md](07-Experience-Framework.md) | `ULyraExperienceDefinition`, `ULyraUserFacingExperienceDefinition`, `ULyraExperienceManagerComponent`, `ULyraExperienceManager`, `UAsyncAction_ExperienceReady` | **核心架构 — Experience 加载状态机与 Ready 异步节点** |
| [08-UI-Framework.md](08-UI-Framework.md) | `ALyraHUD`, `ULyraGameViewportClient`, `ULyraUIManagerSubsystem`, `ULyraSettingsLocal`, `ULyraSettingsShared`, `ULyraLocalPlayer` | 显示层、UI Policy、本地/共享设置、登录加载与音频设备桥接 |

### 编辑器与开发工具

| 文档 | 包含的类 | 适用环境 |
|------|---------|---------|
| [11-Development-Tools.md](11-Development-Tools.md) | `ULyraDeveloperSettings`, `ULyraPlatformEmulationSettings`, `ULyraCheatManager`, `ULyraGameplayRpcRegistrationComponent` | PIE 调试、非发布构建作弊与 External RPC 结构占位 |
| [12-Editor-Module.md](12-Editor-Module.md) | `ULyraEditorEngine`, `FLyraEditorModule` | [Editor-Only] 编辑器引擎与 PIE 钩子 |

### 速查参考

| 文档 | 内容 |
|------|------|
| [02-Engine-Configuration.md](02-Engine-Configuration.md) | DefaultEngine.ini / DefaultGame.ini 中的引擎类替换、AssetManager 扫描、GameData/ActionSet、UI Policy 与加载屏幕配置 |
| [09-GameplayTags-System.md](09-GameplayTags-System.md) | 全部 ~42 个原生 GameplayTag 声明、分类与 GameplayTag Stack 容器 |
| [10-Logging-System.md](10-Logging-System.md) | 9 个全局日志通道 + 1 个文件内静态日志分类速查 |
| [13-Plugins-Catalog.md](13-Plugins-Catalog.md) | 12 个插件的角色、依赖、模块类型 |
| [14-Inheritance-Chains.md](14-Inheritance-Chains.md) | 全部类的完整继承链速查 |

---

## 快速查阅指南

### 我想知道某个引擎生命周期函数何时调用、适合写什么

→ 查看 [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md)（覆盖 30+ 个函数，按类分组，末尾有按开发场景的快速索引）

### 我想知道某个类属于哪个框架

→ 查看 [14-Inheritance-Chains.md](14-Inheritance-Chains.md) 按框架分类的继承链

### 我想知道 Experience 系统是怎么工作的

→ 查看 [07-Experience-Framework.md](07-Experience-Framework.md)（最核心的文档）

### 我想知道某个引擎默认类被替换成了什么

→ 查看 [02-Engine-Configuration.md](02-Engine-Configuration.md)

### 我想知道哪些功能还没做

→ 查看 [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md)

### 我想知道从引擎启动到游戏开始发生了什么

→ 查看 [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md)

### 我想知道登录后如何加载 Shared Settings（共享设置）

→ 查看 [08-UI-Framework.md](08-UI-Framework.md#登录成功后的共享设置加载)，其中明确标出
当前调用链进入的位置、停止位置和 UE 5.7.4 的预期异步终点。

### 我想核对提交 `19e6961` 的 PlayerController（玩家控制器）代码

→ 先查看 [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)，再按其中的源码入口继续阅读。

---

## 标记约定

| 标记 | 含义 |
|------|------|
| `[Runtime]` | 在所有构建目标中编译，Shipping 构建中可用 |
| `[Non-Shipping Runtime]` | 功能路径只在非发布构建启用；类型或 UFUNCTION 声明仍可能参与 Shipping 编译 |
| `[Editor-Only]` | 只在 `WITH_EDITOR` 或 Editor 模块中存在，运行时构建不包含对应代码路径 |
| `[Editor-Dev]` | 存在于 LyraGame/Development/，在所有配置中编译但主要服务于编辑器/PIE |
| `[结构占位]` | 类或函数存在，但只有空实现、简单 `Super`、被注释代码或 TODO |
| 🧩 | 继承 Modular 基类，支持 GameFeature 插件动态注入组件 |

实现状态统一使用：

| 状态 | 判断标准 |
|---|---|
| **已复刻** | 核心结构和主要行为与参考实现基本一致 |
| **部分复刻** | 主体存在，但缺少重要分支、配置、清理或关联功能 |
| **结构占位** | 只有类型、空实现、简单 `Super` 或 TODO |
| **未复刻** | 当前项目没有目标功能的核心实现 |
| **有意简化** | 当前项目明确采用更简单且符合目标的实现 |
| **项目自定义** | 当前差异是主动设计，而不是遗漏 |
| **待确认** | 需要资产、运行、网络、打包或 PIE 验证 |

---

## 文档阅读与证据顺序

1. 先看当前项目源码、Config（配置）、模块和可检查资产。
2. 再用同为 UE 5.7 系列的 Lyra 原项目说明完整形态与差异。
3. 用 UE 5.7.4 引擎源码确认父类行为、直接调用者和网络分支。
4. 静态源码仍不能确认的内容统一标记“待确认”或“推断”，并给出验证方式。

每篇核心框架文档优先包含当前复刻状态、类列表、核心数据流、初始化与清理、
网络与权限、资产与配置、Lyra 差异、TODO 来源和复习要点。

---

## 知识库统计

| 指标 | 数量 |
|------|------|
| 框架文档 | 6 个（System / Game / Player / Character / Experience / UI） |
| 编辑器/工具文档 | 2 个（Development Tools / Editor Module） |
| 速查参考文档 | 5 个（Engine Config / GameplayTags / Logging / Plugins / Inheritance Chains） |
| 综合文档 | 5 个（Architecture Overview / Data Flow / Stubs / Engine Lifecycle / Current Source Comparison） |
| UCLASS / UINTERFACE 类 | 40 个 |
| 非 UObject 类型 | 17 个（枚举、结构体、命名空间、委托、C++ 接口体） |
| 插件 | 12 个（11 Runtime + 1 Editor） |
| GameplayTag | ~42 个 |
| 日志通道 | 9 个全局 + 1 个文件内静态 |
