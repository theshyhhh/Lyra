# 01 - 架构总览

> Lyra 项目的顶层架构设计。在阅读具体框架文档前，先理解这张全景图。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 LocalPlayer（本地玩家）、Settings（设置）、GameInstance（游戏
> 实例）和 Experience（体验）改动。当前项目与 Lyra 参考项目均声明 UE 5.7，
> 底层机制按 Unreal Engine 5.7.4 源码核对。架构图以当前项目为主体，不把
> Lyra 原项目中的插件、类或资产默认算作当前已实现。

---

## 项目结构

```
Lyra/
  Config/
    DefaultEngine.ini              ← 所有引擎类替换 + 项目配置
    DefaultGame.ini                ← AssetManager 扫描规则 + 游戏数据路径 + UI/LoadingScreen 默认配置
  Source/
    LyraEditor/                    ← 纯编辑器模块（仅编辑器构建）
      LyraEditorEngine.h/.cpp      ← 自定义编辑器引擎
      LyraEditor.h/.cpp            ← 编辑器模块（PIE 委托）
    LyraGame/                      ← 运行时游戏模块（+ 编辑器开发工具）
      LyraGameplayTags.h/.cpp      ← 原生 GameplayTag 注册
      LyraLogChannels.h            ← 日志通道 + GetClientServerContextString
      AbilitySystem/               ← GAS 组件与游戏阶段相关代码
      Camera/                      ← 相机辅助接口与 PlayerCameraManager 结构占位
      Character/                   ← 角色框架
      Development/                 ← 编辑器开发工具
      GameModes/                   ← Game 框架 + Experience 框架 + Experience Ready 异步节点
      Messages/                    ← Gameplay Message 通用 payload 与转换工具
      Player/                      ← PlayerController、PlayerState、作弊管理器与玩家状态
      Replays/                     ← 回放能力判断与平台 Trait
      Settings/                    ← 机器级 Local Settings + 每用户 Shared Settings
      System/                      ← System 框架 + GameplayTag Stack 快速数组复制
      Teams/                       ← Team Agent 接口与队伍变更委托
      Tests/                       ← External RPC 自动化测试注册结构占位
      UI/                          ← UI 框架 + UI 管理子系统
  Plugins/                         ← 12 个插件（11 运行时 + 1 编辑器）
  Docs/                            ← 知识库文档
```

---

## 当前复刻状态

| 架构层 | 当前状态 | 当前项目实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| System（系统） | **部分复刻** | 引擎、GameInstance、AssetManager 和 GameData 类型存在 | 完整 GameplayCue、设置和 Replay 子系统 | 全局宿主已建立，部分跨局能力缺失 |
| Game / Experience（游戏 / 体验） | **部分复刻** | Experience 选择、加载状态机、GameState 宿主和 Pawn 生成存在 | 完整错误处理、在线与 Pawn 初始化 | 基础游戏流程可读，边缘分支需验证 |
| Player / GAS（玩家 / 能力系统） | **部分复刻** | PlayerState ASC、Controller 协作、Team 和出生管理存在 | Pawn Extension、AbilitySet、输入和完整 Character 接入 | 当前最重要的主链缺口 |
| Camera（相机） | **部分复刻** | Camera Assist 消费者存在；Manager 是结构占位 | 第三人称 Camera Mode、UI Camera 和调试 | 穿透生产端与 Lyra 相机扩展缺失 |
| Replay（回放） | **部分复刻** | 平台能力判断和播放跟随存在 | 客户端录制、列表、删除、Seek | 播放与录制完成度不同 |
| UI / Settings（界面 / 设置） | **部分复刻** | UI Policy、本地设置入口、共享设置同步加载外壳和 LocalPlayer 桥接存在 | 完整设置字段、应用/保存、异步加载和设备配置重应用 | 登录链已接入，但异步端点和实际设置行为仍是占位 |
| Development Tools（开发工具） | **部分复刻** | PIE 设置可用；Cheat / External RPC 类型存在 | 完整 Cheat 命令和 HTTP 路由 | 后两者仍是结构占位 |

> 🧪 **待验证：**
> 蓝图 GameMode、Experience、GameFeature 和 Camera 资产可能覆盖或补充
> C++ 默认值；未打开资产前只确认代码侧支持，不能确认最终运行配置。

---

## 核心架构原则

### 1. 引擎类替换

Lyra 通过 `DefaultEngine.ini` 替换了几乎所有核心引擎类：

```
引擎默认                          → Lyra 自定义
──────────────────────────────────────────────────
UGameEngine                      → ULyraGameEngine
UUnrealEdEngine                  → ULyraEditorEngine
UAssetManager                    → ULyraAssetManager
AWorldSettings                   → ALyraWorldSettings
ULocalPlayer                     → ULyraLocalPlayer
UGameViewportClient              → ULyraGameViewportClient
UGameUserSettings                → ULyraSettingsLocal
```

这是理解 Lyra 的起点 — 几乎所有引擎默认行为都有一个 Lyra 自定义钩子。

### 2. ModularGameplay 模式

核心框架类全部继承 Modular 变体：

| 引擎基类 | Modular 基类 | Lyra 类 |
|---------|-------------|--------|
| `AGameModeBase` | `AModularGameModeBase` | `ALyraGameMode` |
| `AGameStateBase` | `AModularGameStateBase` | `ALyraGameState` |
| `APlayerState` | `AModularPlayerState` | `ALyraPlayerState` |
| `ACharacter` | `AModularCharacter` | `ALyraCharacter` |

Modular 基类自动注册到 `UGameFrameworkComponentManager`，使得 GameFeature Action 可以在 Experience 加载时**动态注入组件**。这是 Lyra 实现数据驱动玩法组合的基础。

### 3. Experience 系统（核心创新）

传统方案中 GameMode 承载所有玩法逻辑。Lyra 的方案：

```
Experience 数据资产 (ULyraExperienceDefinition)
  ├── 声明需要哪些 GameFeature 插件
  ├── 配置 GameFeatureAction 列表（安装/卸载玩法功能）
  ├── 指定默认 PawnData
  └── 引用可复用的 ActionSet

Experience 状态机 (ULyraExperienceManagerComponent)
  └── 运行在 GameState 上，被复制到客户端
        └── 异步加载资产 → 激活插件 → 执行 Action → 广播完成
```

`ALyraGameMode` 在地图启动后按 URL、PIE 开发设置、命令行、WorldSettings、Dedicated Server 和默认值的优先级决定本局 Experience，再交给 `ULyraExperienceManagerComponent` 加载。`ALyraGameState` 现在直接创建这个组件，并同时挂载 `ULyraAbilitySystemComponent`，作为比赛级 GAS/消息广播的宿主。`ALyraPlayerState` 也开始持有玩家级 ASC，并复制 PawnData、连接类型、Team/Squad、StatTag Stack 和观战视角状态。

### 4. 数据驱动配置

所有全局配置使用 `UPrimaryDataAsset` 子类：

| 数据资产 | 内容 |
|---------|------|
| `ULyraGameData` | 全局 GE 引用（伤害/治疗/动态 Tag） |
| `ULyraPawnData` | 生成哪个 Pawn 类 |
| `ULyraExperienceDefinition` | 完整的玩法体验定义 |
| `ULyraExperienceActionSet` | 可复用的 Action + 插件打包 |
| `ULyraUserFacingExperienceDefinition` | 前端/Playlist 入口：地图、Experience、Session 参数、展示信息 |

运行时 Pawn 生成由 `ALyraGameMode` 根据 PawnData 决定 PawnClass。PawnData 优先来自 `ALyraPlayerState` 已复制的玩家级配置，其次回退到当前 Experience 的 `DefaultPawnData`，最后回退到 `ULyraAssetManager` 的默认 PawnData；`ULyraPawnExtensionComponent` 继续作为 Pawn 初始化/扩展链入口。

### 5. Gameplay Message 解耦

新增的 `FLyraVerbMessage` 使用 `Verb` GameplayTag 表达事件通道，用 `Instigator`、`Target`、三组 Tag 容器和 `Magnitude` 描述一次通用 gameplay 事件。`ALyraGameState` 提供可靠/非可靠 NetMulticast，把服务器事件桥接到客户端的 `UGameplayMessageSubsystem`；`ULyraVerbMessageHelpers` 则负责 PlayerState/PlayerController 解析，以及与 `FGameplayCueParameters` 互转。

### 6. UI 与加载屏幕配置

`DefaultGame.ini` 现在配置 `ULyraUIManagerSubsystem` 的默认 `B_LyraUIPolicy`，并为 CommonLoadingScreen 指定 `W_LoadingScreen_Host`。运行时 `ULyraUIManagerSubsystem` 每帧把 `PrimaryGameLayout` 的可见性同步到 `AHUD::bShowHUD`，而 `UAsyncAction_ExperienceReady` 给蓝图提供等待 Experience 加载完成的统一入口。

### 7. 机器级设置与每用户设置分离

当前项目用 `ULyraSettingsLocal`（Local Settings，本地设置）承载进程 / 机器级
入口，用 `ULyraSettingsShared`（Shared Settings，共享设置）承载按
LocalPlayer 和平台用户保存的数据，并由 `ULyraLocalPlayer` 缓存二者。用户
登录成功后，`ULyraGameInstance::HandlerUserInitialized()` 已进入共享设置加载
链；但 `ULyraSettingsShared::AsyncLoadOrCreateSettings()` 仍直接返回 `false`，
因此异步磁盘请求、回调替换和设置应用尚未发生。Experience 加载完成后也会
调用 `ULyraSettingsLocal::OnExperienceLoaded()`，当前目标函数为空。

> 🧠 **设计意图：** LocalPlayer 跨 World 存活，适合把 CommonUser（通用用户）
> 登录结果、每用户存档和运行时消费者连接起来；机器级设置则继续由引擎的
> `UGameUserSettings` 生命周期管理。

---

## 框架关系全景图

```
                    ┌────────────────────────────────────┐
                    │        DefaultEngine.ini           │
                    │  (所有引擎类替换注册)                  │
                    └──────────┬─────────────────────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
        ▼                      ▼                      ▼
┌───────────────┐   ┌──────────────────┐   ┌──────────────────┐
│ Editor 模块    │   │  System 框架      │   │  其他 Runtime 框架 │
│ (仅编辑器)     │   │  (所有构建)       │   │  (所有构建)       │
├───────────────┤   ├──────────────────┤   ├──────────────────┤
│ EditorEngine  │   │ LyraGameEngine   │   │ Game 框架         │
│ EditorModule  │   │ LyraGameInstance │   │ Player 框架       │
│ PIE 钩子      │   │ LyraAssetManager │   │ Character 框架    │
└───────┬───────┘   │ LyraGameData     │   │ UI 框架           │
        │           └────────┬─────────┘   │ Experience 框架   │
        │                    │             └──────────────────┘
        │           ┌────────┴─────────┐
        │           │   GameplayTags   │
        │           │   Logging         │
        │           │   Plugins (12)    │
        │           └──────────────────┘
        │
        ▼
┌──────────────────────────────────────────────────┐
│               Development Tools                   │
│  ULyraDeveloperSettings  (PIE 调试)               │
│  ULyraPlatformEmulationSettings (平台模拟)         │
└──────────────────────────────────────────────────┘
```

---

## 构建时间分类

| 分类 | 标记 | 包含内容 | 在 Shipping 中存在？ |
|------|------|---------|-------------------|
| **纯编辑器** | `[Editor-Only]` | LyraEditor 模块所有代码 | ❌ |
| **编辑器开发工具** | `[Editor-Dev]` | Development/ 下的设置类 | ✅ (类存在，WITH_EDITOR 代码被移除) |
| **运行时** | `[Runtime]` | 所有游戏框架代码 | ✅ |
| **非发布功能路径** | `[Non-Shipping Runtime]` | Cheat / External RPC 等由构建宏保护的行为 | 类型或声明可能仍编译；Shipping 中功能路径关闭 |

---

## 快速回顾

- **一句话架构：** Config 选择基础类，Experience 组合 GameFeature，
  GameState / PlayerState 保存网络状态，Pawn / UI 消费配置和运行结果。
- **当前主入口：** Engine / GameInstance 启动、GameMode Experience 分配、
  GameState Experience Manager。
- **网络分层：** GameMode 仅服务器；GameState 和 PlayerState 复制；
  LocalPlayer、Local / Shared Settings 和本地 UI 不复制。
- **当前完成度：** 整体**部分复刻**。
- **最重要的未完成项：** Pawn Extension → ASC Avatar → AbilitySet →
  Enhanced Input（增强输入）主链，以及共享设置异步加载 / 应用链。

## 复习要点

1. 为什么 Config 类替换是当前架构的最早入口？
2. Experience 与 GameFeature 分别负责配置和运行时组合的哪一部分？
3. GameMode、GameState、PlayerState 和 Pawn 的网络职责如何划分？
4. 哪些类只是结构占位，不能因父类可用而标记为已复刻？
5. 当前项目与 Lyra 原项目最大的系统性差异是什么？
6. 为什么 Local Settings 与 Shared Settings 要由不同对象和生命周期承载？

---

## 关联文档

| 文档 | 内容 |
|------|------|
| [02-Engine-Configuration.md](02-Engine-Configuration.md) | DefaultEngine.ini 所有映射 |
| [03-System-Framework.md](03-System-Framework.md) | 系统框架类详解 |
| [04-Game-Framework.md](04-Game-Framework.md) | 游戏框架类详解 |
| [05-Player-Framework.md](05-Player-Framework.md) | 玩家框架类详解 |
| [06-Character-Framework.md](06-Character-Framework.md) | 角色框架类详解 |
| [07-Experience-Framework.md](07-Experience-Framework.md) | **Experience 系统详解（核心）** |
| [08-UI-Framework.md](08-UI-Framework.md) | UI 与本地/共享设置框架详解 |
| [09-GameplayTags-System.md](09-GameplayTags-System.md) | GameplayTag 速查 |
| [10-Logging-System.md](10-Logging-System.md) | 日志通道速查 |
| [11-Development-Tools.md](11-Development-Tools.md) | 开发工具类详解 |
| [12-Editor-Module.md](12-Editor-Module.md) | 编辑器模块详解 |
| [13-Plugins-Catalog.md](13-Plugins-Catalog.md) | 插件目录 |
| [14-Inheritance-Chains.md](14-Inheritance-Chains.md) | 完整继承链速查 |
| [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md) | 端到端数据流 |
| [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) | 未完成的功能清单 |
| [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md) | 所有引擎生命周期函数详解（调用时机 + 适合写的逻辑） |
| [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) | 提交 `19e6961` 的 PlayerController、Camera、Replay 与开发入口源码审计 |
