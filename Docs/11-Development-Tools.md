# 11 - 开发工具

> 编辑器开发设置，以及当前未提交代码新增的 Cheat Manager（作弊管理器）和 External RPC（外部远程过程调用）自动化骨架。它们分布在 `Development/`、`Player/` 与 `Tests/`。

---

## 框架概述

`Development/` 下的类为开发者提供编辑器和 PIE（Play In Editor，编辑器内运行）环境中的调试与平台模拟能力；`ULyraCheatManager` 和 `ULyraGameplayRpcRegistrationComponent` 则分别预留游戏内作弊命令与 HTTP 自动化入口。后两条路径在发布构建（Shipping Build）中受编译开关限制。

**设计意图:**
- 加速 PIE 迭代（跳过非必要内容加载、自动执行作弊命令）
- 在 PC 上模拟主机/移动端平台行为
- 通过开发者设置面板集中管理调试选项
- 在受控非发布环境提供游戏内 Cheat（作弊）入口
- 为 Gauntlet / 外部测试工具预留 HTTP 自动化端点，并在发布构建关闭功能路径

---

## 类列表

| 类/类型 | 父类 | 生命周期 | 职责 |
|---------|------|---------|------|
| `ULyraDeveloperSettings` | `UDeveloperSettingsBackedByCVars` | [Editor-Dev] | 开发者调试设置（经验覆盖、BOT、作弊、地图） |
| `ULyraPlatformEmulationSettings` | `UDeveloperSettingsBackedByCVars` | [Editor-Dev] | PIE 平台模拟（平台特征、设备配置） |
| `ECheatExecutionTime` | (UENUM) | [Editor-Dev] | 作弊执行时机枚举 |
| `FLyraCheatToRun` | (USTRUCT) | [Editor-Dev] | 作弊配置结构体 |
| `ULyraCheatManager` | `UCheatManager` | [Non-Shipping Runtime] | 当前继承引擎作弊能力的最小子类 |
| `ULyraGameplayRpcRegistrationComponent` | `UExternalRpcRegistrationComponent` | [Non-Shipping Runtime] | 当前未注册端点的 HTTP 自动化骨架 |

---

## 逐类详解

### ULyraDeveloperSettings [Editor-Dev]

**继承链:** `UObject → UDeveloperSettings → UDeveloperSettingsBackedByCVars → ULyraDeveloperSettings`

**UCLASS:** `UCLASS(Config=EditorPerProjectUserSettings, MinimalAPI)`

**配置:** `Config=EditorPerProjectUserSettings` 意味着每个开发者拥有自己的独立配置，不会被提交到版本控制。

**编辑器分类:** 出现在 Project Settings → Lyra 面板下。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `ExperienceOverride` | `FPrimaryAssetId` | PIE 时覆盖使用哪个 Experience（未设置则使用 WorldSettings 中的默认） |
| `bOverrideBotCount` | `bool` | 是否覆盖机器人数量的切换开关 |
| `OverrideNumPlayerBotsToSpawn` | `int32` | 覆盖的机器人数量（仅当 bOverrideBotCount 为 true 时生效） |
| `bAllowPlayerBotsToAttack` | `bool` | 是否允许机器人攻击 |
| `bTestFullGameFlowInPIE` | `bool` | 是否执行完整游戏流程（跳过等待玩家等阶段） |
| `bShouldAlwaysPlayForceFeedback` | `bool` | 是否始终播放力反馈（CVar: `LyraPC.ShouldAlwaysPlayForceFeedback`） |
| `bSkipLoadingCosmeticBackgroundsInPIE` | `bool` | 是否跳过装饰性背景加载以加速 PIE 迭代 |
| `CheatsToRun` | `TArray<FLyraCheatToRun>` | PIE 启动时自动执行的作弊命令列表 |
| `LogGameplayMessages` | `bool` | 是否记录 GameplayMessageSubsystem 的所有消息广播（CVar: `GameplayMessageSubsystem.LogMessages`） |
| `CommonEditorMaps` | `TArray<FSoftObjectPath>` | [Editor-Only] 编辑器工具栏可快速访问的常用地图列表 |

**编辑器方法 (WITH_EDITOR):**
- `OnPlayInEditorStarted()`: 由 `ULyraEditorEngine::PreCreatePIEInstances()` 调用。若 `ExperienceOverride` 有效，显示 2 秒通知"Developer Settings Override\nExperience {Name}"。
- `ApplySettings()`: 预留的设置应用钩子，当前为空实现。
**重写的 UObject 生命周期:**

##### `PostInitProperties()`
> ⏱️ **引擎调用时机:** 对象构造完成、属性从 CDO 或 Config 文件加载后。在所有属性初始化完成之后。⚠️ 必须调用 `Super::PostInitProperties()`。
>
> **适合写的逻辑:** 基于已加载的属性做二次初始化、验证配置一致性。与构造函数不同——此时所有 `Config`/`EditDefaultsOnly` 属性已加载。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 6](17-Engine-Lifecycle-Reference.md#6-uobject-生命周期)

**当前行为:** 调用 `FModuleManager::Get().LoadModuleChecked("GameplayMessageRuntime")` 确保 CVar 绑定的模块已加载（`LogGameplayMessages` 属性绑定到该模块中的 CVar），再调用 `Super::PostInitProperties()` 和 `ApplySettings()`。

##### `PostEditChangeProperty(FPropertyChangedEvent&)`
> ⏱️ **引擎调用时机:** 用户在编辑器 Details 面板修改属性后。`PropertyChangedEvent` 包含哪个属性被修改、修改类型。仅在编辑器触发，PIE 中修改不触发。
>
> **适合写的逻辑:** 属性联动（改 A 自动调 B）、实时验证用户输入、预览效果。通过 `PropertyChangedEvent.MemberProperty` 判断变更的属性。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 6](17-Engine-Lifecycle-Reference.md#6-uobject-生命周期)

[Editor-Only] 调用 `ApplySettings()` 应用变更。

##### `PostReloadConfig(FProperty*)`
> ⏱️ **引擎调用时机:** 配置文件被热重载后（如修改 .ini 后编辑器自动重载，或调用 `ReloadConfig()`）。多个属性变更时，每个都触发一次。
>
> **适合写的逻辑:** 在 Config 热重载后重新应用设置、响应外部配置变更。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 6](17-Engine-Lifecycle-Reference.md#6-uobject-生命周期)

[Editor-Only] 调用 `ApplySettings()`。

---

### ECheatExecutionTime [Editor-Dev]

**UENUM:** `UENUM()`

| 值 | 说明 |
|-----|------|
| `OnCheatManagerCreated` | 当作弊管理器被创建时执行 |
| `OnPlayerPawnPossession` | 当玩家 Pawn 被控制时执行 |

---

### FLyraCheatToRun [Editor-Dev]

**USTRUCT:** `USTRUCT()`

| 属性 | 类型 | 默认值 | 说明 |
|------|------|-------|------|
| `Phase` | `ECheatExecutionTime` | `OnPlayerPawnPossession` | 何时执行此作弊 |
| `Cheat` | `FString` | — | 要执行的控制台命令字符串 |

---

### ULyraCheatManager [Non-Shipping Runtime]

**继承链:** `UObject → UCheatManager → ULyraCheatManager`

**UCLASS:** `UCLASS(config=Game, Within=PlayerController, MinimalAPI)`

**当前行为:**

- `USING_CHEAT_MANAGER` 定义为 `1 && !UE_BUILD_SHIPPING`，所以当前项目只在非发布构建中选择该类。
- `ALyraPlayerController` 构造时把 `CheatClass` 指向 `ULyraCheatManager`；UE 5.7.4 的 `APlayerController::PostInitializeComponents()` 随后调用 `AddCheats()`，当前重写会在宏启用时强制请求创建 Cheat Manager。
- 当前类体没有 Lyra 专属函数，但会继承 `UCheatManager` 的基础行为。Lyra 原项目中的 GAS（Gameplay Ability System，游戏玩法能力系统）调试、伤害/治疗、标签、无敌、固定相机和 Debug Camera（调试相机）等扩展命令尚未复刻。

**服务器作弊 RPC（网络远程过程调用）:**

`ServerCheat()` 与 `ServerCheatAll()` 是 Reliable Server RPC（可靠服务器 RPC），都声明了 `WithValidation`，但当前 `_Validate()` 无条件返回 `true`。执行体仍要求 `CheatManager` 存在，并受非发布构建宏保护；`ServerCheatAll()` 会遍历世界中的全部 `ALyraPlayerController` 执行命令。

> 风险：编译开关阻止了发布构建暴露这条路径，但非发布联网环境没有额外的命令白名单或权限校验。测试 Dedicated Server（专用服务器）或共享开发服务器时，应把它视为高权限调试入口。

---

### ULyraGameplayRpcRegistrationComponent [Non-Shipping Runtime]

**继承链:** `UObject → UExternalRpcRegistrationComponent → ULyraGameplayRpcRegistrationComponent`

**当前行为:** 当前类只有 `GENERATED_BODY()`，没有单例、对象创建、路由注册、注销或请求处理函数。`ALyraPlayerController::BeginPlay()` 在 `WITH_RPC_REGISTRY` 开启时调用 `FHttpServerModule::StartAllListeners()` 并解析 `rpcport`，但 `GetInstance()`、`RegisterAlwaysOnHttpCallbacks()` 和 `RegisterInMatchHttpCallbacks()` 全部被注释。

**UE 5.7.4 底层含义:**

- `StartAllListeners()` 只启用并启动模块中已经存在的 HTTP Listener（监听器），不会自动创建 Router（路由器）或 Endpoint（端点）。
- 当前 `BeginPlay()` 在检查 `rpcport` 之前无条件调用它，所以非发布构建即使没有 `-rpcport=`，也会启动其他系统已经向全局 HTTP Server 模块创建的 Listener；多个 PlayerController 还可能重复调用这个全局入口。
- 父类 `RegisterAlwaysOnHttpCallbacks()` 只广播 RPC 列表发生变化，本身不添加路由。
- 因为当前子类从未实例化、父子注册函数也未被调用，所以即使传入 `-rpcport=`，也没有静态证据表明存在可访问的 Lyra HTTP 端点。

**Lyra 原设计:** 原项目子类负责创建并 Root（根引用）单例、解析 JSON、注册/注销路由，并提供作弊命令、单次开火、玩家状态和前端/比赛阶段等自动化接口。当前只完成了类型与构建依赖骨架。

> 名称区分：这里的 External RPC 通过 HTTP 服务自动化工具；`ServerCheat()` 是 Unreal 网络复制系统的 Server RPC（服务器远程过程调用）。二者共享“RPC”名称，但不走同一套传输、权限或生命周期。

---

### ULyraPlatformEmulationSettings [Editor-Dev]

**继承链:** `UObject → UDeveloperSettings → UDeveloperSettingsBackedByCVars → ULyraPlatformEmulationSettings`

**UCLASS:** `UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)`

**职责:** 在开发 PC 上模拟不同平台（Xbox、PS5、移动端）的行为，用于测试 UI 可见性、性能选项和帧率策略，无需目标硬件。

**配置:** 与 `ULyraDeveloperSettings` 一样使用 `EditorPerProjectUserSettings`，平台模拟设置属于每个开发者自己的编辑器配置，不应作为项目默认运行时配置提交。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `AdditionalPlatformTraitsToEnable` | `FGameplayTagContainer` | 在 PIE 中额外启用的平台特征 |
| `AdditionalPlatformTraitsToSuppress` | `FGameplayTagContainer` | 在 PIE 中额外屏蔽的平台特征 |
| `PretendPlatform` | `FName` | 模拟的平台名称 |
| `PretendBaseDeviceProfile` | `FName` | 模拟的基础设备配置文件 |
| `bApplyFrameRateSettingsInPIE` | `bool` | 是否在 PIE 中应用帧率限制（CVar: `Lyra.Settings.ApplyFrameRateSettingsInPIE`） |
| `bApplyFrontEndPerformanceOptionsInPIE` | `bool` | 是否在 PIE 中应用前端性能选项（CVar: `Lyra.Settings.ApplyFrontEndPerformanceOptionsInPIE`） |
| `bApplyDeviceProfilesInPIE` | `bool` | 是否在 PIE 中应用模拟平台的设备配置（CVar: `Lyra.Settings.ApplyDeviceProfilesInPIE`） |

**注意事项（来自源文件注释）:**
- `bApplyFrameRateSettingsInPIE`: 帧率限制是引擎范围的，可能与其他 PIE 窗口冲突。建议同时禁用编辑器偏好中的 "Use Less CPU when in Background"。
- `bApplyFrontEndPerformanceOptionsInPIE` (默认 false): 前端性能选项是全局的。若一个 PIE 窗口在前端界面而另一个在游戏内界面，会互相覆盖。默认禁用以避免冲突。

**编辑器方法 (WITH_EDITOR):**
- `OnPlayInEditorStarted()`: 由 `ULyraEditorEngine::PreCreatePIEInstances()` 调用，弹出配置变更提醒。
- `ApplySettings()` / `ChangeActivePretendPlatform()`: 应用平台模拟设置。

---

## 触发流程

```
PIE 启动
  └── ULyraEditorEngine::PreCreatePIEInstances()
        ├── ULyraDeveloperSettings::OnPlayInEditorStarted()
        │     ├── ApplySettings()
        │     ├── 弹出作弊/调试功能激活提醒
        │     └── 检查 ExperienceOverride
        └── ULyraPlatformEmulationSettings::OnPlayInEditorStarted()
              └── ApplySettings() → ChangeActivePretendPlatform()

非发布运行时
  ├── APlayerController::PostInitializeComponents()
  │     └── ALyraPlayerController::AddCheats()
  │           └── 创建 ULyraCheatManager（若引擎允许作弊）
  └── ALyraPlayerController::BeginPlay()
        ├── FHttpServerModule::StartAllListeners()
        └── 解析 rpcport，但当前不创建注册对象、不绑定 HTTP 路由
```

---

## 关联框架

- [12-Editor-Module.md](12-Editor-Module.md) — ULyraEditorEngine 在 PreCreatePIEInstances 中调用这些设置
- [07-Experience-Framework.md](07-Experience-Framework.md) — ExperienceOverride 影响加载哪个 Experience
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — Cheat、External RPC 与 PlayerController 生命周期对照
