# 14 - 继承链速查表

> 所有 Lyra 自定义类的完整继承链。标记 🧩 的类使用 Modular 基类，可通过 GameFeature 插件注入组件。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 LocalPlayer（本地玩家）与 Settings（设置）改动。引擎父类按
> Unreal Engine 5.7.4 核对。继承关系只说明“是什么类型”，不说明 Lyra
> 专属行为是否已经实现。

> ⚠️ **注意：**
> 空子类仍继承父类能力，但按实现状态应标为**结构占位**，不能仅凭继承链
> 标记为“已复刻”。

---

## System 框架

```
ULyraGameEngine [Runtime]
  UObject
    UEngine
      UGameEngine
        ULyraGameEngine           ← 自定义 Init(IEngineLoop*)，目前为透传
```

```
ULyraGameInstance [Runtime]
  UObject
    UGameInstance
      UCommonGameInstance         ← CommonGame 插件
        ULyraGameInstance         ← 加密、用户初始化、共享设置入口、会话旅行
```

```
ULyraReplaySubsystem [Runtime]
  UObject
    USubsystem
      UGameInstanceSubsystem
        ULyraReplaySubsystem      ← 平台 Trait 驱动的回放能力判断
```

```
ULyraAssetManager [Runtime]
  UObject
    UAssetManager
      ULyraAssetManager           ← 启动作业队列、GameData/PawnData 类型化加载
```

```
ULyraGameData [Runtime]
  UObject
    UDataAsset
      UPrimaryDataAsset
        ULyraGameData             ← 全局 GE 引用（DamageSetByCaller、HealSetByCaller、DynamicTag）
```

```
ULyraSettingsLocal [Runtime]
  UObject
    UGameUserSettings
      ULyraSettingsLocal          ← [结构占位] 机器 / 进程级设置入口与事件钩子
```

```
ULyraSettingsShared [Runtime]
  UObject
    USaveGame
      ULocalPlayerSaveGame
        ULyraSettingsShared       ← [部分复刻] 每 LocalPlayer 存档对象与同步加载外壳
```

---

## Game 框架

```
ALyraGameMode [Runtime] 🧩
  AActor → AInfo
    AGameModeBase                  ← 轻量级，无 MatchState
      AModularGameModeBase         ← ModularGameplayActors 插件 → GameFeature 可注入组件
        ALyraGameMode              ← Experience 分配、Pawn 生成、出生/重生代理
```

```
ALyraGameState [Runtime] 🧩
  AActor → AInfo
    AGameStateBase
      AModularGameStateBase       ← GameFeature 可注入组件
        ALyraGameState            ← ExperienceManager、比赛级 ASC、复制状态与消息广播宿主
  实现: IAbilitySystemInterface
```

```
ALyraGameSession [Runtime]
  UObject → AActor → AInfo
    AGameSession
      ALyraGameSession            ← [已复刻] 禁用默认自动登录、保留比赛生命周期钩子
```

```
ALyraWorldSettings [Runtime + Editor-Only 部分]
  UObject → AActor → AInfo
    AWorldSettings
      ALyraWorldSettings          ← DefaultGameplayExperience、ForceStandaloneNetMode
```

---

## Player 框架

```
ULyraLocalPlayer [Runtime]
  UObject
    UPlayer
      ULocalPlayer
        UCommonLocalPlayer        ← CommonUI 插件
          ULyraLocalPlayer        ← [部分复刻] Team、设置与音频设备本地桥梁
  实现: ILyraTeamAgentInterface
```

```
ALyraPlayerController [Runtime]
  AActor
    AController
      APlayerController
        ACommonPlayerController   ← CommonUI 插件
          ALyraPlayerController   ← [部分复刻] PlayerState/队伍/相机辅助/自动奔跑协调
            ALyraReplayPlayerController  ← [部分复刻] 回放录制者 Pawn 跟随与检查点恢复
  实现: ILyraCameraAssistInterface, ILyraTeamAgentInterface
```

```
Camera [Runtime]
  UObject
    UInterface
      ULyraCameraAssistInterface  ← Unreal Reflection（虚幻反射）接口类型
  ILyraCameraAssistInterface      ← C++ 相机穿透协作协议

  AActor
    APlayerCameraManager
      ALyraPlayerCameraManager    ← [结构占位] 当前只继承引擎默认相机行为
```

```
ALyraPlayerState [Runtime] 🧩
  AActor → AInfo
    APlayerState
      AModularPlayerState         ← GameFeature 可注入组件
        ALyraPlayerState          ← [部分复刻] 玩家级 ASC、PawnData、Team/Squad、StatTag 复制状态
  实现: IAbilitySystemInterface, ILyraTeamAgentInterface
```

```
ALyraPlayerStart [Runtime]
  AActor
    ANavigationObjectBase
      APlayerStart
        ALyraPlayerStart          ← 出生点占用检测、Claim、标签容器
```

```
ULyraPlayerSpawningManagerComponent [Runtime]
  UObject
    UActorComponent
      UGameStateComponent
        ULyraPlayerSpawningManagerComponent  ← 出生点缓存、选择、重生扩展点
```

---

## Teams 框架

```
ULyraTeamAgentInterface [Runtime]
  UObject
    UInterface
      UGenericTeamAgentInterface  ← AIModule 通用队伍接口
        ULyraTeamAgentInterface   ← Lyra 队伍变更委托入口
```

```
ILyraTeamAgentInterface [Runtime]
  IGenericTeamAgentInterface       ← SetGenericTeamId()/GetGenericTeamId()
    ILyraTeamAgentInterface        ← GetOnTeamIndexChangedDelegate() + 条件广播辅助函数
```

---

## Character 框架

```
ALyraCharacter [Runtime] 🧩
  AActor
    APawn
      ACharacter
        AModularCharacter         ← GameFeature 可注入组件
          ALyraCharacter          ← 类型化访问器
            ALyraCharacterWithAbilities [Blueprintable]  ← GAS 就绪子类
  [注释掉] IAbilitySystemInterface, IGameplayCueInterface, IGameplayTagAssetInterface, ILyraTeamAgentInterface
```

```
ULyraPawnData [Runtime]
  UObject
    UDataAsset
      UPrimaryDataAsset
        ULyraPawnData              ← PawnClass (TSubclassOf<APawn>)
```

```
ULyraPawnExtensionComponent [Runtime]
  UObject
    UActorComponent
      UPawnComponent
        ULyraPawnExtensionComponent  ← Pawn 初始化/扩展占位组件
```

---

## AbilitySystem 框架

```
ULyraAbilitySystemComponent [Runtime]
  UObject
    UActorComponent
      UAbilitySystemComponent
        ULyraAbilitySystemComponent  ← Lyra 类型化 ASC；当前为最小构造函数封装
```

---

## Experience 框架

```
ULyraExperienceDefinition [Runtime + Editor-Only 验证]
  UObject
    UDataAsset
      UPrimaryDataAsset
        ULyraExperienceDefinition  ← GameFeaturesToEnable[]、Actions[]、DefaultPawnData、ActionSets[]
```

```
ULyraUserFacingExperienceDefinition [Runtime]
  UObject
    UDataAsset
      UPrimaryDataAsset
        ULyraUserFacingExperienceDefinition  ← 前端 Playlist、地图、Experience、Session 请求参数
```

```
ULyraExperienceActionSet [Runtime + Editor-Only 验证]
  UObject
    UDataAsset
      UPrimaryDataAsset
        ULyraExperienceActionSet   ← 可复用的 Actions[] + GameFeaturesToEnable[] 打包
```

```
ULyraExperienceManagerComponent [Runtime，复制]
  UActorComponent
    UGameStateComponent
      ULyraExperienceManagerComponent  ← Experience 加载状态机
  实现: ILoadingProcessInterface       ← ShouldShowLoadingScreen()
```

```
ULyraExperienceManager [Runtime，编辑器逻辑]
  UObject
    USubsystem
      UEngineSubsystem
        ULyraExperienceManager     ← GameFeature 插件引用计数（多 PIE 仲裁）
```

```
UAsyncAction_ExperienceReady [Runtime，蓝图异步节点]
  UObject
    UBlueprintAsyncActionBase
      UAsyncAction_ExperienceReady ← 等待 Experience Loaded 后广播 OnReady
```

---

## UI 框架

```
ALyraHUD [Runtime]
  AActor
    AHUD
      ALyraHUD                     ← GetDebugActorList() 重写
```

```
ULyraGameViewportClient [Runtime]
  UObject
    UGameViewportClient
      UCommonGameViewportClient    ← CommonUI 插件
        ULyraGameViewportClient    ← 自定义 Init()
```

```
ULyraUIManagerSubsystem [Runtime]
  UObject
    USubsystem
      UGameInstanceSubsystem
        UGameUIManagerSubsystem
          ULyraUIManagerSubsystem  ← 默认 UI Policy + RootLayout/HUD 可见性同步
```

---

## Messages 框架

```
ULyraVerbMessageHelpers [Runtime]
  UObject
    UBlueprintFunctionLibrary
      ULyraVerbMessageHelpers     ← PlayerState/Controller 解析 + VerbMessage/Cue 参数互转
```

---

## 编辑器模块

```
ULyraEditorEngine [Editor-Only]
  UObject
    UEngine
      UEditorEngine
        UUnrealEdEngine
          ULyraEditorEngine        ← PreCreatePIEInstances()、FirstTickSetup()
```

```
FLyraEditorModule [Editor-Only]
  FDefaultGameModuleImpl           ← 绑定 OnBeginPIE/OnEndPIE 委托
```

---

## 开发工具

```
ULyraDeveloperSettings [Editor-Dev]
  UObject
    UDeveloperSettings
      UDeveloperSettingsBackedByCVars
        ULyraDeveloperSettings     ← Experience（体验）覆盖、Bot 计数、作弊、CVar 后备

ULyraPlatformEmulationSettings [Editor-Dev]
  UObject
    UDeveloperSettings
      UDeveloperSettingsBackedByCVars
        ULyraPlatformEmulationSettings  ← 平台特征模拟、设备配置模拟

ULyraCheatManager [Non-Shipping Runtime]
  UObject
    UCheatManager
      ULyraCheatManager           ← [结构占位] 当前继承引擎基础 Cheat 行为

ULyraGameplayRpcRegistrationComponent [Non-Shipping Runtime]
  UObject
    UExternalRpcRegistrationComponent
      ULyraGameplayRpcRegistrationComponent  ← [结构占位] 当前无 External RPC 路由
```

---

## 非 UObject 类型

```
FLyraAssetManagerStartupJob       — 启动作业包装器（TFunction + 进度委托 + 权重）
FLyraBundles                      — 静态 Bundle 名称常量（Equipped）
FLyraCheatToRun                   — 作弊配置结构体（Phase + Cheat 字符串）
FGameplayTagStack                 — 单个 GameplayTag + StackCount，支持 FastArray 复制项
FGameplayTagStackContainer        — GameplayTag Stack 的 FastArray 容器 + 查询 Map
FLyraVerbMessage                  — GameplayTag 驱动的通用 gameplay event payload
FExperienceReadyAsyncDelegate     — Experience Ready 蓝图异步节点的 OnReady 动态多播委托
FOnLyraGameModePlayerInitialized  — GameMode 玩家初始化完成委托（AGameModeBase*, AController*）
FOnLyraTeamIndexChangedDelegate   — Team ID 变化动态多播委托（Object, OldTeamID, NewTeamID）
ULyraSettingsLocal::FAudioDeviceChanged — 音频输出设备变化原生 Event（DeviceId）
ULyraSettingsShared::FOnSettingsLoadedEvent — 共享设置加载完成单播 Delegate（Settings）
ULyraSettingsShared::FOnSettingChangedEvent — 共享设置变化原生 Event（当前无广播者）
FOnRecorderPlayerStateChanged     — GameState 回放录制者 PlayerState 变化委托
ECheatExecutionTime               — 枚举（OnCheatManagerCreated, OnPlayerPawnPossession）
ELyraExperienceLoadState          — 枚举（Unloaded→Loaded→Deactivating 7 个状态）
ELyraPlayerConnectionType         — 枚举（Player, LiveSpectator, ReplaySpectator, InactivePlayer）
ELyraPlayerStartLocationOccupancy — 枚举（Empty, Partial, Full）
ILyraTeamAgentInterface           — Lyra C++ Team Agent 接口（非 UObject 接口体）
ILyraCameraAssistInterface        — Lyra C++ Camera Assist 接口（非 UObject 接口体）
LyraGameplayTags (namespace)      — 原生 GameplayTag 声明 + MovementModeTagMap
```

---

## 复习要点

1. `UInterface` 反射类型和对应 C++ 接口体为什么会同时出现？
2. Modular 基类改变了哪些组件注入能力，而不直接实现哪些业务逻辑？
3. PlayerState、PlayerController 和 PlayerCameraManager 分别继承哪条引擎链？
4. 为什么空的 `ALyraPlayerCameraManager` 仍能运行基础相机，却只能标记为结构占位？
5. `Within=PlayerController` 对 `ULyraCheatManager` 的 Outer 约束是什么？
6. `ULyraSettingsLocal` 与 `ULyraSettingsShared` 为什么分别继承
   `UGameUserSettings` 和 `ULocalPlayerSaveGame`？

## 关联框架

- [05-Player-Framework.md](05-Player-Framework.md) — 解释 Player 继承链背后的所有权、网络位置和实际职责
- [11-Development-Tools.md](11-Development-Tools.md) — 说明 Cheat 与 External RPC 类型的构建和功能边界
- [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md) — 从父类继续查看引擎自动调用的生命周期函数
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — 验证提交 `19e6961` 新增类型是否真正接入调用链
- [08-UI-Framework.md](08-UI-Framework.md) — 解释 Local / Shared Settings 的所有权、加载方式和当前停止边界
