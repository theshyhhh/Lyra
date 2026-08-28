# 15 - 数据流与生命周期

> 端到端数据流：从引擎启动到 Experience 加载完成的完整调用链。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 LocalPlayer（本地玩家）、Settings（设置）、GameInstance（游戏
> 实例）和 Experience（体验）改动。当前项目与 Lyra 参考项目均声明 UE 5.7，
> 直接调用者和网络分支按 Unreal Engine 5.7.4 源码核对。本文描述静态调用链，
> 不代表已经完成编译、PIE、登录、多人联网、地图切换或 Replay（回放）验证。

---

## 阅读范围与当前状态

**阅读目标：** 从进程启动、PIE、Experience 选择与加载、PlayerState
（玩家状态）复制和 Pawn（受控实体）生成出发，追到可观察的玩家控制、
UI 或网络状态；进入通用渲染、网络驱动和资产容器底层后停止。

| 主流程 | 当前状态 | 起点 | 本文终点 | 关键缺口 |
|---|---|---|---|---|
| 引擎与 GameInstance 启动 | **部分复刻** | 应用启动 | InitState 与 GameData 初始化 | GameplayCue Manager 为空 |
| Experience 加载 / 卸载 | **部分复刻** | GameMode 分配 Experience | Action 激活或反激活 | 失败处理与异步停用验证 |
| GameState / PlayerState 复制 | **部分复刻** | 服务器写入状态 | 客户端 RepNotify / 本地加载 | 多人和 Late Join 未验证 |
| 玩家生成与出生点 | **部分复刻** | Experience Loaded | Pawn 生成并完成 Restart | Pawn Extension 未接入 |
| PlayerController 协作 | **部分复刻** | Possess / PlayerState 到达 | ASC、Team、观战或隐藏状态更新 | GAS 输入、相机生产端、远程旋转 |
| LocalPlayer Team 转发 | **已复刻** | Controller 创建 / 切换或 Team 变化 | LocalPlayer Team Delegate | 静态主链与 Lyra 基本一致，尚未多人验证 |
| 登录后 Shared Settings（共享设置） | **结构占位** | CommonUser 用户初始化成功 | 项目异步包装函数返回 `false` | 未发起 UE 异步加载，缓存不被回调替换 |
| Experience 后设置重应用 | **结构占位** | Experience 三档完成委托结束 | 空 `OnExperienceLoaded()` | 调用已进入但没有设置副作用 |
| Replay 客户端录制 | **未复刻** | 录制资格检查 | 实际启动录制 | 资格恒 false，Subsystem API 缺失 |

---

## 1. 引擎启动序列

```
应用程序启动
  │
  └── ULyraGameEngine::Init(IEngineLoop*)
        └── Super::Init() (UGameEngine::Init)
              │
              └── ULyraAssetManager::StartInitialLoading()
                    ├── 记录启动时间戳
                    ├── DoAllStartupJobs()
                    │     ├── 作业 1: InitializeGameplayCueManager() [TODO: 空实现]
                    │     └── 作业 2: LoadGameDataOfClass<ULyraGameData>()
                    │           └── 同步加载 ULyraGameData 资产
                    │           └── 缓存到 GameDataMap
                    └── 记录完成时间 + 广播进度
              │
              └── ULyraGameInstance::Init()
                    ├── Super::Init() (UCommonGameInstance)
                    ├── 向 UGameFrameworkComponentManager 注册 InitState 状态转移:
                    │     InitState_Spawned → InitState_DataAvailable
                    │     → InitState_DataInitialized → InitState_GameplayReady
                    ├── 生成 DebugTestEncryptionKey (用于测试加密)
                    └── 绑定 OnPreClientTravelToSession 委托
```

---

## 2. 编辑器 PIE 启动序列

```
用户点击 PIE (Play In Editor)
  │
  ├── ULyraEditorEngine::PreCreatePIEInstances()
  │     ├── 从 WorldSettings 读取 ForceStandaloneNetMode
  │     │     └── 若 true → 强制 PIE 为 PIE_Standalone，弹出通知
  │     ├── ULyraDeveloperSettings::OnPlayInEditorStarted()
  │     │     ├── ApplySettings()
  │     │     └── 弹出作弊/调试功能激活提醒通知
  │     └── ULyraPlatformEmulationSettings::OnPlayInEditorStarted()
  │           ├── ApplySettings()
  │           │     └── ChangeActivePretendPlatform()
  │           └── 弹出平台模拟配置变更提醒
  │
  └── FLyraEditorModule (BeginPIE)
        └── ULyraExperienceManager::OnPlayInEditorBegun()
              └── 重置 GameFeaturePluginRequestCountMap (确保空状态)
```

---

## 3. Experience 加载序列（核心数据流）

这是 Lyra 最关键的执行流程。以下是从地图加载到 Experience 就绪的完整异步流水线：

```
【阶段 0: 触发】
  地图加载 → ALyraGameMode::InitGame()
    └── SetTimerForNextTick(HandleMatchAssignmentIfNotExpectingOne)

  ALyraGameMode::InitGameState()
    └── ExperienceManagerComponent.CallOrRegister_OnExperienceLoaded(OnExperienceLoaded)
          └── 注册到 Normal Priority（普通优先级），不是 High Priority（高优先级）

  下一帧:
    ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne()
      ├── URL ?Experience=
      ├── PIE DeveloperSettings::ExperienceOverride
      ├── CommandLine -Experience=
      ├── ALyraWorldSettings::DefaultGameplayExperience
      ├── Dedicated Server 登录/Host 流程
      └── 默认 LyraExperienceDefinition:B_LyraDefaultExperience

  ALyraGameMode::OnMatchAssignmentGiven(ExperienceId, Source)
    └── ULyraExperienceManagerComponent::SetCurrentExperience(FPrimaryAssetId)
      ├── AssetManager.GetPrimaryAssetPath() → FSoftObjectPath
      ├── TryLoad() → TSubclassOf<ULyraExperienceDefinition>
      ├── GetDefault<ULyraExperienceDefinition>(Class) → CDO
      ├── 断言: CurrentExperience == nullptr (不支持热切换)
      └── StartExperienceLoad()

【阶段 1: Loading】
  StartExperienceLoad():
    ├── 收集 BundleAssetList: Experience + 所有 ActionSet 的 PrimaryAssetId
    ├── 确定 BundlesToLoad:
    │     ├── FLyraBundles::Equipped (始终加载)
    │     ├── LoadStateClient (非 DedicatedServer)
    │     └── LoadStateServer (非客户端)
    ├── AssetManager.ChangeBundleStateForPrimaryAssets() (异步高优先级)
    ├── AssetManager.LoadAssetList() (RawAssetList，当前为空)
    ├── 创建 CombinedHandle (合并所有异步操作)
    ├── 绑定 OnExperienceLoadComplete 回调
    └── [可选] 预加载资产列表 (不阻塞，不绑定回调)

【阶段 2: LoadingGameFeatures】
  OnExperienceLoadComplete():
    ├── 从 Experience.GameFeaturesToEnable 收集插件名称 → 转 URL
    ├── 从所有 ActionSet.GameFeaturesToEnable 收集插件名称 → 转 URL
    ├── 去重 → GameFeaturePluginURLs
    ├── 若插件数 == 0 → 直接进入 OnExperienceFullLoadCompleted
    ├── 若插件数 > 0:
    │     └── 对每个插件 URL:
    │           ├── ULyraExperienceManager::NotifyOfPluginActivation() (引用计数 +1)
    │           └── UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin()
    │                 └── 异步回调: OnGameFeaturePluginLoadComplete()
    └── OnGameFeaturePluginLoadComplete():
          └── NumGameFeaturePluginsLoading--
          └── 全部加载完毕 → OnExperienceFullLoadCompleted()

【阶段 3: LoadingChaosTestingDelay (可选)】
  OnExperienceFullLoadCompleted():
    ├── 检查 CVars: lyra.chaos.ExperienceDelayLoad.MinSecs + .RandomSecs
    ├── 若有延迟 → SetTimer → 到期后重新调用本函数
    └── 若无延迟 → 继续

【阶段 4: ExecutingActions】
    ├── 创建 FGameFeatureActivatingContext (绑定到当前 World Context)
    ├── 对 Experience.Actions[] 中的每个 Action:
    │     ├── Action == nullptr → 跳过
    │     ├── Action->OnGameFeatureRegistering()
    │     ├── Action->OnGameFeatureLoading()
    │     └── Action->OnGameFeatureActivating(Context)
    ├── 对所有 ActionSet.Actions[] 中的每个 Action (同上)
    └── LoadState = Loaded

【阶段 5: Loaded (委托广播)】
    ├── OnExperienceLoaded_HighPriority.Broadcast(CurrentExperience) → Clear()
    ├── OnExperienceLoaded.Broadcast(CurrentExperience) → Clear()
    │     ├── ALyraGameMode::OnExperienceLoaded()
    │     │     └── RestartPlayer() 生成等待中的 PlayerController Pawn
    │     └── ALyraPlayerState::OnExperienceLoaded()
    │           └── 选择并写入 PawnData
    ├── OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience) → Clear()
    ├── [#if !UE_SERVER] ULyraSettingsLocal::OnExperienceLoaded()
    │     └── [当前函数体为空，不产生设置副作用]
    └── ShouldShowLoadingScreen() → false → 隐藏加载画面
```

### 蓝图等待 Experience Ready

```
蓝图调用 UAsyncAction_ExperienceReady::WaitForExperienceReady(WorldContextObject)
  ├── GEngine->GetWorldFromContextObject()
  ├── NewObject<UAsyncAction_ExperienceReady>()
  ├── 保存 TWeakObjectPtr<UWorld>
  └── RegisterWithGameInstance(World) 保持节点生命周期

Activate()
  ├── World 不存在 → SetReadyToDestroy()
  ├── World 已有 GameState
  │     └── Step2_ListenToExperienceLoading(GameState)
  └── World 尚无 GameState
        └── 绑定 World::GameStateSetEvent

Step2_ListenToExperienceLoading(GameState)
  ├── 查找 ULyraExperienceManagerComponent
  ├── IsExperienceLoaded() == true
  │     └── SetTimerForNextTick(Step4_BroadcastReady)
  └── IsExperienceLoaded() == false
        └── CallOrRegister_OnExperienceLoaded(Step3_HandleExperienceLoaded)

Step4_BroadcastReady()
  ├── OnReady.Broadcast()
  └── SetReadyToDestroy()
```

这条路径主要服务蓝图和 UI：它不负责选择或加载 Experience，只是等待 `ULyraExperienceManagerComponent` 的 Loaded 状态。已 Loaded 时仍延迟一帧广播，避免调用方写出依赖即时完成的时序假设。

---

## 4. Experience 卸载序列

```
GameState::EndPlay() → ULyraExperienceManagerComponent::EndPlay()

【阶段 1: 插件卸载】
  对 GameFeaturePluginURLs 中的每个插件:
    ├── ULyraExperienceManager::RequestToDeactivatePlugin(URL)
    │     └── 引用计数 -1
    │     └── 计数归零? → true (允许卸载) : false (保留)
    └── 若允许卸载 → UGameFeaturesSubsystem::DeactivateGameFeaturePlugin()

【阶段 2: Action 反激活 (仅当 LoadState == Loaded)】
  LoadState = Deactivating
  └── 创建 FGameFeatureDeactivatingContext (绑定 World Context)
  └── 对所有 Action (FILO 顺序):
        ├── Action->OnGameFeatureDeactivating(Context)
        │     └── [若异步] Action 向 Context 注册 pauser
        └── Action->OnGameFeatureUnregistering()
  └── Context.GetNumPausers() → NumExpectedPausers
  └── 若 NumExpectedPausers == NumObservedPausers:
        └── OnAllActionsDeactivated()
              ├── LoadState = Unloaded
              └── CurrentExperience = nullptr
```

> ⚠️ 异步反激活框架已搭建但未完全测试。触发异步反激活时会打印 Error 日志。

---

## 5. 网络复制

```
服务器:
  SetCurrentExperience(ExperienceId)
    └── CurrentExperience = CDO
          │
          │ (DOREPLIFETIME)
          ▼
客户端:
  OnRep_CurrentExperience()
    └── StartExperienceLoad()
          └── 客户端独立走完整加载流程（Bundle 加载 → 插件激活 → Action 执行）
```

`CurrentExperience` 是唯一被复制的属性。`LoadState`、`NumGameFeaturePluginsLoading` 等状态变量不复制 — 每个客户端独立管理自己的加载状态。

### GameState 复制与客户端消息广播

```
ALyraGameState::构造
  ├── CreateDefaultSubobject<ULyraAbilitySystemComponent>()
  │     ├── SetIsReplicated(true)
  │     └── SetReplicationMode(Mixed)
  └── CreateDefaultSubobject<ULyraExperienceManagerComponent>()

ALyraGameState::PostInitializeComponents()
  └── AbilitySystemComponent->InitAbilityActorInfo(this, this)

服务器 Tick
  └── ServerFPS = GAverageFPS
        └── DOREPLIFETIME(ServerFPS) → 客户端可读 GetServerFPS()

Replay 场景
  └── SetRecorderPlayerState(PlayerState)
        └── RecorderPlayerState (COND_ReplayOnly)
              └── OnRep_RecorderPlayerState()
                    └── OnRecorderPlayerStateChangedEvent.Broadcast()

服务器 gameplay 事件
  ├── MulticastMessageToClients(FLyraVerbMessage) [Unreliable]
  └── MulticastReliableMessageToClients(FLyraVerbMessage) [Reliable]
        └── 客户端 UGameplayMessageSubsystem::BroadcastMessage(Message.Verb, Message)
```

`FLyraVerbMessage` 的 `Verb` 既是事件语义也是消息通道。Instigator、Target、Tag 容器和 Magnitude 共同构成 payload，避免伤害、淘汰、助攻、UI 通知等系统互相直接依赖。

### PlayerState 复制、ASC 与队伍状态

```
ALyraPlayerState::构造
  ├── CreateDefaultSubobject<ULyraAbilitySystemComponent>()
  │     ├── SetIsReplicated(true)
  │     └── SetReplicationMode(Mixed)
  ├── MyPlayerConnectionType = Player
  ├── MyTeamID = NoTeam
  └── MySquadID = INDEX_NONE

ALyraPlayerState::PostInitializeComponents()
  ├── AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn())
  └── [服务器 GameWorld] ExperienceManager.CallOrRegister_OnExperienceLoaded(OnExperienceLoaded)

ALyraPlayerState::GetLifetimeReplicatedProps()
  ├── Push Model: PawnData, MyPlayerConnectionType, MyTeamID, MySquadID
  ├── FastArray: StatTags (FGameplayTagStackContainer)
  └── COND_SkipOwner: ReplicatedViewRotation

Team ID 改变
  ├── SetGenericTeamId(NewTeamID) [Authority]
  ├── MARK_PROPERTY_DIRTY_FROM_NAME(MyTeamID)
  ├── MyTeamID = NewTeamID
  └── ConditionalBroadcastTeamChanged()
        └── OnTeamChangedDelegate.Broadcast(Object, OldTeamIndex, NewTeamIndex)

客户端收到 MyTeamID
  └── OnRep_MyTeamID(OldTeamID)
        └── ConditionalBroadcastTeamChanged()
```

`FGameplayTagStackContainer` 的复制数组只负责网络增量同步，查询仍走本地 `TagToCountMap`。客户端收到 Add/Change/Remove 后通过 FastArray 回调维护 Map，因此 `HasStatTag()` 与 `GetStatTagStackCount()` 不需要每次遍历数组。

---

## 6. 加载进度与加载画面

```
ILoadingProcessInterface::ShouldShowLoadingScreen()
  └── LoadState != Loaded → true → CommonLoadingScreen 显示 W_LoadingScreen_Host
  └── LoadState == Loaded → false → 隐藏加载画面

ULyraAssetManager::DoAllStartupJobs()
  └── 进度权重系统:
        ├── 有界进度 (BoundProgress): 使用 FScopedSlowTask
        └── 无界进度 (UnboundProgress): 通过委托回调报告
  └── DedicatedServer 跳过进度 UI
```

`DefaultGame.ini` 中的 `[/Script/CommonLoadingScreen.CommonLoadingScreenSettings]` 指定默认加载界面：

```ini
LoadingScreenWidget = /Game/UI/Foundation/LoadingScreen/W_LoadingScreen_Host.W_LoadingScreen_Host_C
ForceTickLoadingScreenEvenInEditor = False
```

---

## 7. UI 根布局可见性同步

```
GameInstance 创建
  └── ULyraUIManagerSubsystem::Initialize()
        ├── UGameUIManagerSubsystem 读取 DefaultUIPolicyClass
        │     └── /Game/UI/B_LyraUIPolicy.B_LyraUIPolicy_C
        └── FTSTicker::AddTicker(Tick)

每帧 Tick
  └── SyncRootLayoutVisibilityToShowHUD()
        ├── GetCurrentUIPolicy()
        ├── 遍历 GameInstance->GetLocalPlayers()
        ├── LocalPlayer → PlayerController → HUD
        ├── HUD && !HUD->bShowHUD
        │     └── RootLayout.Visibility = Collapsed
        └── 否则
              └── RootLayout.Visibility = SelfHitTestInvisible

GameInstance 销毁
  └── ULyraUIManagerSubsystem::Deinitialize()
        └── FTSTicker::RemoveTicker(TickHandle)
```

这保证了传统 `AHUD::bShowHUD` 开关可以控制 CommonUI 的 `UPrimaryGameLayout`，不会出现 HUD 被隐藏但 CommonUI 根布局仍显示的状态。

---

## 8. 面向用户 Experience 到 Session 请求

`ULyraUserFacingExperienceDefinition` 是前端/Playlist 层入口。它把玩家看到的卡片转换为 CommonSession 可执行的 Host 请求。

```
前端选择 Playlist / UserFacingExperience
  │
  └── ULyraUserFacingExperienceDefinition::CreateHostingRequest(WorldContextObject)
        ├── WorldContextObject → UWorld → UGameInstance
        ├── 优先使用 UCommonSessionSubsystem::CreateOnlineHostSessionRequest()
        ├── 若 Subsystem 不可用 → NewObject<UCommonSession_HostSessionRequest>()
        │     ├── OnlineMode = Online
        │     ├── bUseLobbies = true
        │     └── bUsePresence = !IsRunningDedicatedServer()
        ├── HostSessionRequest.MapID = MapID
        ├── HostSessionRequest.ModeNameForAdvertisement = 自身 PrimaryAssetName
        ├── HostSessionRequest.MaxPlayerCount = MaxPlayerCount
        ├── HostSessionRequest.ExtraArgs = ExtraArgs
        ├── ExtraArgs["Experience"] = ExperienceID.PrimaryAssetName
        └── 若 bRecordReplay && 平台拥有 Platform.Trait.ReplaySupport:
              └── ExtraArgs["DemoRec"] = ""
```

后续 Session/Travel 使用 `MapID` 打开地图，并通过 URL 参数中的 `Experience` 选择真实的 `ULyraExperienceDefinition`。这条路径和 `ALyraWorldSettings::DefaultGameplayExperience` 是互补关系：前者来自前端选择，后者是直接打开地图时的默认兜底。

---

## 9. 玩家出生点缓存、选择与 Claim

`ULyraPlayerSpawningManagerComponent` 负责出生点缓存和选择逻辑，`ALyraPlayerStart` 负责判断一个点是否能放下 Pawn 以及短期占用。

```
ULyraPlayerSpawningManagerComponent::InitializeComponent()
  ├── 绑定 FWorldDelegates::LevelAddedToWorld
  ├── 绑定 UWorld::AddOnActorSpawnedHandler()
  └── 遍历当前 World 中所有 ALyraPlayerStart → CachedPlayerStarts

关卡流式加载 / 动态生成 Actor
  ├── OnLevelAdded() → 新增 ALyraPlayerStart 加入缓存
  └── HandleOnActorSpawned() → 动态生成的 ALyraPlayerStart 加入缓存

ALyraGameMode::ChoosePlayerStart_Implementation(Controller)
  └── ULyraPlayerSpawningManagerComponent::ChoosePlayerStart(Controller)
        ├── [Editor] APlayerStartPIE 优先支持 Play From Here
        ├── 清理失效弱引用
        ├── Spectator → 随机出生点，不 Claim
        ├── OnChoosePlayerStart() 子类扩展
        ├── 默认选择:
        │     ├── Empty 出生点随机优先
        │     └── Partial 出生点随机次选
        └── ALyraPlayerStart::TryClaim(Controller)
              └── 定时 CheckUnclaimed()，Pawn 离开/点位空出后释放 Claim
```

**当前接入状态:** `ALyraGameMode` 已经代理出生点选择、重生许可和重生完成钩子到该组件。组件侧 `ControllerCanRestart()` 仍是最小实现，当前直接允许重生。

---

## 10. Experience 驱动的玩家生成流程

最新流程把“玩家登录”“PlayerState 数据初始化”和“Pawn 生成”拆开：玩家可以先完成 GameMode 级初始化，但只有 Experience 加载完成后才真正生成 Pawn；PlayerState 也会在 Experience 就绪后写入复制的 PawnData。

```
玩家进入 / PostLogin
  └── ALyraGameMode::GenericPlayerInitialization(NewPlayer)
        ├── Super::GenericPlayerInitialization()
        └── OnGameModePlayerInitialized.Broadcast(GameMode, Controller)

ALyraGameMode::HandleStartingNewPlayer_Implementation(PlayerController)
  ├── IsExperienceLoaded() == false → 暂不 RestartPlayer
  └── IsExperienceLoaded() == true  → Super::HandleStartingNewPlayer_Implementation()

Experience Loaded
  ├── [NormalPriority] ALyraGameMode::OnExperienceLoaded(CurrentExperience)
  │     └── 遍历 PlayerController:
  │           └── 没有 Pawn 且 PlayerCanRestart() → RestartPlayer()
  └── [NormalPriority] ALyraPlayerState::OnExperienceLoaded(CurrentExperience)
        └── ALyraGameMode::GetPawnDataForController(GetOwningController())
              └── SetPawnData(NewPawnData) → 复制 PawnData + 发送 LyraAbilitiesReady

RestartPlayer()
  ├── ChoosePlayerStart_Implementation()
  │     └── ULyraPlayerSpawningManagerComponent::ChoosePlayerStart()
  ├── GetDefaultPawnClassForController_Implementation()
  │     └── GetPawnDataForController()
  │           ├── PlayerState::GetPawnData<ULyraPawnData>() (若已设置；否则回退)
  │           ├── CurrentExperience->DefaultPawnData
  │           └── ULyraAssetManager::GetDefaultPawnData()
  ├── SpawnDefaultPawnAtTransform_Implementation()
  │     ├── SpawnActor(PawnClass, bDeferConstruction = true)
  │     ├── [TODO] ULyraPawnExtensionComponent 初始化
  │     └── FinishSpawning()
  └── FinishRestartPlayer()
        ├── ULyraPlayerSpawningManagerComponent::FinishRestartPlayer()
        └── Super::FinishRestartPlayer()
```

如果生成失败，`FailedToRestartPlayer()` 会在仍存在 PawnClass 且 Controller 仍可重启时，调用 `RequestPlayerRestartNextFrame()` 下一帧重试。

`ALyraGameMode::OnExperienceLoaded()` 与 `ALyraPlayerState::OnExperienceLoaded()` 都注册到同一个 Normal Priority（普通优先级）多播委托（Multicast Delegate）。当前初始化路径中 GameMode 通常在 `InitGameState()` 注册，而 PlayerState 在自己的 `PostInitializeComponents()` 注册，但代码没有用优先级 API 固化二者相对次序，因此不应把“GameMode 必定先执行”写成契约。无论回调先后，`GetPawnDataForController()` 都会在 PlayerState 尚无 PawnData 时回退到 Experience 或 AssetManager 默认值；仍需通过日志或断点验证目标运行模式下的实际顺序。

---

## 11. 提交 `19e6961` 的 PlayerController 协作链

### 11.1 创建与控制权

```text
ALyraGameMode 构造
  |-- PlayerControllerClass = ALyraPlayerController
  `-- ReplaySpectatorPlayerControllerClass = ALyraReplayPlayerController

UE APlayerController::PostInitializeComponents()
  |-- [Server] InitPlayerState()
  |-- SpawnPlayerCameraManager()
  |     `-- ALyraPlayerCameraManager [结构占位]
  `-- AddCheats()
        `-- [Non-Shipping 功能路径] ULyraCheatManager [结构占位]

[Server Authority]
AController::Possess(Pawn)
  `-- ALyraPlayerController::OnPossess()
        |-- Super 建立 Controller / Pawn 关系
        `-- 清除 Status_AutoRunning

AController::UnPossess()
  `-- ALyraPlayerController::OnUnPossess()
        |-- ASC.Avatar == 旧 Pawn 时先清 Avatar
        `-- Super 清除 Pawn 关系
```

这条链在 Game Thread（游戏线程）同步执行。引擎 `Possess()` 会拒绝非
Authority（网络权威）调用。当前 Controller 只清旧 Avatar，不负责为新 Pawn
设置 Avatar；后者仍依赖未完成的 Pawn Extension（Pawn 扩展）链。

### 11.2 PlayerState 延迟到达与 GAS 补偿

```text
[Owning Client]
PlayerState 引用复制到达
  v
AController::OnRep_PlayerState()
  |-- PlayerState::ClientInitialize(Controller)
  v
ALyraPlayerController::OnRep_PlayerState()
  |-- 解绑旧 Team Delegate
  |-- 绑定新 Team Delegate
  |-- ASC::RefreshAbilityActorInfo()
  `-- [当前注释] TryActivateAbilitiesOnSpawn()
```

`RefreshAbilityActorInfo()` 只重建 ASC 对现有 OwnerActor / AvatarActor /
PlayerController 的缓存，不授予或激活 Ability（能力）。因此该路径状态为
**部分复刻**，不能按中文注释直接写成“补激活完成”。

### 11.3 本地输入、观战与相机隐藏

```text
[本地 PlayerController::PlayerTick]
  |-- Status_AutoRunning 存在
  |     `-- Pawn::AddMovementInput()
  |-- 本地或 Authority 视角
  |     `-- PlayerState::SetReplicatedViewRotation()
  `-- 观战远程 Pawn
        `-- 读取目标 PlayerState::ReplicatedViewRotation
              `-- 写入本地 TargetViewRotation 并平滑

[Lyra 中存在，当前缺失]
Camera Mode 穿透检测
  v
[当前已实现]
OnCameraPenetratingTarget()
  v
下一次 ULocalPlayer::CalcSceneView()
  v
UpdateHiddenComponents()
  `-- View Target 图元加入本帧隐藏集合
```

`PostProcessInput()` 中 `ProcessAbilityInput()` 仍被注释，所以 Auto-run 的
Loose Gameplay Tag（松散游戏玩法标签）不能代表完整 GAS Prediction
（GAS 预测）已经接入。

> ⚠️ **推断：**
> UE 5.7.4 服务器远程 Autonomous Proxy（自治代理）Controller 的
> `TickActor()` 分支绕过 `PlayerTick()`，而当前没有第二处
> `SetReplicatedViewRotation()` 调用。Dedicated Server 上远程玩家的权威
> 观战旋转可能缺少持续写入来源；需要运行验证。

### 11.4 Replay 播放与录制

```text
[Replay 播放，当前部分复刻]
Replay Controller Tick
  |-- Recorder PlayerState 失效或变化
  |-- 重新绑定 GameState / PlayerState
  `-- Pawn 到达时 SetViewTarget(NewPawn)

[客户端录制，当前未复刻]
ShouldRecordClientReplay()
  `-- Local Settings 判断被注释 → 恒 false

TryToRecordClientReplay() [仅当派生资格检查返回 true]
  |-- 设置 RecorderPlayerState
  |-- [当前注释] ReplaySubsystem::RecordClientReplay()
  `-- 潜在返回 true，但没有真实录制
```

Replay 播放跟随和客户端录制是两条独立链，不能用一条的完成度推断另一条。

---

## 12. 当前工作区的 LocalPlayer、Team 与 Settings 主链

### 12.1 LocalPlayer 创建、Controller 更换与 Team 转发

```text
Config/DefaultEngine.ini
  `-- LocalPlayerClassName = ULyraLocalPlayer
        `-- UObject 创建链
              `-- ULyraLocalPlayer::PostInitProperties()
                    `-- 监听 ULyraSettingsLocal::OnAudioOutputDeviceChanged

ULocalPlayer 创建或更换 PlayerController
  |-- ULyraLocalPlayer::SpawnPlayActor()
  |-- ULyraLocalPlayer::SwitchController()
  `-- APlayerController::SetPlayer()
        `-- ULyraLocalPlayer::InitOnlineSession()
              `-- ULyraLocalPlayer::OnPlayerControllerChanged()
                    |-- 从旧 Controller 移除 Team Delegate
                    |-- 向新 Controller 注册 Team Delegate
                    `-- 广播 LocalPlayer 旧 / 新 Team

[Server Authority]
ALyraPlayerState::SetGenericTeamId()
  `-- MyTeamID 属性复制
        `-- [Owning Client] ALyraPlayerState::OnRep_MyTeamID()
              `-- PlayerState Team Delegate
                    `-- ALyraPlayerController::OnPlayerStateChangedTeam()
                          `-- Controller Team Delegate
                                `-- ULyraLocalPlayer::OnControllerChangedTeam()
                                      `-- LocalPlayer Team Delegate
```

Team 数据的网络权威仍在 PlayerState；Controller 和 LocalPlayer 都只是观察 /
转发层。LocalPlayer 不复制，但可把已经复制到本机的 Team 状态暴露给 CommonUI
或其他仅本机系统。Controller 更换时的解绑 / 重绑在 Game Thread（游戏线程）
同步完成；多人和分屏场景仍需运行验证。

### 12.2 用户登录后的 Shared Settings 异步链

**起点：** `UCommonGameInstance::Init()` 绑定 CommonUser 用户初始化完成事件。

**当前终点：** `ULyraSettingsShared::AsyncLoadOrCreateSettings()` 直接返回
`false`。设计终点是 LocalPlayer 缓存被磁盘对象替换，但当前不可达。

```text
UCommonUserSubsystem 用户初始化成功
  `-- 下一 Tick 广播 OnUserInitializeComplete
        `-- ULyraGameInstance::HandlerUserInitialized()
              |-- Super::HandlerUserInitialized()
              |-- 检查 bSuccess 与 UserInfo
              `-- GetLocalPlayerByIndex(UserInfo->LocalPlayerIndex)
                    |-- Dedicated Server 无 LocalPlayer → 跳过
                    `-- ULyraLocalPlayer::LoadSharedSettingsFromDisk()
                          |-- 相同 Unique Net ID 已缓存且非强制 → 返回
                          `-- ensure(ULyraSettingsShared::AsyncLoadOrCreateSettings())
                                `-- [当前直接 false]
                                      |-- ensure 失败
                                      |-- 不调用 UE 异步 SaveGame API
                                      |-- 不执行设置 Apply（应用）
                                      `-- 不进入 OnSharedSettingsLoaded()
```

> 🧩 **引擎机制：** UE 5.7.4 的
> `ULocalPlayerSaveGame::AsyncLoadOrCreateSaveGameForLocalPlayer()` 在参数有效时
> 安排异步任务，并在加载失败或 Slot（存档槽）不存在时创建新对象；父类使用
> 弱 LocalPlayer 捕获避免异步回调悬挂。当前项目尚未调用该 API。

### 12.3 首次同步访问、对象所有权与 Experience 设置钩子

```text
ULyraLocalPlayer::GetSharedSettings()
  |-- SharedSettings 已缓存 → 返回
  |-- PLATFORM_DESKTOP
  |     `-- LoadOrCreateSettings() [同步，可能阻塞 Game Thread]
  |           `-- Slot = SharedGameSettings
  `-- 其他平台
        `-- CreateTemporarySettings()
              `-- 等登录后真实对象替换 [当前异步端点未实现]

ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()
  `-- 三档 Experience Loaded 委托全部广播并清空
        `-- [#if !UE_SERVER] ULyraSettingsLocal::OnExperienceLoaded()
              `-- [当前空实现]
```

`ULyraSettingsShared` 由 SaveGame（存档）系统以 Transient Package（瞬态包）
为 Outer 创建；其父类保存 Owning LocalPlayer，LocalPlayer 再以
`UPROPERTY(Transient)` 强引用当前缓存。异步成功替换后，旧对象在没有其他强
引用时由 GC（垃圾回收）回收。当前 Apply / Save（应用 / 保存）和设置字段均
未实现，详细差异见 [08-UI-Framework.md](08-UI-Framework.md)。

---

## 已识别的流程验证 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 验证 TODO | 记录 GameMode / PlayerState Experience Loaded 回调顺序 | 两者均使用 Normal Priority | Server 日志与两个玩家 |
| 高 | 验证 TODO | 验证 PlayerState / ASC / Controller 不同复制顺序 | `OnRep_PlayerState()` 为补偿路径 | Dedicated Server、远程客户端 |
| 高 | 验证 TODO | 验证远程玩家观战旋转权威写入 | UE `TickActor()` 分支疑点 | 观战与 Late Join 场景 |
| 高 | 当前源码 TODO | 实现 Shared Settings 异步请求和完成回调 | 登录链在包装函数直接返回 `false` | `ULocalPlayerSaveGame` API 与 Apply 策略 |
| 中 | 验证 TODO | 验证 Controller 切换 / 重新登录时 Team 委托和设置缓存 | 静态解绑与缓存条件已存在 | 多人 PIE、账号切换与分屏 |
| 中 | Lyra 对比 TODO | 实现 Experience 完成后的本地设置重应用 | 调用点已启用，目标函数为空 | Device Profile 与非分辨率设置 |
| 中 | 验证 TODO | 验证 Spawn Manager 在目标 Experience 中注入和清理 | C++ 不固定创建该组件 | Experience / GameFeature 资产 |
| 中 | 验证 TODO | 验证 Experience 异步反激活 | 现有代码含 Pauser（暂停器）跟踪与错误日志 | 地图切换、多 PIE |
| 中 | 验证 TODO | 验证 Replay Seek / Checkpoint 后 View Target | 播放跟随只有静态证据 | 可用 Replay 文件 |
| 低 | 文档 TODO | 补充蓝图 / 资产覆盖后的最终运行流程 | 当前二进制资产未逐项检查 | Unreal Editor 资产审计 |

## 复习要点

1. 哪些初始化发生在 World 创建前，哪些依赖 GameState？
2. Experience 从服务器复制什么，客户端又自行执行什么？
3. GameMode 与 PlayerState 的 Loaded 回调为何没有固定优先级契约？
4. PawnData 写入、PawnClass 选择和 Pawn Extension 初始化分别在哪一步？
5. PlayerState 晚到时 `OnRep_PlayerState()` 补了什么、没补什么？
6. `COND_SkipOwner` 如何影响观战旋转？
7. Replay 播放和录制为什么必须分开画调用链？
8. 登录后 Shared Settings 链目前在哪一步停止，哪些回调不会发生？
9. Team 从服务器 PlayerState 到本机 LocalPlayer 经过哪些 Delegate？

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — 引擎启动序列
- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 状态机详细实现
- [12-Editor-Module.md](12-Editor-Module.md) — PIE 启动序列
- [04-Game-Framework.md](04-Game-Framework.md) — UserFacingExperience 与 Session 请求
- [05-Player-Framework.md](05-Player-Framework.md) — 出生点缓存、选择和 Claim
- [06-Character-Framework.md](06-Character-Framework.md) — PawnData、PawnClass 和 PawnExtensionComponent
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — 将流程断点按源码、Lyra 对比和验证来源分类
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — 深入提交 `19e6961` 中的 Controller、Camera、Replay 与网络分支
- [08-UI-Framework.md](08-UI-Framework.md) — Local / Shared Settings 的字段、所有权、引擎 SaveGame 机制和 Lyra 差异
