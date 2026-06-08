# 01 - 架构总览

> Lyra 项目的顶层架构设计。在阅读具体框架文档前，先理解这张全景图。

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
      Character/                   ← 角色框架
      Development/                 ← 编辑器开发工具
      GameModes/                   ← Game 框架 + Experience 框架 + Experience Ready 异步节点
      Messages/                    ← Gameplay Message 通用 payload 与转换工具
      Player/                      ← Player 框架
      Replays/                     ← 回放能力判断与平台 Trait
      Settings/                    ← 游戏设置（部分仅日志声明）
      System/                      ← System 框架
      UI/                          ← UI 框架 + UI 管理子系统
  Plugins/                         ← 12 个插件（11 运行时 + 1 编辑器）
  Docs/                            ← 知识库文档
```

---

## 四大架构原则

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

`ALyraGameMode` 在地图启动后按 URL、PIE 开发设置、命令行、WorldSettings、Dedicated Server 和默认值的优先级决定本局 Experience，再交给 `ULyraExperienceManagerComponent` 加载。`ALyraGameState` 现在直接创建这个组件，并同时挂载 `ULyraAbilitySystemComponent`，作为比赛级 GAS/消息广播的宿主。

### 4. 数据驱动配置

所有全局配置使用 `UPrimaryDataAsset` 子类：

| 数据资产 | 内容 |
|---------|------|
| `ULyraGameData` | 全局 GE 引用（伤害/治疗/动态 Tag） |
| `ULyraPawnData` | 生成哪个 Pawn 类 |
| `ULyraExperienceDefinition` | 完整的玩法体验定义 |
| `ULyraExperienceActionSet` | 可复用的 Action + 插件打包 |
| `ULyraUserFacingExperienceDefinition` | 前端/Playlist 入口：地图、Experience、Session 参数、展示信息 |

运行时 Pawn 生成由 `ALyraGameMode` 根据 PawnData 决定 PawnClass，并通过 `ULyraPawnExtensionComponent` 预留 Pawn 初始化/扩展链入口。

### 5. Gameplay Message 解耦

新增的 `FLyraVerbMessage` 使用 `Verb` GameplayTag 表达事件通道，用 `Instigator`、`Target`、三组 Tag 容器和 `Magnitude` 描述一次通用 gameplay 事件。`ALyraGameState` 提供可靠/非可靠 NetMulticast，把服务器事件桥接到客户端的 `UGameplayMessageSubsystem`；`ULyraVerbMessageHelpers` 则负责 PlayerState/PlayerController 解析，以及与 `FGameplayCueParameters` 互转。

### 6. UI 与加载屏幕配置

`DefaultGame.ini` 现在配置 `ULyraUIManagerSubsystem` 的默认 `B_LyraUIPolicy`，并为 CommonLoadingScreen 指定 `W_LoadingScreen_Host`。运行时 `ULyraUIManagerSubsystem` 每帧把 `PrimaryGameLayout` 的可见性同步到 `AHUD::bShowHUD`，而 `UAsyncAction_ExperienceReady` 给蓝图提供等待 Experience 加载完成的统一入口。

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
| [08-UI-Framework.md](08-UI-Framework.md) | UI 框架类详解 |
| [09-GameplayTags-System.md](09-GameplayTags-System.md) | GameplayTag 速查 |
| [10-Logging-System.md](10-Logging-System.md) | 日志通道速查 |
| [11-Development-Tools.md](11-Development-Tools.md) | 开发工具类详解 |
| [12-Editor-Module.md](12-Editor-Module.md) | 编辑器模块详解 |
| [13-Plugins-Catalog.md](13-Plugins-Catalog.md) | 插件目录 |
| [14-Inheritance-Chains.md](14-Inheritance-Chains.md) | 完整继承链速查 |
| [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md) | 端到端数据流 |
| [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) | 未完成的功能清单 |
| [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md) | 所有引擎生命周期函数详解（调用时机 + 适合写的逻辑） |
