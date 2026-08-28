# 04 - Game 框架

> 游戏模式、游戏状态、游戏会话和世界设置。这些类构成了 Lyra 的"比赛"层。

> **核对基线：** 当前项目最新提交 `19e6961`；当前项目与 Lyra 参考项目
> 均声明 UE 5.7，生命周期和网络复制按 Unreal Engine 5.7.4 源码核对。
> 文档本次复核日期为 2026-07-13，未执行 Dedicated Server（专用服务器）、
> Session（会话）、Replay（回放）或资产运行验证。

---

## 框架概述

**问题：** 一局游戏需要选择 Experience（游戏体验配置）、创建玩家对象、
保存全局复制状态并决定 Pawn 生成时机；若把所有规则固定在 GameMode 中，
客户端无法共享状态，GameFeature（游戏功能）也难以按 Experience 动态组合。

**当前项目的解决方案：** GameMode 只存在于服务器，负责 Experience 分配
和 Pawn 生成；GameState 复制到客户端并承载 Experience Manager、比赛级
ASC（Ability System Component，能力系统组件）和消息桥；WorldSettings
保存地图默认 Experience；UserFacingExperience（面向用户的体验）把前端选择
转换为 Session 请求。

**设计意图:**

- GameMode 负责 Experience 分配、玩家进入时机和 Pawn 生成管线，实际玩法规则仍由 Experience 的 GameFeatureAction 注入
- GameState 作为 Experience 加载状态机、比赛级 AbilitySystemComponent、客户端 gameplay message 广播的宿主
- WorldSettings 承载每关卡级别的 Experience 选择
- UserFacingExperience 承载前端/Playlist 入口，把玩家看到的游戏模式转换为可创建的 Session 请求

**当前完成度：** Experience 选择、等待加载后生成 Pawn、GameState ASC、
消息广播和基础 Session 请求已经接入；Pawn Extension、完整比赛规则、
Replay 客户端录制以及部分 Dedicated Server / 资产路径仍未完整验证。

---

## 当前复刻状态

| 模块 | 当前状态 | 当前实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| Experience 分配 | **部分复刻** | URL、PIE、命令行、WorldSettings、Dedicated Server 和默认值路径存在 | Lyra 完整流程还有更多在线服务与错误处理 | 基础选择可用，在线与失败分支需验证 |
| 玩家生成 | **部分复刻** | 等待 Experience，按 PawnData 选类并代理出生点 | Lyra 完成 Pawn Extension / Ability 初始化 | Pawn 可以生成，但玩家初始化主链不完整 |
| GameState 共享状态 | **部分复刻** | Experience Manager、比赛级 ASC、ServerFPS、Recorder PlayerState 和消息广播存在 | Lyra 有更多比赛系统和监听者 | 核心宿主已建立，关联功能仍不完整 |
| GameSession（游戏会话） | **已复刻** | `ProcessAutoLogin()` 返回 true；两个比赛钩子调用 `Super` | 当前代码与 Lyra 5.7 对应类一致 | 类职责本身很窄，不等于完整比赛状态机 |
| UserFacingExperience | **部分复刻** | 能创建 Host Session 请求并附加 Experience / DemoRec | Lyra 在完整前端与在线服务中消费 | C++ 路径存在，资产与在线运行待确认 |
| WorldSettings（世界设置） | **部分复刻** | 地图默认 Experience 与 Map Check（地图检查）存在 | Lyra 同时依赖完整地图资产配置 | 字段存在不代表每张地图已正确配置 |

---

## 类列表

| 类 | 父类或接口 | 生命周期 | 网络位置 | 当前状态 | 职责 |
|---|---|---|---|---|---|
| `ALyraGameMode` | `AModularGameModeBase` | Runtime 🧩 | Server Only（仅服务器），不复制 | **部分复刻** | Experience 分配、玩家初始化、Pawn 生成和重生代理 |
| `ALyraGameState` | `AModularGameStateBase`、`IAbilitySystemInterface` | Runtime 🧩 | Server Authority，复制到客户端 | **部分复刻** | 承载 Experience、比赛级 ASC 和共享状态 |
| `ALyraGameSession` | `AGameSession` | Runtime | Server Only | **已复刻** | 禁用默认自动登录并保留比赛生命周期钩子 |
| `ALyraWorldSettings` | `AWorldSettings` | Runtime + Editor-Only 部分 | World 配置；Server / Client 均可存在 | **部分复刻** | 保存关卡默认 Experience 并执行 Map Check |
| `ULyraUserFacingExperienceDefinition` | `UPrimaryDataAsset` | Runtime，配置资产 | 不直接复制 | **部分复刻** | 前端 Playlist 配置并创建 Host Session 请求 |

> 🧩 = 使用 Modular 基类，GameFeature 可注入组件

---

## 逐类详解

### ALyraGameMode [Runtime] 🧩

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：`Source/LyraGame/GameModes/LyraGameMode.h/.cpp`。
- Lyra 参考：`Source/LyraGame/GameModes/LyraGameMode.h/.cpp`。
- 引擎父类：`Engine/Source/Runtime/Engine/Private/GameModeBase.cpp`。

**继承链:** `AActor → AInfo → AGameModeBase → AModularGameModeBase → ALyraGameMode`

**UCLASS:**
`UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "该项目使用的基础GameMode类"))`

**职责:** 服务器权威的游戏模式入口。负责选择并启动 Experience、控制玩家何时生成 Pawn、根据 PawnData 决定 PawnClass，并把出生/重生细节转发给组件化系统。

**创建位置与所有权：** UE 只在 Authority World（权威世界）创建 GameMode，
World 持有该 Actor。`DefaultEngine.ini` 的 `GlobalDefaultGameMode` 指向
`B_LyraGameMode` 蓝图子类，因此最终类默认值可能覆盖以下 C++ 构造值。

**构造函数实现:** 在构造函数中设置所有默认游戏框架类映射，将引擎默认类全部替换为 Lyra 自定义类：

| 属性 | 设置值 |
|------|--------|
| `GameStateClass` | `ALyraGameState::StaticClass()` |
| `GameSessionClass` | `ALyraGameSession::StaticClass()` |
| `PlayerControllerClass` | `ALyraPlayerController::StaticClass()` |
| `ReplaySpectatorPlayerControllerClass` | `ALyraReplayPlayerController::StaticClass()` |
| `PlayerStateClass` | `ALyraPlayerState::StaticClass()` |
| `DefaultPawnClass` | `ALyraCharacter::StaticClass()` |
| `HUDClass` | `ALyraHUD::StaticClass()` |

> ⚠️ **注意:** 蓝图子类 `B_LyraGameMode` 可以覆盖这些默认值。`DefaultEngine.ini` 中 `GlobalDefaultGameMode=/Game/B_LyraGameMode.B_LyraGameMode_C` 使用的是蓝图子类。

**关键设计决策:**

- 继承 `AGameModeBase`（而非 `AGameMode`）— 不使用传统的 MatchState 驱动流程（WaitingToStart → InProgress → WaitingPostMatch 等）
- 继承 `AModularGameModeBase` — 注册到 `UGameFrameworkComponentManager`，允许 GameFeature Action 注入组件
- 不让玩家在 Experience 未加载完成前生成 Pawn，避免 PawnClass、PawnData、输入/UI/Ability 等依赖尚未准备好
- 将出生点选择和重生判断转发给 `ULyraPlayerSpawningManagerComponent`，使不同 Experience 可以替换重生系统

**与 Experience 的关系:**
GameMode 现在负责选择“本局使用哪个 Experience”，然后调用 `ULyraExperienceManagerComponent::SetCurrentExperience()` 启动加载。ExperienceManagerComponent 仍然是实际加载状态机，负责加载资产、激活 GameFeature、执行 Action 并广播加载完成。

**Experience 分配入口:**

- `InitGame()` 在父类初始化后，用下一帧定时器调用 `HandleMatchAssignmentIfNotExpectingOne()`，避免在 World/GameState 尚未稳定时立即分配。
- `InitGameState()` 获取 `ULyraExperienceManagerComponent`，通过 `CallOrRegister_OnExperienceLoaded()` 把 `OnExperienceLoaded()` 注册到 Normal Priority（普通优先级）委托。
- `OnMatchAssignmentGiven()` 在最终得到有效 `FPrimaryAssetId` 后调用 `SetCurrentExperience()`。

**Experience 选择优先级:**

| 优先级 | 来源 | 说明 |
|--------|------|------|
| 1 | URL Option `?Experience=` | 例如 `/Game/Maps/L_Map?Experience=B_LyraDefaultExperience` |
| 2 | `ULyraDeveloperSettings::ExperienceOverride` | 仅 PIE，用于开发调试覆盖 |
| 3 | 命令行 `-Experience=` | 可写完整 `Type:Name`，也可只写资产名 |
| 4 | `ALyraWorldSettings::DefaultGameplayExperience` | 地图默认配置 |
| 5 | Dedicated Server 自动登录/Host 流程 | 默认地图上的专用服务器会先尝试在线登录并 Host Session |
| 6 | 硬编码默认 `LyraExperienceDefinition:B_LyraDefaultExperience` | 最终兜底 |

**玩家生成与重生管线:**

- `HandleStartingNewPlayer_Implementation()` 只有在 `IsExperienceLoaded()` 为 true 时才调用父类生成逻辑；否则等待 Experience 完成。
- `OnExperienceLoaded()` 遍历世界中的 `PlayerController`，对没有 Pawn 且允许重启的玩家调用 `RestartPlayer()`。
- `GetPawnDataForController()` 优先使用 `ALyraPlayerState::GetPawnData<ULyraPawnData>()`，其次使用当前 Experience 的 `DefaultPawnData`，最后使用 `ULyraAssetManager::GetDefaultPawnData()`。
- `ALyraPlayerState::PostInitializeComponents()` 也会在服务器上监听同一个 Normal Priority（普通优先级）Experience Loaded 委托，并通过 `ALyraGameMode::GetPawnDataForController()` 一次性写入复制的 `PawnData`。两者没有 High Priority（高优先级）与普通优先级的硬性排序保证；即使重启时 `PawnData` 尚未写入，`GetPawnDataForController()` 仍会回退到 Experience 默认值。
- `GetDefaultPawnClassForController_Implementation()` 从 PawnData 中取 `PawnClass`。
- `SpawnDefaultPawnAtTransform_Implementation()` 使用 deferred spawn，先生成 Pawn，再 `FinishSpawning()`；`ULyraPawnExtensionComponent` 相关初始化仍是 TODO。
- `ChoosePlayerStart_Implementation()`、`PlayerCanRestart_Implementation()`、`FinishRestartPlayer()` 会代理到 `ULyraPlayerSpawningManagerComponent`。
- `FailedToRestartPlayer()` 在存在 PawnClass 且仍可重启时，通过 `RequestPlayerRestartNextFrame()` 下一帧重试。

**Dedicated Server Host 流程:**
`TryDedicatedServerLogin()` 只在 Dedicated Server 打开默认地图时触发，先通过 `UCommonUserSubsystem` 尝试在线登录。登录完成后调用 `HostDedicatedServerMatch()`，按命令行 `-UserExperience=` / `-Playlist=` 查找 `ULyraUserFacingExperienceDefinition`；若未指定或未找到，则使用标记为 `bIsDefaultExperience` 的 Playlist，并通过 `UCommonSessionSubsystem::HostSession()` 启动会话和地图旅行。

**委托:**

- `OnGameModePlayerInitialized` — `GenericPlayerInitialization()` 调用父类逻辑后广播，通知外部系统某个 Controller 已完成 GameMode 级初始化。

---

### ALyraGameState [Runtime] 🧩

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：`Source/LyraGame/GameModes/LyraGameState.h/.cpp`。
- Lyra 参考：`Source/LyraGame/GameModes/LyraGameState.h/.cpp`。
- 引擎父类：`Engine/Source/Runtime/Engine/Private/GameStateBase.cpp`。

**继承链:** `AActor → AInfo → AGameStateBase → AModularGameStateBase → ALyraGameState`，并实现 `IAbilitySystemInterface`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 比赛级共享状态。关键载体包括 `ULyraExperienceManagerComponent`、`ULyraAbilitySystemComponent`、服务器 FPS 复制、Replay 录制者 PlayerState，以及服务器到客户端的通用 gameplay message 广播。

**创建位置与所有权：** 服务器按 `GameStateClass` 创建权威 GameState，
再复制到客户端。Experience Manager 和比赛级 ASC 是 GameState 的
Default Subobject（默认子对象），Outer / Owner 生命周期随 GameState。

**构造函数:** 启用 Tick：`PrimaryActorTick.bCanEverTick = true; bStartWithTickEnabled = true;`。同时创建：
- `AbilitySystemComponent` — `ULyraAbilitySystemComponent`，开启复制并使用 `EGameplayEffectReplicationMode::Mixed`
- `ExperienceManagerComponent` — `ULyraExperienceManagerComponent`，作为本局 Experience 状态机
- `ServerFPS` — 初始化为 `0.0f`

**GAS 接入:** `ALyraGameState` 已实现 `IAbilitySystemInterface`。`PostInitializeComponents()` 中调用 `AbilitySystemComponent->InitAbilityActorInfo(this, this)`，让 GameState 本身同时作为 OwnerActor 和 AvatarActor。`GetAbilitySystemComponent()` 返回通用 `UAbilitySystemComponent*`，`GetLyraAbilitySystemComponent()` 返回类型化的 `ULyraAbilitySystemComponent*`。

**Tick 与复制状态:**

- `Tick()` 只在 Authority 上把全局 `GAverageFPS` 写入 `ServerFPS`，该字段通过 `DOREPLIFETIME` 复制给客户端。
- `RecorderPlayerState` 使用 `ReplicatedUsing=OnRep_RecorderPlayerState`，并通过 `DOREPLIFETIME_CONDITION(..., COND_ReplayOnly)` 只在 Replay 场景复制。
- `OnRep_RecorderPlayerState()` 广播 `OnRecorderPlayerStateChangedEvent`，供回放视角或 UI 监听。

**玩家列表与无缝旅行:**

- `AddPlayerState()` 当前只调用 Super，保留扩展点。
- `RemovePlayerState()` 当前也只调用 Super，并带有注释说明 `AGameModeBase` 路径下可能不会像完整版 `AGameMode` 那样被调用。
- `SeamlessTravelTransitionCheckpoint()` 会在无缝旅行 checkpoint 中从 `PlayerArray` 移除 Bot 或 inactive 的 PlayerState，避免它们被错误带入下一张地图。

**客户端消息广播:**

- `MulticastMessageToClients()` 使用 Unreliable NetMulticast，适合淘汰提示、加入提示等允许丢失的客户端通知。
- `MulticastReliableMessageToClients()` 使用 Reliable NetMulticast，内部复用同一套广播逻辑，适合不能丢的通知。
- 客户端收到后调用 `UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message)`，以 `FLyraVerbMessage::Verb` 作为消息通道。

**Modular 基类的好处:**
`AModularGameStateBase` 注册到 `UGameFrameworkComponentManager`。即使 ExperienceManagerComponent 和 ASC 已作为默认子对象创建，GameFeature Action 仍可以在 Experience 加载期间继续向 GameState 添加其他组件。

---

### ALyraGameSession [Runtime]

**当前状态：** **已复刻**

**继承链:** `UObject → AActor → AInfo → AGameSession → ALyraGameSession`

**UCLASS:** `UCLASS(Config = Game)`

**职责:** 会话管理。

**重写的生命周期函数:**

##### `ProcessAutoLogin()`

> ⏱️ **引擎调用时机:** UE 5.7.4 的 `AGameModeBase::InitGame()` 创建并初始化
> GameSession 后，在非 Standalone（非独立运行）且当前会话不存在时直接调用。
>
> **适合写的逻辑:** 发起平台自动登录，并准确返回“异步登录是否正在进行”。
> 返回 false 时，`AGameModeBase::InitGame()` 会继续调用 `RegisterServer()`；
> 返回 true 时会跳过该默认注册步骤。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 11](17-Engine-Lifecycle-Reference.md#11-游戏会话生命周期)

当前返回 `true`，使 `AGameModeBase::InitGame()` 不再调用
`GameSession->RegisterServer()`；实际 Dedicated Server 登录和 Host（托管）逻辑
由 `ALyraGameMode::TryDedicatedServerLogin()` 接管。

##### `HandleMatchHasStarted()` / `HandleMatchHasEnded()`

> ⏱️ **引擎调用时机:** GameMode 的 MatchState 变化时：`InProgress` 时调用 `HandleMatchHasStarted`，`WaitingPostMatch` 时调用 `HandleMatchHasEnded`。
>
> **适合写的逻辑:** 记录比赛时间、统计数据、通知外部系统、触发奖励发放。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 11](17-Engine-Lifecycle-Reference.md#11-游戏会话生命周期)

当前仅调用 `Super`，作为预留钩子；Lyra 5.7 参考源码也是同样实现。不能把
这两个钩子单独当成完整比赛逻辑，但 `ALyraGameSession` 作为一个整体已实现
参考类的核心行为：关闭引擎默认自动登录，并把 Dedicated Server（专用服务器）
登录交给 `ALyraGameMode`。

---

### ULyraUserFacingExperienceDefinition [Runtime]

**当前状态：** **部分复刻**

**继承链:** `UObject → UDataAsset → UPrimaryDataAsset → ULyraUserFacingExperienceDefinition`

**UCLASS:** `UCLASS(BlueprintType)`

**职责:** 描述“玩家在前端看到的一局游戏/Playlist”。它不是实际玩法逻辑本身，而是把 UI 展示信息、地图、真正要加载的 `ULyraExperienceDefinition` 和 Session 创建参数打包到一个可由 AssetManager 扫描的数据资产中。

**核心属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `MapID` | `FPrimaryAssetId` (`AllowedTypes="Map"`) | 要打开的地图 Primary Asset |
| `ExperienceID` | `FPrimaryAssetId` (`AllowedTypes="LyraExperienceDefinition"`) | 进入地图后要加载的真实 Experience |
| `ExtraArgs` | `TMap<FString, FString>` | 附加 URL Options，会拼入 travel/session 参数 |
| `TileTitle` / `TileSubTitle` / `TileDescription` | `FText` | 前端列表展示文本 |
| `TileIcon` | `UTexture2D*` | 前端模式图标 |
| `LoadingScreenWidget` | `TSoftClassPtr<UUserWidget>` | 进入或离开该 Experience 时使用的加载界面 |
| `bIsDefaultExperience` | `bool` | 是否作为快速游玩/默认入口优先显示 |
| `bShowInFrontEnd` | `bool` | 是否显示在前端 Experience 列表中 |
| `bRecordReplay` | `bool` | 是否请求录制本局回放 |
| `MaxPlayerCount` | `int32` | Session 最大玩家数 |

**关键方法: `CreateHostingRequest(const UObject* WorldContextObject)`**
1. 从 `WorldContextObject` 找到 `UGameInstance`，优先通过 `UCommonSessionSubsystem::CreateOnlineHostSessionRequest()` 创建请求。
2. 如果 CommonSessionSubsystem 不可用，则创建一个基础 `UCommonSession_HostSessionRequest`，默认使用 Online 模式、Lobby、Presence。
3. 写入 `MapID`、`ModeNameForAdvertisement`、`MaxPlayerCount` 和 `ExtraArgs`。
4. 额外追加 `Experience=<ExperienceID.PrimaryAssetName>`，让后续 travel/加载流程知道要使用哪个真实 Experience。
5. 当 `bRecordReplay == true` 且 `ULyraReplaySubsystem::DoesPlatformSupportReplays()` 返回 true 时，追加 `DemoRec` 参数请求录制回放。

**与 `ALyraWorldSettings` 的区别:**

- `ALyraWorldSettings::DefaultGameplayExperience` 是关卡自身的默认 Experience，适合“直接打开这张地图时”的兜底配置。
- `ULyraUserFacingExperienceDefinition` 是前端/Playlist 入口，适合“玩家选择某个模式后创建 Session 并打开地图”的流程，可以覆盖地图、Experience、最大人数和回放参数。

---

### ALyraWorldSettings [Runtime + Editor-Only 部分]

**当前状态：** **部分复刻**

**继承链:** `UObject → AActor → AInfo → AWorldSettings → ALyraWorldSettings`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 每关卡级别的配置中心。关卡制作者在此配置该关卡的默认 Experience。

**属性:**

| 属性 | 类型 | 生命周期 | 说明 |
|------|------|---------|------|
| `DefaultGameplayExperience` | `TSoftClassPtr<ULyraExperienceDefinition>` | [Runtime] | 此关卡的默认 Experience |
| `ForceStandaloneNetMode` | `bool` | [Editor-Only] | 强制 PIE 为 Standalone 模式（用于前端/菜单地图） |

**关键方法:**

- `GetDefaultGameplayExperience()`: 返回 `DefaultGameplayExperience` 的 `FPrimaryAssetId`。若无效则记录错误。

**重写的生命周期函数:**

##### `CheckForErrors()`

> ⏱️ **引擎调用时机:** 编辑器 Map Check（地图检查）功能触发时。用户点击 Build → Map Check 或 PIE 启动前自动检查时调用。
>
> **适合写的逻辑:** 检查关卡中 Actor 配置是否正确、验证 PlayerStart 类型、检查缺失的必需配置、输出警告/错误。仅在编辑器中编译（`#if WITH_EDITOR`）。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 12](17-Engine-Lifecycle-Reference.md#12-aworldsettings)

[Editor-Only] 验证关卡中所有 PlayerStart 都是 `ALyraPlayerStart` 类型。

**ForceStandaloneNetMode 的工作原理:**
`ULyraEditorEngine::PreCreatePIEInstances()` 在执行 PIE 前检查此标志。当设为 true 时，强制 PIE 网络模式为 `PIE_Standalone`。这对于不需要网络模拟的前端/菜单关卡很有用。

---

## 核心数据流

```
ALyraGameMode (基础框架)
  ├── GameState = ALyraGameState
  │     ├── ULyraExperienceManagerComponent (Experience 状态机)
  │     ├── ULyraAbilitySystemComponent (比赛级 ASC)
  │     ├── ServerFPS / RecorderPlayerState 复制状态
  │     └── FLyraVerbMessage → GameplayMessageSubsystem 客户端广播
  ├── HandleMatchAssignmentIfNotExpectingOne()
  │     └── 选择 ExperienceId → SetCurrentExperience()
  ├── OnExperienceLoaded()
  │     └── RestartPlayer() 生成等待中的玩家 Pawn
  └── ChoosePlayerStart / FinishRestartPlayer
        └── 代理到 ULyraPlayerSpawningManagerComponent

ALyraWorldSettings (配置)
  └── DefaultGameplayExperience → 驱动 Experience 加载

ULyraUserFacingExperienceDefinition (前端/Playlist)
  ├── MapID → HostSessionRequest.MapID
  ├── ExperienceID → ExtraArgs["Experience"]
  └── bRecordReplay → ExtraArgs["DemoRec"] (平台支持时)

ALyraGameSession (会话)
  └── 在 ALyraGameMode 下管理比赛生命周期
```

主流程以服务器为起点：GameMode 同步选择 Experience ID（体验标识）后，
Experience Manager 异步加载资产和 GameFeature；`CurrentExperience`
复制到客户端后，各客户端独立执行自己的加载。只有 Loaded（已加载）委托
到达后，GameMode 才重启等待中的玩家。

失败边界：

- 找不到 Experience / PawnData 的关键路径仍包含 `check` 或错误日志，
  不是完整的可恢复流程。
- GameMode 与 PlayerState 都注册 Normal Priority（普通优先级）委托，
  没有 High Priority（高优先级）保证；代码依靠 PawnData 回退路径避免
  回调顺序成为硬性契约。
- Spawn 失败时 `FailedToRestartPlayer()` 可在条件仍成立时请求下一帧重试。

---

## 网络与权限

| 对象或状态 | 权威端 | 复制方式 | 客户端结果 |
|---|---|---|---|
| `ALyraGameMode` | Server Only | 不复制 | 客户端无法直接读取 GameMode |
| `CurrentExperience` | Server 设置 | GameState Component 属性复制 | 客户端 `OnRep` 后独立加载 Bundle / GameFeature |
| GameState `ServerFPS` | Server Tick 写入 | 普通属性复制 | 客户端只读服务器帧率 |
| `RecorderPlayerState` | Server / Replay 录制路径 | `COND_ReplayOnly` + RepNotify | Replay 客户端收到后广播变化 |
| 比赛级 ASC | Server Authority | ASC Mixed Replication Mode | 按 ASC 注册条件同步；GameState 没有普通玩家 Owning Connection，具体效果可见范围需验证 |
| `FLyraVerbMessage` | Server | Reliable 或 Unreliable NetMulticast | 客户端转入 Gameplay Message Subsystem |

持续状态应放在复制属性或 ASC 中；`FLyraVerbMessage` Multicast（多播远程
调用）表示一次性事件，Late Join（中途加入）客户端不会补收过去的消息。

---

## 资产、配置与模块依赖

| 来源 | 当前配置 | 作用与待确认边界 |
|---|---|---|
| `Config/DefaultEngine.ini` | `GlobalDefaultGameMode` 指向 `B_LyraGameMode` | 蓝图可覆盖 C++ 默认类 |
| `ALyraWorldSettings::DefaultGameplayExperience` | 地图级软类引用 | 直接打开地图时的 Experience 兜底 |
| `ULyraUserFacingExperienceDefinition` | Map ID、Experience ID、Extra Args、人数、Replay 开关 | 前端 / Playlist 创建 Session 请求 |
| AssetManager Primary Asset 扫描 | Map、Experience、UserFacingExperience 等类型 | 决定 Dedicated Server 能否按 ID 找到资产 |
| GameFeature / ModularGameplayActors | Experience Action 和组件注入 | 出生管理等组件不一定由 C++ 固定创建 |
| CommonSession / CommonUser | Host Session 与 Dedicated Server 登录 | 在线服务行为需运行环境验证 |

> 🧪 **待验证：**
> `B_LyraGameMode`、WorldSettings、UserFacingExperience 和目标 Experience
> 资产的最终字段值尚未逐项打开确认，不能仅凭 C++ 默认值断言运行时配置。

---

## 与 Lyra 的差异

### 差异：Pawn 生成已接入，Pawn 初始化仍断链

**当前项目：** 能等待 Experience、选择 PawnData / PawnClass、Deferred
Spawn 并代理出生点；`ULyraPawnExtensionComponent` 初始化仍是 TODO。

**Lyra：** Pawn Extension 把 PawnData、ASC、输入和初始化状态机连接起来。

**影响：** Pawn Actor 可以生成，但不能据此认定 Ability、Input 和
GameFeature 组件已经 Gameplay Ready（游戏玩法就绪）。

**状态：** **部分复刻**

### 对照：GameSession 已对齐，但职责本身很窄

**当前项目：** `HandleMatchHasStarted()` / `HandleMatchHasEnded()` 只调用
`Super`；自动登录实际由 GameMode 的 Dedicated Server 路径处理。

**Lyra：** Lyra 5.7 的 `ALyraGameSession` 代码与当前项目一致；更完整的
在线、Playlist 和 Dedicated Server 流程位于 GameMode、CommonSession
（通用会话）及相关资产中，而不是这个类本身。

**影响：** 当前类没有缺失的 Lyra 代码，但不能因此把它视为完整比赛状态机。

**状态：** **已复刻**

### 差异：Replay Session 参数与客户端录制执行者断开

**当前项目：** UserFacingExperience 可附加 `DemoRec`；PlayerController
客户端录制 API 缺失。

**Lyra：** 平台支持时，Session 参数与 Replay Subsystem 的录制能力共同工作。

**影响：** 服务器 Demo 参数和客户端自动录制是两条不同路径，前者存在不能
证明后者可用。

**状态：** **部分复刻**

---

## 已识别的 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 当前源码 TODO | 在 Deferred Spawn 完成前接入 Pawn Extension | `SpawnDefaultPawnAtTransform_Implementation()` 注释 | Pawn Extension / PawnData |
| 高 | 验证 TODO | 验证 GameMode 与 PlayerState Experience 回调顺序 | 两者均为 Normal Priority | Listen Server、Dedicated Server 日志 |
| 中 | 验证 TODO | 验证 Dedicated Server Playlist / Host Session 流程 | 依赖 CommonUser / CommonSession 与资产 | 在线服务或 Null Subsystem 配置 |
| 低 | 验证 TODO | 验证 `ProcessAutoLogin()` 与 Dedicated Server 登录链 | 当前实现与 Lyra 一致，但在线运行尚未验证 | CommonUser / CommonSession 与目标 Playlist |
| 中 | 验证 TODO | 验证 UserFacingExperience 的 `DemoRec` 行为 | C++ 只构造参数 | 平台 Replay Trait 与 Session 旅行 |
| 低 | 文档 TODO | 核对 `B_LyraGameMode` 和地图 WorldSettings 资产覆盖 | 二进制资产边界 | Unreal Editor 资产检查 |

---

## 快速回顾

- **一句话职责：** Game 框架在服务器选择本局 Experience，并通过 GameState
  把共享状态和加载入口带到客户端。
- **核心入口：** `InitGame()`、`InitGameState()`、
  `OnMatchAssignmentGiven()`、`OnExperienceLoaded()`。
- **核心状态：** GameState 上的 CurrentExperience、比赛级 ASC、
  ServerFPS 和 Recorder PlayerState。
- **网络位置：** GameMode 仅服务器；GameState 复制到客户端。
- **当前完成度：** **部分复刻**。
- **最重要的未完成项：** Pawn Extension 初始化和资产 / Dedicated Server
  运行验证。

## 复习要点

1. 为什么 GameMode 只负责权威规则，而共享状态放在 GameState？
2. Experience ID 从哪些来源按什么优先级选择？
3. 客户端为什么需要独立加载 CurrentExperience？
4. GameMode 和 PlayerState 的 Normal Priority 回调顺序能否作为契约？
5. PawnData 的三层回退是什么？
6. NetMulticast 消息为什么不能保存晚加入客户端需要的持续状态？
7. UserFacingExperience 与 WorldSettings 的职责区别是什么？

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — AssetManager 提供 DefaultPawnData 保底
- [07-Experience-Framework.md](07-Experience-Framework.md) — ExperienceManagerComponent 在 GameState 上运行
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerState 也使用 Modular 基类
- [09-GameplayTags-System.md](09-GameplayTags-System.md) — 回放支持通过 `Platform.Trait.ReplaySupport` 平台 Trait 判断
- [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md) — 展开 Experience 加载、复制与玩家生成时序
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — 跟踪 GameMode 选择的 Controller / Replay Controller 后续协作
