# 05 - Player 框架

> 本地玩家、玩家控制器、玩家状态、出生点和重生管理。这些类管理玩家从登录、生成到游戏中操作的全流程。

---

## 框架概述

Player 框架涵盖所有代表"玩家"的对象层次：
- `ULyraLocalPlayer` — 本地玩家的引擎级表示（UI 绑定等）
- `ALyraPlayerController` — 控制 Pawn 的控制器（输入、相机等）
- `ALyraPlayerState` — 复制到所有客户端的玩家状态、ASC、PawnData、Team/Squad 与 StatTag 数据
- `ALyraPlayerStart` / `ULyraPlayerSpawningManagerComponent` — 出生点占用检测、缓存和重生扩展点

**设计意图:**
- 通过 CommonUI 插件基类（`UCommonLocalPlayer`、`ACommonPlayerController`）实现增强的 UI 和本地玩家集成
- PlayerState 使用 Modular 基类以实现 GameFeature 组件注入，同时直接接入玩家级 AbilitySystemComponent 和 Team Agent 接口
- 提供类型化访问器以在 Lyra 类之间安全导航
- 将重生选择逻辑从 GameMode 中拆出为 GameStateComponent，方便不同 Experience 注入不同出生规则

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ULyraLocalPlayer` | `UCommonLocalPlayer` | [Runtime] | 本地玩家包装器（CommonUser 集成） |
| `ALyraPlayerController` | `ACommonPlayerController`、`ILyraTeamAgentInterface`、`ILyraCameraAssistInterface` | [Runtime] | 输入、PlayerState、队伍、相机辅助与观战视角的协调者 |
| `ALyraReplayPlayerController` | `ALyraPlayerController` | [Runtime] | 回放播放期间跟随录制者 Pawn |
| `ULyraCameraAssistInterface` / `ILyraCameraAssistInterface` | `UInterface` / C++ 接口 | [Runtime] | 相机穿透协作协议 |
| `ALyraPlayerCameraManager` | `APlayerCameraManager` | [Runtime] | 当前使用引擎默认行为的 Lyra 相机管理器类型 |
| `ALyraPlayerState` | `AModularPlayerState`, `IAbilitySystemInterface`, `ILyraTeamAgentInterface` | [Runtime] 🧩 | 可复制的玩家状态、ASC、PawnData、Team/Squad 和 StatTag 数据 |
| `ALyraPlayerStart` | `APlayerStart` | [Runtime] | Lyra 出生点，占用检测与短期 Claim |
| `ULyraPlayerSpawningManagerComponent` | `UGameStateComponent` | [Runtime] | 出生点缓存、选择和重生扩展点 |

> 🧩 = 使用 Modular 基类，GameFeature 可注入组件

---

## 逐类详解

### ULyraLocalPlayer [Runtime]

**继承链:** `UObject → ULocalPlayer → UCommonLocalPlayer → ULyraLocalPlayer`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 本地玩家的引擎侧表示。当前为最小实现。

**接口:** 本类中的 `ILyraTeamAgentInterface` 仍注释掉；当前团队状态先由 `ALyraPlayerState` 承载。

**与 CommonUser 的关系:**
`UCommonLocalPlayer`（CommonUI 插件）提供了与 CommonUser 框架的集成。`ULyraGameInstance::HandlerUserInitialized()` 中有注释掉的代码调用 `LoadSharedSettingsFromDisk()` 来为此 LocalPlayer 加载设置。

---

### ALyraPlayerController [Runtime]

**继承链:** `AActor → AController → APlayerController → ACommonPlayerController → ALyraPlayerController`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "..."))`

**职责:** 基础玩家控制器（PlayerController）。它把“本地输入与相机”“可复制的玩家状态（PlayerState）”“当前角色实体（Pawn）”连接起来；本工作区还让它承担队伍（Team）转发、自动奔跑（Auto-run）和观战视角旋转同步的部分职责。

**类型化访问器:**
- `GetLyraPlayerState()` — 将 `PlayerState` 转换为 `ALyraPlayerState*`
- `GetLyraAbilitySystemComponent()` — 从 `ALyraPlayerState` 取得玩家级能力系统组件（Ability System Component，ASC）
- `GetLyraHUD()` — 将 `MyHUD` 转换为 `ALyraHUD*`

**当前接口状态（工作区快照，未运行验证）:**

- 已实现 `ILyraTeamAgentInterface`：`GetGenericTeamId()` 从 `PlayerState` 读取队伍编号；`SetGenericTeamId()` 明确拒绝写入；`BroadcastOnPlayerStateChanged()` 负责迁移队伍变化委托（Delegate）。因此队伍数据的权威来源仍是 `ALyraPlayerState`，控制器只是面向控制器使用者的代理。
- 已实现 `ILyraCameraAssistInterface`：`OnCameraPenetratingTarget()` 会把一次性隐藏标志设为 `true`，`UpdateHiddenComponents()` 在下一次视图构建时隐藏当前 View Target（视图目标）及其直接附加的 Primitive Component（图元组件），然后消费并重置该标志。
- 当前项目没有 `LyraCameraMode_ThirdPerson` 等调用 `OnCameraPenetratingTarget()` 的相机碰撞生产端，所以隐藏消费者虽然已写入，正常游戏中仍没有静态可见的触发来源。
- 构造函数设置 `PlayerCameraManagerClass = ALyraPlayerCameraManager::StaticClass()`。当前自定义类没有 Lyra 专属成员或重写，但继承的 `APlayerCameraManager` 仍提供完整的引擎基础相机行为；缺少的是原项目的 UI Camera（界面相机）组件、80° 默认 FOV（视野角）、视图更新覆盖和调试显示。

**当前关键调用点（工作区快照）:**

| 引擎钩子 | 当前代码行为 | 理解时的重点 |
|------|------|------|
| 构造函数 / `PostInitializeComponents()` | 选择 `ALyraPlayerCameraManager` 和非发布构建的 `ULyraCheatManager`；引擎稍后生成相机管理器并尝试添加 Cheat Manager（作弊管理器） | 选择子类不等于 Lyra 专属行为已实现；空子类仍继承父类功能 |
| `BeginPlay()` | 非发布构建调用 `StartAllListeners()`、解析 `rpcport`，但注册对象和路由调用被注释；最后取消隐藏 Controller Actor（控制器 Actor） | 启动已有 HTTP Listener（监听器）不等于已经创建路由或端点 |
| `GetLifetimeReplicatedProps()` | 禁用父类 `TargetViewRotation` 的 `COND_OwnerOnly` 复制 | 观战旋转改由 `PlayerState::ReplicatedViewRotation` 承载，并以 `COND_SkipOwner` 复制给非拥有者 |
| `OnPossess()` | 先调用 `Super`，编辑器服务器条件下执行配置的作弊命令，随后关闭自动奔跑 | `Super` 完成 Pawn 所有权与控制关系；其后才能以 `GetPawn()` 作为已控制角色读取 |
| `OnUnPossess()` | 在 `Super` 之前，若 ASC 的 Avatar（化身）正是即将解除控制的 Pawn，则将 Avatar 清空 | ASC 的 OwnerActor（拥有者）仍是 PlayerState，AvatarActor（化身）才是会切换的 Pawn；两者不能混为一谈 |
| `OnRep_PlayerState()` | 广播 PlayerState 改变；客户端刷新 ASC 的 ActorInfo | `TryActivateAbilitiesOnSpawn()` 仍被注释，因此这里只是补刷新，不是完整的出生技能激活 |
| `PlayerTick()` | 自动奔跑时添加前向移动输入；同步或消费 `ReplicatedViewRotation` | UE 5.7.4 的服务器远程 Controller 会走 `TickActor()` 的专用分支并绕过 `PlayerTick()`；`HasAuthority()` 不等于所有服务器 Controller 都会执行这里 |
| `UpdateHiddenComponents()` | 一次性把 View Target 的已注册图元及未标记 `NoParentAutoHide` 的直接附加图元加入隐藏集合 | UE 5.7.4 在 `ULocalPlayer::CalcSceneView()` 构建每个视图时调用该链；当前缺少相机碰撞触发者 |
| `PostProcessInput()` | 取得 ASC，但 `ProcessAbilityInput()` 调用仍被注释，之后调用 `Super` | 说明 GAS 输入分发尚未完成；自动奔跑标签不等同于技能输入链完整 |

> 详细的引擎入口、Lyra 原项目差异、状态变化和验证清单见 [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)。

**子类:** `ALyraReplayPlayerController` 已实现 `Tick()`、`SmoothTargetViewRotation()`、`ShouldRecordClientReplay()` 和录制者 Pawn 变化回调。它会在回放拖动或检查点恢复导致旧 PlayerState 失效后，重新绑定 `ALyraGameState::OnRecorderPlayerStateChangedEvent`，再把 View Target（视图目标）切到录制者的新 Pawn；这部分与 Lyra 原项目静态对齐。它始终拒绝客户端录制，而基础控制器的录制资格检查当前也必定返回 `false`；即使派生类放开资格，真正的 `RecordClientReplay()` 调用仍被注释，却会在只设置 `RecorderPlayerState` 后返回 `true`，因此客户端录制仍属于明确缺口。

**新增命名空间 (Lyra::Input):**

`ALyraPlayerController.cpp` 中定义了 `Lyra::Input` 命名空间，包含力反馈 CVar：

```cpp
namespace Lyra::Input
{
    static int32 ShouldAlwaysPlayForceFeedBack = 0;
    static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(
        TEXT("LyraPC.ShouldAlwaysPlayForceFeedback"), ...);
}
```

`ULyraDeveloperSettings::bShouldAlwaysPlayForceFeedback` 通过 `ConsoleVariable` meta 标签与此 CVar 绑定。当在 Project Settings 中勾选此选项时，即使上次输入设备不是手柄，力反馈也会播放。

---

### ILyraCameraAssistInterface [Runtime]

**组成:** `ULyraCameraAssistInterface` 提供 Unreal Reflection（虚幻反射）类型，`ILyraCameraAssistInterface` 提供 C++ 协议。

**当前默认行为:** `GetIgnoredActorsForCameraPenetration()` 不追加 Actor，`GetCameraPreventPenetrationTarget()` 返回未设置的 `TOptional`，`OnCameraPenetratingTarget()` 为空。`ALyraPlayerController` 只重写最后一个回调。

**与 Lyra 原项目的差异:** 原项目第三人称相机模式在穿透检测命中时，把 Controller（控制器）、View Target（视图目标）和防穿透目标转换为该接口并调用回调。当前项目没有对应 Camera Mode（相机模式），所以接口只能视为“消费者已接入、生产者缺失”。

---

### ALyraPlayerCameraManager [Runtime]

**继承链:** `AActor → APlayerCameraManager → ALyraPlayerCameraManager`

**当前行为:** 类体没有自定义字段或重写，`.cpp` 只包含头文件。UE 5.7.4 父类仍负责 View Target（视图目标）、镜头缓存、混合、旋转限制和相机更新，默认 FOV 为 90°、Pitch（俯仰）范围约为 -89.9° 到 89.9°。

**Lyra 原设计:** 构造时改为 80° FOV 和 -89° 到 89° Pitch，创建 `ULyraUICameraManagerComponent`，并重写 `UpdateViewTarget()` 与 `DisplayDebug()`。这些 Lyra 专属能力尚未进入当前项目。

---

### ALyraPlayerState [Runtime] 🧩

**继承链:** `AActor → AInfo → APlayerState → AModularPlayerState → ALyraPlayerState`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 可复制的玩家状态数据。当前已经接入玩家级 `ULyraAbilitySystemComponent`、复制的 `PawnData`、连接类型、Team/Squad、StatTag Stack、观战视角旋转，以及面向客户端的 gameplay message 转发。

**关键方法:**
- `GetLyraPlayerController()` — 将 Owner 转换为 `ALyraPlayerController*`
- `GetLyraAbilitySystemComponent()` / `GetAbilitySystemComponent()` — 返回玩家级 ASC。
- `GetPawnData<T>()` — 返回复制的 `PawnData` 并按模板类型转换；GameMode 会优先使用它，再回退到 Experience 默认 PawnData 和 AssetManager 默认 PawnData。
- `SetPawnData(const ULyraPawnData*)` — 仅 Authority 可写，一次性设置 `PawnData`，标记 Push Model dirty，预留 AbilitySet 授予 TODO，并发送 `LyraAbilitiesReady` 扩展事件。
- `SetPlayerConnectionType()` / `GetPlayerConnectionType()` — 管理玩家、实时旁观者、回放旁观者、InactivePlayer 等连接状态。
- `SetGenericTeamId()` / `GetGenericTeamId()` / `GetOnTeamIndexChangedDelegate()` — 实现 `ILyraTeamAgentInterface`，队伍变化时广播 `FOnLyraTeamIndexChangedDelegate`。
- `SetSquadID()` / `GetSquadId()` / `GetTeamId()` — 管理队伍下的小队 ID 和蓝图可读的队伍整数 ID。
- `AddStatTagStack()` / `RemoveStatTagStack()` / `GetStatTagStackCount()` / `HasStatTag()` — 通过 `FGameplayTagStackContainer` 管理可复制的玩家统计标签栈。
- `ClientBroadcastMessage()` — Unreliable Client RPC，只在客户端把 `FLyraVerbMessage` 广播到 `UGameplayMessageSubsystem`。
- `GetReplicatedViewRotation()` / `SetReplicatedViewRotation()` — 复制观战视角旋转；复制条件为 `COND_SkipOwner`。

**初始化与 Experience 关系:**
- 构造函数创建 `ULyraAbilitySystemComponent`，开启复制并使用 `EGameplayEffectReplicationMode::Mixed`，同时把 `MyTeamID` 初始化为 `NoTeam`、`MySquadID` 初始化为 `INDEX_NONE`。
- `PostInitializeComponents()` 调用 `AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn())`。
- 服务器 GameWorld 中会向 `ULyraExperienceManagerComponent` 注册 `OnExperienceLoaded()` 回调；Experience 就绪后通过 `ALyraGameMode::GetPawnDataForController()` 选择 PawnData 并写入 PlayerState。

**复制字段:**
- Push Model 复制：`PawnData`、`MyPlayerConnectionType`、`MyTeamID`、`MySquadID`。
- 普通复制：`StatTags`，内部使用 FastArray 增量复制。
- 条件复制：`ReplicatedViewRotation` 使用 `COND_SkipOwner`，只同步给非拥有者。

**Team Agent 接口:**
`ULyraTeamAgentInterface` 继承自 `UGenericTeamAgentInterface`，禁止蓝图实现。辅助函数 `GenericTeamIdToInteger()` / `IntegerToGenericTeamId()` 在 `FGenericTeamId::NoTeam` 和 `INDEX_NONE` 之间转换；`ConditionalBroadcastTeamChanged()` 只有在队伍实际变化时打印 `LogLyraTeams` 并广播委托。

**StatTag Stack:**
`FGameplayTagStackContainer` 使用 `FFastArraySerializer` 复制 `FGameplayTagStack` 数组，并维护本地 `TagToCountMap` 加速查询。`AddStack()` / `RemoveStack()` 只接受有效 GameplayTag，数量小于 1 时不改变状态；客户端通过 FastArray 的 `PreReplicatedRemove()`、`PostReplicatedAdd()`、`PostReplicatedChange()` 同步查询 Map。

**Modular 基类的好处:**
`AModularPlayerState` 注册到 `UGameFrameworkComponentManager`。GameFeature Action 仍可以在 Experience 加载期间向 PlayerState 添加额外组件；当前 ASC 已作为默认子对象直接存在，后续更适合把 AbilitySet 授予、初始化状态和 PawnExtension 协作接到这条链上。

---

### ALyraPlayerStart [Runtime]

**继承链:** `AActor → ANavigationObjectBase → APlayerStart → ALyraPlayerStart`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** Lyra 专用出生点。它在普通 `APlayerStart` 基础上增加了占用检测、临时 Claim 和标签容器，为多人出生和 Experience 自定义出生规则做准备。

**占用状态枚举:** `ELyraPlayerStartLocationOccupancy`

| 值 | 含义 | 选择优先级 |
|----|------|-----------|
| `Empty` | 当前默认 Pawn 可以直接放下，无阻挡碰撞 | 最高 |
| `Partial` | 原点有阻挡，但 `FindTeleportSpot()` 能找到附近可用位置 | 次选 |
| `Full` | 无法放下 Pawn | 不选择 |

**关键方法:**
- `GetLocationOccupancy(AController*)` — 通过当前 GameMode 的 `GetDefaultPawnClassForController()` 取得 Pawn CDO，再用 `EncroachingBlockingGeometry()` / `FindTeleportSpot()` 判断是否可用。
- `TryClaim(AController*)` — 如果未被占用，则把该 Controller 记录为 `ClaimingController`，并启动定时器周期检查。
- `CheckUnclaimed()` — 当 ClaimingController 已拥有 Pawn，且该出生点重新变为空闲时，清除 Claim 并停止定时器。

**属性:**
- `ExpirationCheckInterval` — Claim 释放检查间隔，默认 1 秒。
- `StartPointTags` — 出生点标签容器，当前默认选择逻辑还未使用，可供后续按队伍、模式、区域筛选。

---

### ULyraPlayerSpawningManagerComponent [Runtime]

**继承链:** `UObject → UActorComponent → UGameStateComponent → ULyraPlayerSpawningManagerComponent`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 出生点管理组件。它设计为挂在 GameState 上，由 Experience/GameFeature 按需注入，再由 `ALyraGameMode` 代理调用，从而让不同 Experience 能替换出生规则。

**初始化行为 (`InitializeComponent()`):**
1. 绑定 `FWorldDelegates::LevelAddedToWorld`，监听流式关卡加入。
2. 绑定 `UWorld::AddOnActorSpawnedHandler()`，监听运行时生成的 `ALyraPlayerStart`。
3. 遍历当前 World 中已有的 `ALyraPlayerStart`，缓存到 `CachedPlayerStarts`。

**出生点选择流程:**

```
ChoosePlayerStart(Controller)
  ├── [Editor] PlayerController 优先使用 APlayerStartPIE (Play From Here)
  ├── 清理 CachedPlayerStarts 中已失效的弱引用
  ├── Spectator: 随机选择一个 StartPoint，不 Claim
  ├── OnChoosePlayerStart(Player, StartPoints) 允许 C++ 子类自定义
  ├── 默认逻辑: Empty 随机优先，其次 Partial 随机
  └── 如果选中 ALyraPlayerStart → TryClaim(Player)
```

**扩展点:**
- `OnChoosePlayerStart(AController*, TArray<ALyraPlayerStart*>&)` — C++ 子类覆写，用于按队伍、距离、权重或标签选择出生点。
- `OnFinishRestartPlayer(AController*, const FRotator&)` — C++ 子类覆写的重生完成钩子。
- `K2_OnFinishRestartPlayer` — 蓝图实现事件，方便 Experience 蓝图逻辑响应重生完成。

**当前接入状态:**
`ALyraGameMode` 已经覆盖并代理引擎的出生/重生函数到该组件：
- `ChoosePlayerStart_Implementation()` → `ULyraPlayerSpawningManagerComponent::ChoosePlayerStart()`
- `ControllerCanRestart()` → `ULyraPlayerSpawningManagerComponent::ControllerCanRestart()`
- `FinishRestartPlayer()` → `ULyraPlayerSpawningManagerComponent::FinishRestartPlayer()`

组件侧的 `ControllerCanRestart()` 当前仍是最小实现，直接允许重生；死亡状态、比赛状态、队伍规则等限制需要后续扩展。

---

## 框架内部关系

```
ULyraLocalPlayer
  └── 由 ULyraGameInstance 管理（1 个本地玩家 = 1 个控制器）

ALyraPlayerController
  ├── PlayerState → ALyraPlayerState
  ├── Pawn → ALyraCharacter
  └── MyHUD → ALyraHUD

ALyraPlayerState (复制到所有客户端)
  ├── GetLyraPlayerController() → ALyraPlayerController
  ├── IAbilitySystemInterface → ULyraAbilitySystemComponent
  ├── ILyraTeamAgentInterface → TeamID 变化委托
  ├── PawnData → 复制的玩家级 Pawn 配置
  └── StatTags → FGameplayTagStackContainer

ULyraPlayerSpawningManagerComponent (GameStateComponent)
  ├── CachedPlayerStarts[] → ALyraPlayerStart
  ├── ChoosePlayerStart() → Empty/Partial 优先级选择
  └── 由 ALyraGameMode 出生/重生流程调用

ALyraPlayerStart
  └── ClaimingController → 短期防止多个玩家抢同一出生点
```

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 管理 ULyraLocalPlayer
- [06-Character-Framework.md](06-Character-Framework.md) — PlayerController 控制的 Pawn 通常是 ALyraCharacter
- [08-UI-Framework.md](08-UI-Framework.md) — PlayerController 持有对 ALyraHUD 的引用
- [09-GameplayTags-System.md](09-GameplayTags-System.md) — StatTag Stack 使用 GameplayTag 表达玩家统计状态
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — PlayerState 已接入 GAS/Team，Character/Controller/LocalPlayer 仍有接口与初始化 TODO
- [04-Game-Framework.md](04-Game-Framework.md) — ALyraGameMode 代理出生/重生流程，ALyraWorldSettings 在 Map Check 中要求使用 ALyraPlayerStart
