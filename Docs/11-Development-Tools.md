# 11 - 开发工具

> 开发工具框架包含编辑器设置、PIE（Play In Editor，编辑器内运行）平台
> 模拟、Cheat Manager（作弊管理器）和 External RPC（外部远程过程调用）。
> 最新提交只为后两者补齐类型或入口，不能把它们视为完整 Lyra 工具链。

> **核对基线：** 当前项目最新提交 `19e6961`；当前项目与 Lyra 参考项目
> 均声明 UE 5.7，HTTP、Cheat 和生命周期机制按 UE 5.7.4 源码核对。
> 文档本次复核日期为 2026-07-13，尚未运行非发布服务器或 HTTP 测试。

---

## 框架概述

**问题：** PIE 调试、平台模拟、游戏内 Cheat 和外部 HTTP 自动化属于不同
生命周期与权限边界；若只按“开发工具”统称，容易误判 Shipping（发布）
构建是否包含功能路径，也容易把 Unreal Server RPC（服务器远程调用）和
HTTP RPC 混为一谈。

**当前项目的解决方案：** `Development/` 下的 Developer Settings
（开发者设置）管理 PIE 覆盖和平台模拟；PlayerController 在非发布功能路径
中选择 `ULyraCheatManager` 并实现两个 Server RPC；Tests 目录中保留
`ULyraGameplayRpcRegistrationComponent` 类型，但没有 HTTP 路由。

**设计意图:**

- 加速 PIE 迭代（跳过非必要内容加载、自动执行作弊命令）
- 在 PC 上模拟主机/移动端平台行为
- 通过开发者设置面板集中管理调试选项

**当前完成度：** PIE 设置与平台模拟有实际行为；Cheat Manager 和 External
RPC 类属于**结构占位**。Server Cheat RPC 已有执行体，但非发布联网环境的
命令范围仍需验证。

---

## 当前复刻状态

| 模块 | 当前状态 | 当前实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| Developer Settings（开发者设置） | **部分复刻** | Experience 覆盖、Bot、PIE Cheat、力反馈和消息日志设置存在 | Lyra 还与更多完整系统联动 | 部分配置只保留字段或依赖未完成系统 |
| Platform Emulation（平台模拟） | **部分复刻** | 平台 Trait、Device Profile（设备配置）和 PIE 性能开关已接入 | Lyra 在完整设置/UI 环境中使用 | 多 PIE 的全局设置冲突仍需人工控制 |
| Cheat Manager 扩展 | **结构占位** | 空 `UCheatManager` 子类，由 Controller 在非发布功能路径中选择 | Lyra 有 GAS、伤害、标签、相机和无敌命令 | 只有引擎基础 Cheat 能力 |
| Server Cheat RPC | **已复刻** | 两个 Reliable Server RPC 及执行体存在 | Lyra 同名入口 | `WithValidation` 恒 true，需受控环境 |
| External RPC | **结构占位** | 类型、模块依赖、构建宏和 Listener 启动入口存在 | Lyra 有单例、JSON、路由与注销 | 当前没有 Lyra HTTP 端点 |

---

## 类列表

| 类/类型 | 父类或接口 | 生命周期 | 网络位置 | 当前状态 | 职责 |
|---|---|---|---|---|---|
| `ULyraDeveloperSettings` | `UDeveloperSettingsBackedByCVars` | Editor-Dev | 本地编辑器配置，不复制 | **部分复刻** | 管理 Experience、Bot、Cheat、力反馈和地图设置 |
| `ULyraPlatformEmulationSettings` | `UDeveloperSettingsBackedByCVars` | Editor-Dev | 本地编辑器配置，不复制 | **部分复刻** | 模拟平台 Trait 与 Device Profile |
| `ECheatExecutionTime` | `UENUM` | Editor-Dev | 不复制 | **已复刻** | 区分 Cheat Manager 创建与 Possess 时机 |
| `FLyraCheatToRun` | `USTRUCT` | Editor-Dev | Config 数据，不复制 | **已复刻** | 保存 Cheat 阶段与命令字符串 |
| `ULyraCheatManager` | `UCheatManager` | Non-Shipping Runtime 功能路径 | 各 Controller 本地持有 | **结构占位** | 继承引擎基础 Cheat 能力 |
| `ULyraGameplayRpcRegistrationComponent` | `UExternalRpcRegistrationComponent` | Non-Shipping Runtime 功能路径 | 计划服务外部 HTTP，当前未实例化 | **结构占位** | 保留 External RPC 类型边界 |

---

## 逐类详解

### ULyraDeveloperSettings [Editor-Dev]

**当前状态：** **部分复刻**

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

**当前状态：** **已复刻**

**UENUM:** `UENUM()`

| 值 | 说明 |
|-----|------|
| `OnCheatManagerCreated` | 当作弊管理器被创建时执行 |
| `OnPlayerPawnPossession` | 当玩家 Pawn 被控制时执行 |

---

### FLyraCheatToRun [Editor-Dev]

**当前状态：** **已复刻**

**USTRUCT:** `USTRUCT()`

| 属性 | 类型 | 默认值 | 说明 |
|------|------|-------|------|
| `Phase` | `ECheatExecutionTime` | `OnPlayerPawnPossession` | 何时执行此作弊 |
| `Cheat` | `FString` | — | 要执行的控制台命令字符串 |

---

### ULyraCheatManager [Non-Shipping Runtime]

**当前状态：** **结构占位**

**继承链:** `UObject → UCheatManager → ULyraCheatManager`

**UCLASS:** `UCLASS(config=Game, Within=PlayerController, MinimalAPI)`

**当前行为:**

- `USING_CHEAT_MANAGER` 定义为 `1 && !UE_BUILD_SHIPPING`，所以当前项目只在非发布构建中选择该类。
- `ALyraPlayerController` 构造时把 `CheatClass` 指向 `ULyraCheatManager`；UE 5.7.4 的 `APlayerController::PostInitializeComponents()` 随后调用 `AddCheats()`，当前重写会在宏启用时强制请求创建 Cheat Manager。
- 当前类体没有 Lyra 专属函数，但会继承 `UCheatManager` 的基础行为。Lyra 原项目中的 GAS（Gameplay Ability System，游戏玩法能力系统）调试、伤害/治疗、标签、无敌、固定相机和 Debug Camera（调试相机）等扩展命令尚未复刻。

**创建位置与所有权：** UE 5.7.4
`APlayerController::PostInitializeComponents()` 调用 `AddCheats()`；
允许或强制创建时，`NewObject<UCheatManager>(PlayerController, CheatClass)`
让 PlayerController 成为 Outer（外部对象），父类 `CheatManager` 属性持有
该对象并由 GC（垃圾回收）管理。

**服务器作弊 RPC（网络远程过程调用）:**

`ServerCheat()` 与 `ServerCheatAll()` 是 Reliable Server RPC（可靠服务器
远程调用），都声明 `WithValidation`，但当前 `_Validate()` 无条件返回
true。执行体要求 `CheatManager` 存在，并受非发布功能宏保护；
`ServerCheatAll()` 会遍历 World 中全部 `ALyraPlayerController` 执行命令。

> 风险：Shipping 中不会选择该 Cheat Manager，两个命令执行体也成为空路径，
> 但 UCLASS / UFUNCTION 声明本身仍可参与编译。非发布联网环境没有额外的
> 命令白名单或权限校验；测试 Dedicated Server（专用服务器）或共享开发
> 服务器时，应把它视为高权限调试入口。

---

### ULyraGameplayRpcRegistrationComponent [Non-Shipping Runtime]

**当前状态：** **结构占位**

**继承链:** `UObject → UExternalRpcRegistrationComponent → ULyraGameplayRpcRegistrationComponent`

**当前行为:** 当前类只有 `GENERATED_BODY()`，没有单例、对象创建、路由
注册、注销或请求处理函数。`ALyraPlayerController::BeginPlay()` 在
`WITH_RPC_REGISTRY` 开启时调用
`FHttpServerModule::StartAllListeners()` 并解析 `rpcport`，但
`GetInstance()`、`RegisterAlwaysOnHttpCallbacks()` 和
`RegisterInMatchHttpCallbacks()` 全部被注释。

**UE 5.7.4 底层含义:**

- `StartAllListeners()` 只启用并启动模块中已经存在的 HTTP Listener（监听器），不会自动创建 Router（路由器）或 Endpoint（端点）。
- 父类 `RegisterAlwaysOnHttpCallbacks()` 只广播 RPC 列表发生变化，本身不添加路由。
- 因为当前子类从未实例化、父子注册函数也未被调用，所以即使传入 `-rpcport=`，也没有静态证据表明存在可访问的 Lyra HTTP 端点。

**Lyra 原设计:** 原项目子类负责创建并 Root（根引用）单例、解析 JSON、
注册 / 注销路由，并提供作弊命令、单次开火、玩家状态和前端 / 比赛阶段等
自动化接口。当前只保留类型与构建依赖边界，状态为**结构占位**。

> 名称区分：这里的 External RPC 通过 HTTP 服务自动化工具；`ServerCheat()` 是 Unreal 网络复制系统的 Server RPC（服务器远程过程调用）。二者共享“RPC”名称，但不走同一套传输、权限或生命周期。

---

### ULyraPlatformEmulationSettings [Editor-Dev]

**当前状态：** **部分复刻**

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

## 核心数据流与触发流程

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

这两条路径都在 Game Thread（游戏线程）执行。PIE 设置读取的是
`EditorPerProjectUserSettings`（每项目每用户编辑器设置），不会通过网络
复制；Cheat Server RPC 则由所属 PlayerController 的网络连接把请求发送给
服务器。HTTP Listener 启动和路由注册是两步，当前只存在前者。

## 网络与权限

| 路径 | 调用方向 | 可靠性 / 持久性 | 当前验证 | Shipping 边界 |
|---|---|---|---|---|
| `ServerCheat()` | Owning Client → Server | Reliable，一次性命令事件，不保存状态 | `_Validate()` 恒 true | UFUNCTION 声明存在，执行体受 `USING_CHEAT_MANAGER` 保护 |
| `ServerCheatAll()` | Owning Client → Server，服务器遍历 Controller | Reliable，一次性命令事件 | `_Validate()` 恒 true | 同上 |
| External RPC | 外部 HTTP Client → 计划中的 Route Handler | 当前没有 Route | 无请求处理函数 | `WITH_RPC_REGISTRY=0` 关闭功能分支 |
| Developer Settings | 本机 Editor / PIE | Config 持久化，不复制 | 不适用 | 类可编译，`WITH_EDITOR` 代码被移除 |

Server RPC（服务器远程调用）不是管理员鉴权机制；Reliable（可靠）只表示
网络层重试和有序传递，不表示命令安全。External RPC 也不是 Unreal 属性复制
或 Net RPC，两者不能共享权限结论。

---

## 资产、配置与模块依赖

| 来源 | 当前配置 | 作用 |
|---|---|---|
| `Config=EditorPerProjectUserSettings` | 每位开发者独立保存 Developer / Platform 设置 | 不应把个人配置当项目运行时默认值 |
| `ULyraDeveloperSettings::CheatsToRun` | 按 `ECheatExecutionTime` 选择执行时机 | Possess 路径只在 `WITH_SERVER_CODE && WITH_EDITOR` 中执行 |
| `ConsoleVariable` Meta | 力反馈和 Gameplay Message 日志字段绑定 CVar（控制台变量） | Config 加载后可自动影响运行行为 |
| `LyraGame.Build.cs` | 依赖 `ExternalRpcRegistry`、`HTTPServer`、`Json` | 只证明可编译引用，不证明路由已实现 |
| `WITH_RPC_REGISTRY` / `WITH_HTTPSERVER_LISTENERS` | Shipping 为 0，其他配置为 1 | 控制功能路径，不包围当前 UCLASS 声明 |
| `Gauntlet` 插件 | 当前项目启用 | 提供自动化测试背景，但当前项目的 Lyra HTTP 端点仍缺失 |

---

## 与 Lyra 的差异

### 差异：Cheat Manager 只有空子类

**当前项目：** Controller 能在非发布功能路径中创建
`ULyraCheatManager`，但类体没有项目命令。

**Lyra：** Cheat Manager 提供 GAS、伤害/治疗、GameplayTag（游戏玩法标签）、
无敌、固定相机和 Debug Camera（调试相机）等命令。

**影响：** 当前只能使用引擎基础 Cheat，无法执行 Lyra 项目专属调试操作。

**建议：** 仅复刻当前学习和测试真正需要的命令，避免把 ShooterCore 专属
调试能力无差别迁入。

**状态：** **结构占位**

### 差异：External RPC 没有形成 HTTP 生命周期

**当前项目：** 有空子类、构建依赖和 Listener 启动入口，没有对象实例、
Route（路由）、Handler（处理函数）或 Deregister（注销）。

**Lyra：** 创建 Root 单例，解析 JSON，按前端 / 比赛阶段注册不同路由，并
在阶段结束时注销。

**影响：** `rpcport` 不会产生可访问的 Lyra 自动化端点。

**建议：** 如果当前阶段没有外部自动化需求，可保持低优先级；如需开放，
先定义绑定地址、身份验证、命令白名单和对称清理。

**状态：** **结构占位**

---

## 已识别的 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 中 | 当前源码 TODO | 完成共享设置后恢复 Controller 力反馈绑定 | `ALyraPlayerController::SetPlayer()` 注释代码 | `ULyraSettingsShared` |
| 中 | Lyra 对比 TODO | 按实际需要增加 Lyra Cheat 命令 | `ULyraCheatManager` 空类 | 对应 GAS / 相机 / 伤害系统 |
| 中 | 验证 TODO | 验证非发布联网服务器的 Cheat 调用身份和命令范围 | 两个 `_Validate()` 恒 true | Dedicated Server 测试环境 |
| 低 | Lyra 对比 TODO | 决定是否继续实现 External RPC | 当前只有结构占位 | Gauntlet / 外部自动化需求 |
| 低 | 验证 TODO | 验证无路由时 `rpcport` 不产生 Lyra Endpoint | 当前注册代码注释 | 非发布构建与端口观察工具 |
| 低 | 文档 TODO | 记录实际采用的 HTTP 绑定和鉴权策略 | 当前没有项目决策 | External RPC 设计完成 |

---

## 快速回顾

- **一句话职责：** 开发工具框架为 PIE 调试、平台模拟和受控测试入口提供配置。
- **核心入口：** `PreCreatePIEInstances()`、
  `APlayerController::PostInitializeComponents()`、`BeginPlay()`。
- **核心状态：** 每用户 Editor Config、CheatManager 实例和计划中的 HTTP 路由。
- **网络位置：** Developer Settings 只在本机；Cheat RPC 由 Owning Client
  请求服务器；External RPC 计划由外部 HTTP 客户端调用。
- **当前完成度：** 设置类**部分复刻**，Cheat / External RPC 类
  **结构占位**。
- **最重要的未完成项：** 项目专属 Cheat 命令与 External RPC 路由生命周期。

## 复习要点

1. `EditorPerProjectUserSettings` 为什么不能当作项目默认运行配置？
2. Cheat Manager 是 Actor、Component 还是以 PlayerController 为 Outer 的 UObject？
3. `PostInitializeComponents()` 何时创建 Cheat Manager？
4. Reliable Server RPC 为什么不等于已完成权限验证？
5. `StartAllListeners()` 为什么不等于已经存在 HTTP Endpoint？
6. Shipping 宏关闭的是类型声明还是功能路径？
7. Server Cheat RPC 与 External RPC 的传输和生命周期有何不同？

---

## 关联框架

- [12-Editor-Module.md](12-Editor-Module.md) — ULyraEditorEngine 在 PreCreatePIEInstances 中调用这些设置
- [07-Experience-Framework.md](07-Experience-Framework.md) — ExperienceOverride 影响加载哪个 Experience
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerController 创建 Cheat Manager，并承载 Server Cheat RPC
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — 按来源和优先级记录 Cheat、External RPC 与设置缺口
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — Cheat、External RPC 与 PlayerController 生命周期对照
