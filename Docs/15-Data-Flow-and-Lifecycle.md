# 15 - 数据流与生命周期

> 端到端数据流：从引擎启动到 Experience 加载完成的完整调用链。

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
    └── ExperienceManagerComponent.CallOrRegister_OnExperienceLoaded_HighPriority(OnExperienceLoaded)

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
    │     └── ALyraGameMode::OnExperienceLoaded()
    │           └── RestartPlayer() 生成等待中的 PlayerController Pawn
    ├── OnExperienceLoaded.Broadcast(CurrentExperience) → Clear()
    ├── OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience) → Clear()
    └── [注释掉] ULyraSettingsLocal::OnExperienceLoaded() (应用设置)
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

最新流程把“玩家登录”和“Pawn 生成”拆开：玩家可以先完成 GameMode 级初始化，但只有 Experience 加载完成后才真正生成 Pawn。

```
玩家进入 / PostLogin
  └── ALyraGameMode::GenericPlayerInitialization(NewPlayer)
        ├── Super::GenericPlayerInitialization()
        └── OnGameModePlayerInitialized.Broadcast(GameMode, Controller)

ALyraGameMode::HandleStartingNewPlayer_Implementation(PlayerController)
  ├── IsExperienceLoaded() == false → 暂不 RestartPlayer
  └── IsExperienceLoaded() == true  → Super::HandleStartingNewPlayer_Implementation()

Experience Loaded
  └── ALyraGameMode::OnExperienceLoaded(CurrentExperience)
        └── 遍历 PlayerController:
              └── 没有 Pawn 且 PlayerCanRestart() → RestartPlayer()

RestartPlayer()
  ├── ChoosePlayerStart_Implementation()
  │     └── ULyraPlayerSpawningManagerComponent::ChoosePlayerStart()
  ├── GetDefaultPawnClassForController_Implementation()
  │     └── GetPawnDataForController()
  │           ├── PlayerState::GetPawnData<ULyraPawnData>() (当前占位返回 nullptr)
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

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — 引擎启动序列
- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 状态机详细实现
- [12-Editor-Module.md](12-Editor-Module.md) — PIE 启动序列
- [04-Game-Framework.md](04-Game-Framework.md) — UserFacingExperience 与 Session 请求
- [05-Player-Framework.md](05-Player-Framework.md) — 出生点缓存、选择和 Claim
- [06-Character-Framework.md](06-Character-Framework.md) — PawnData、PawnClass 和 PawnExtensionComponent
