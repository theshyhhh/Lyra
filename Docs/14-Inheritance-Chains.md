# 14 - 继承链速查表

> 所有 Lyra 自定义类的完整继承链。标记 🧩 的类使用 Modular 基类，可通过 GameFeature 插件注入组件。

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
        ULyraGameInstance         ← 加密、用户初始化、会话旅行
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
      ULyraSettingsLocal          ← 每用户本地设置持久化
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
      ALyraGameSession            ← 禁用自动登录、比赛生命周期钩子
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
    ULocalPlayer
      UCommonLocalPlayer          ← CommonUI 插件
        ULyraLocalPlayer          ← 当前最小实现
  [注释掉] ILyraTeamAgentInterface
```

```
ALyraPlayerController [Runtime]
  AActor
    AController
      APlayerController
        ACommonPlayerController   ← CommonUI 插件
          ALyraPlayerController   ← 类型化访问器 GetLyraPlayerState()/GetLyraHUD()
            ALyraReplayPlayerController  ← 回放支持
  [注释掉] ILyraCameraAssistInterface, ILyraTeamAgentInterface
```

```
ALyraPlayerState [Runtime] 🧩
  AActor → AInfo
    APlayerState
      AModularPlayerState         ← GameFeature 可注入组件
        ALyraPlayerState          ← GetLyraPlayerController() 辅助方法
  [注释掉] IAbilitySystemInterface, ILyraTeamAgentInterface
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
        ULyraDeveloperSettings     ← 经验覆盖、BOT 计数、作弊、CVar 后备

ULyraPlatformEmulationSettings [Editor-Dev]
  UObject
    UDeveloperSettings
      UDeveloperSettingsBackedByCVars
        ULyraPlatformEmulationSettings  ← 平台特征模拟、设备配置模拟
```

---

## 非 UObject 类型

```
FLyraAssetManagerStartupJob       — 启动作业包装器（TFunction + 进度委托 + 权重）
FLyraBundles                      — 静态 Bundle 名称常量（Equipped）
FLyraCheatToRun                   — 作弊配置结构体（Phase + Cheat 字符串）
FLyraVerbMessage                  — GameplayTag 驱动的通用 gameplay event payload
FOnLyraGameModePlayerInitialized  — GameMode 玩家初始化完成委托（AGameModeBase*, AController*）
FOnRecorderPlayerStateChanged     — GameState 回放录制者 PlayerState 变化委托
ECheatExecutionTime               — 枚举（OnCheatManagerCreated, OnPlayerPawnPossession）
ELyraExperienceLoadState          — 枚举（Unloaded→Loaded→Deactivating 7 个状态）
ELyraPlayerStartLocationOccupancy — 枚举（Empty, Partial, Full）
LyraGameplayTags (namespace)      — 原生 GameplayTag 声明 + MovementModeTagMap
```
