# 04 - Game 框架

> 游戏模式、游戏状态、游戏会话和世界设置。这些类构成了 Lyra 的"比赛"层。

---

## 框架概述

Game 框架承载比赛级别的逻辑和配置。Lyra 的关键设计决策是使用 `AGameModeBase` 而非 `AGameMode`（无 MatchState 状态机），并通过 Modular 基类启用 GameFeature 插件的组件注入。

**设计意图:**
- GameMode 负责 Experience 分配、玩家进入时机和 Pawn 生成管线，实际玩法规则仍由 Experience 的 GameFeatureAction 注入
- GameState 作为 Experience 加载状态机、比赛级 AbilitySystemComponent、客户端 gameplay message 广播的宿主
- WorldSettings 承载每关卡级别的 Experience 选择
- UserFacingExperience 承载前端/Playlist 入口，把玩家看到的游戏模式转换为可创建的 Session 请求

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ALyraGameMode` | `AModularGameModeBase` | [Runtime] 🧩 | Experience 分配、玩家初始化、Pawn 生成和重生代理 |
| `ALyraGameState` | `AModularGameStateBase`, `IAbilitySystemInterface` | [Runtime] 🧩 | 承载 ExperienceManagerComponent、比赛级 ASC、ServerFPS 和回放录制者状态 |
| `ALyraGameSession` | `AGameSession` | [Runtime] | 会话管理与比赛生命周期钩子 |
| `ALyraWorldSettings` | `AWorldSettings` | [Runtime + Editor-Only 部分] | 每关卡 Experience 配置 |
| `ULyraUserFacingExperienceDefinition` | `UPrimaryDataAsset` | [Runtime] | 前端可见的 Playlist/游戏入口，创建 Host Session 请求 |

> 🧩 = 使用 Modular 基类，GameFeature 可注入组件

---

## 逐类详解

### ALyraGameMode [Runtime] 🧩

**继承链:** `AActor → AInfo → AGameModeBase → AModularGameModeBase → ALyraGameMode`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "..."))`

**职责:** 服务器权威的游戏模式入口。负责选择并启动 Experience、控制玩家何时生成 Pawn、根据 PawnData 决定 PawnClass，并把出生/重生细节转发给组件化系统。

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
- `InitGameState()` 获取 `ULyraExperienceManagerComponent`，注册 `OnExperienceLoaded()` 为 HighPriority 回调。
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

**继承链:** `AActor → AInfo → AGameStateBase → AModularGameStateBase → ALyraGameState`，并实现 `IAbilitySystemInterface`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 比赛级共享状态。关键载体包括 `ULyraExperienceManagerComponent`、`ULyraAbilitySystemComponent`、服务器 FPS 复制、Replay 录制者 PlayerState，以及服务器到客户端的通用 gameplay message 广播。

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

**继承链:** `UObject → AInfo → AActor → AGameSession → ALyraGameSession`

**UCLASS:** `UCLASS(Config = Game)`

**职责:** 会话管理。

**重写的生命周期函数:**

##### `ProcessAutoLogin()`
> ⏱️ **引擎调用时机:** GameMode 初始化后，引擎尝试为本地玩家自动登录时。在 `HandleMatchIsWaitingToStart` 附近触发。
>
> **适合写的逻辑:** 返回 false 禁用引擎默认自动登录，实现自定义玩家加入逻辑。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 11](17-Engine-Lifecycle-Reference.md#11-游戏会话生命周期)

Lyra 返回 `true` 表示已处理自动登录（防止引擎再次执行默认流程），实际登录逻辑在 `LyraGameMode::TryDedicatedServerLogin` 中处理。

##### `HandleMatchHasStarted()` / `HandleMatchHasEnded()`
> ⏱️ **引擎调用时机:** GameMode 的 MatchState 变化时：`InProgress` 时调用 `HandleMatchHasStarted`，`WaitingPostMatch` 时调用 `HandleMatchHasEnded`。
>
> **适合写的逻辑:** 记录比赛时间、统计数据、通知外部系统、触发奖励发放。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 11](17-Engine-Lifecycle-Reference.md#11-游戏会话生命周期)

当前仅调用 Super，作为预留钩子。

---

### ULyraUserFacingExperienceDefinition [Runtime]

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

**继承链:** `UObject → AInfo → AActor → AWorldSettings → ALyraWorldSettings`

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

## 框架内部关系

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

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — AssetManager 提供 DefaultPawnData 保底
- [07-Experience-Framework.md](07-Experience-Framework.md) — ExperienceManagerComponent 在 GameState 上运行
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerState 也使用 Modular 基类
- [09-GameplayTags-System.md](09-GameplayTags-System.md) — 回放支持通过 `Platform.Trait.ReplaySupport` 平台 Trait 判断
