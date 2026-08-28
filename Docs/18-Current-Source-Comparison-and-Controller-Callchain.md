# 18 - 最新提交源码对照与 PlayerController 调用链

> 本文以当前项目最新提交 `19e6961` 为代码基线，解释
> `ALyraPlayerController`（Lyra 玩家控制器）如何连接 Pawn（受控实体）、
> `ALyraPlayerState`（Lyra 玩家状态）、GAS（Gameplay Ability System，
> 游戏玩法能力系统）、Camera（相机）、Replay（回放）、Cheat（作弊命令）
> 和 External RPC（外部远程过程调用），并严格区分当前项目事实、Lyra
> 对照结论、UE 5.7.4 引擎机制与尚未验证的行为。

> **版本与时间：**
> 当前项目和 Lyra 参考项目的 `.uproject` 都声明
> `EngineAssociation: 5.7`；本文使用的引擎源码为 Unreal Engine
> 5.7.4，Changelist 51494982。Lyra 参考项目没有单独记录精确补丁号，
> 因此只确认到 5.7 系列。文档本次复核日期为 2026-07-13。

> ⚠️ **范围说明：** 本文保留为提交 `19e6961` 的专项审计快照。2026-08-28
> 工作区新增但尚未提交的 LocalPlayer（本地玩家）、Settings（设置）、
> GameInstance（游戏实例）与 Experience（体验）改动不改变本文对该提交的
> Controller / Camera / Replay 结论；新调用链见
> [08-UI-Framework.md](08-UI-Framework.md) 和
> [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md#12-当前工作区的-localplayerteam-与-settings-主链)。

> 🧪 **待验证：**
> 本文是静态源码审计，没有执行编译、UHT（Unreal Header Tool，
> 虚幻头文件工具）、PIE（Play In Editor，编辑器内运行）、多人联网、
> Replay 或 HTTP 测试。

---

## 框架概述

**问题：** `APlayerController`（玩家控制器）同时处在本地输入、服务器
控制权、PlayerState（玩家状态）复制、相机、观战和调试入口的交界处。
只确认某个同名函数存在，容易把父类能力、当前项目实现和 Lyra 完整实现
混为一谈。

**当前项目的解决方案：** 当前项目在 `ALyraPlayerController` 中接入
Pawn 控制切换、PlayerState 变化通知、队伍代理、自动奔跑、力反馈过滤、
观战旋转、相机穿透后的单帧隐藏、回放跟随和非发布构建调试入口；
玩家级 ASC（Ability System Component，能力系统组件）仍由
`ALyraPlayerState` 持有。

**设计意图：**

- 让 PlayerState 保存跨 Pawn 生命周期持续存在的玩家状态和 ASC。
- 让 PlayerController 协调本地输入、相机和当前 Pawn，而不成为队伍数据
  的权威来源。
- 用 `ReplicatedViewRotation`（复制视角旋转）替代父类只面向拥有者的
  `TargetViewRotation` 复制，以服务观战和客户端保存回放。
- 把 Camera Assist（相机辅助）、Replay 和开发工具做成可继续复刻的独立
  协作边界。

**当前完成度：** Controller 主体属于**部分复刻**；队伍代理和回放跟随
已有主要代码，但 GAS 输入、生成能力补激活、共享设置、相机穿透生产端、
Lyra 相机管理器扩展、客户端回放录制和 External RPC 路由仍未形成完整流程。

---

## 阅读目标与证据边界

### 阅读目标

本次源码阅读目标是弄清楚：

`AController::Possess()` 或 PlayerState 复制到达后，当前项目如何更新
Controller、Pawn、ASC、Team（队伍）和相机相关状态，并最终产生可观察的
移动、观战或隐藏结果。

### 当前项目证据

| 范围 | 稳定源码位置与符号 | 证据用途 |
|---|---|---|
| Controller / Replay | `Source/LyraGame/Player/LyraPlayerController.h/.cpp`：`ALyraPlayerController::*`、`ALyraReplayPlayerController::*` | 生命周期、输入、复制、相机隐藏、作弊、回放 |
| PlayerState | `Source/LyraGame/Player/LyraPlayerState.h/.cpp`：`InitAbilityActorInfo()`、`SetPawnData()`、`SetReplicatedViewRotation()` | ASC Owner/Avatar、Push Model（推送模型）、队伍与观战状态 |
| Camera | `Source/LyraGame/Camera/LyraCameraAssistInterface.h/.cpp`、`LyraPlayerCameraManager.h/.cpp` | 相机协作接口与空子类边界 |
| Cheat | `Source/LyraGame/Player/LyraCheatManager.h/.cpp` | 构建宏、类体和日志分类 |
| External RPC | `Source/LyraGame/Tests/LyraGameplayRpcRegistrationComponent.h/.cpp`、`Source/LyraGame/LyraGame.Build.cs` | HTTP 自动化类型、模块依赖与构建开关 |
| 创建入口 | `Source/LyraGame/GameModes/LyraGameMode.cpp`：构造函数 | Controller 和 Replay Controller 的默认类选择 |
| 配置边界 | `Config/DefaultEngine.ini`：`GlobalDefaultGameMode` | 蓝图 GameMode 可能覆盖 C++ 默认类 |

最新提交还修改了 `LyraPlayerState.h` 与
`LyraTeamAgentInterface.h` 的中文注释。注释用于解释设计意图，但字段、
函数和复制注册仍以实际代码为准。

### Lyra 与引擎对照

- Lyra 原项目：
  `D:\UE\project\C++Project\LyraStarterGame\Source\LyraGame\Player`、
  `Camera`、`Replays`、`Tests`。
- UE Controller（控制器）：
  `Engine/Source/Runtime/Engine/Private/Controller.cpp` 中的
  `AController::Possess()`、`OnPossess()`、`OnRep_PlayerState()`。
- UE PlayerController（玩家控制器）：
  `Engine/Source/Runtime/Engine/Private/PlayerController.cpp` 中的
  `PostInitializeComponents()`、`TickActor()`、
  `GetLifetimeReplicatedProps()`、`BuildHiddenComponentList()`。
- UE 视图构建：
  `Engine/Source/Runtime/Engine/Private/LocalPlayer.cpp` 中的
  `ULocalPlayer::CalcSceneView()`。
- UE GAS：
  `GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp` 中的
  `InitAbilityActorInfo()`、`RefreshAbilityActorInfo()`。
- UE HTTP / External RPC：
  `HTTPServer/Private/HttpServerModule.cpp` 和
  `ExternalRPCRegistry/Private/ExternalRpcRegistrationComponent.cpp`。

### 调用链起点、终点与停止边界

| 阅读链 | 起点 | 本文终点 | 停止深入的原因 |
|---|---|---|---|
| 控制权与 GAS | `AController::Possess()` / `UnPossess()` | Pawn 控制关系和 ASC Avatar 状态稳定 | 后续 Character Movement（角色移动）不改变本文结论 |
| PlayerState 到达 | `AController::OnRep_PlayerState()` | ASC ActorInfo（能力执行体信息）完成补刷新 | Ability 内部激活策略尚未接入 |
| 观战视角 | `APlayerController::TickActor()` | `TargetViewRotation` 被读取并平滑消费 | 渲染矩阵和骨骼动画不属于本次目标 |
| 相机穿透 | Camera Mode（相机模式）检测命中 | Primitive ID（图元标识）进入本帧隐藏集合 | 当前项目没有生产端，无法继续追当前链 |
| Replay | 录制资格或 Recorder PlayerState 到达 | 开始录制或恢复 View Target（视图目标） | DemoNetDriver 编码和文件格式不影响接入判断 |
| External RPC | 构建宏与 `BeginPlay()` | Listener（监听器）、Router（路由器）和 Route（路由）是否存在 | 当前项目没有路由处理函数 |

暂时忽略通用容器遍历、日志格式、物理穿透算法和 Replay 文件编码；它们不
改变“当前调用链是否闭合”的结论。

---

## 当前复刻状态

| 模块 | 当前状态 | 当前项目代码证据 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| Controller 生命周期协调 | **部分复刻** | `OnPossess()`、`OnUnPossess()`、PlayerState 变化、输入后处理等钩子已接入 | Lyra 同名路径还会处理完整 GAS 输入、生成能力和共享设置 | 基础控制切换可读，完整玩家初始化链仍断开 |
| Team Agent（队伍代理） | **部分复刻** | Controller 从 PlayerState 读取 Team ID（队伍编号），迁移 Team Delegate（队伍委托），拒绝直接写入 | Lyra 还让 Character、LocalPlayer 等对象参与同一协议 | Controller 代理已成形，跨对象队伍链未完整验证 |
| Camera Assist（相机辅助） | **部分复刻** | 接口、`OnCameraPenetratingTarget()` 和 `UpdateHiddenComponents()` 已存在 | `LyraCameraMode_ThirdPerson` 负责生产穿透事件 | 没有生产者时，正常游戏流程不会设置隐藏标志 |
| PlayerCameraManager（玩家相机管理器） | **结构占位** | 类体为空，`.cpp` 仅包含头文件；Controller 已选择该类型 | Lyra 设置 80° FOV（视野角）、UI Camera（界面相机）并重写视图更新和调试 | 当前只有 UE 父类基础相机行为 |
| 观战旋转复制 | **部分复刻** | 禁用父类字段，PlayerState 以 `COND_SkipOwner` 复制 `ReplicatedViewRotation` | Lyra 用于观战和客户端保存回放 | 服务器远程 Controller 的持续写入来源存在待验证疑点 |
| Replay 播放跟随 | **部分复刻** | Replay Controller 重新绑定 Recorder PlayerState 和 Pawn，并调用 `SetViewTarget()` | 与 Lyra 主体逻辑一致 | 尚无 Seek（跳转）、Checkpoint（检查点）和 Pawn 更换运行证据 |
| 客户端 Replay 录制 | **未复刻** | 资格检查最终恒为 false；`RecordClientReplay()` API 与调用均缺失 | Lyra 由 Replay Subsystem（回放子系统）真正启动录制并清理旧回放 | 当前不能声称自动客户端录制可用 |
| Cheat Manager（作弊管理器）扩展 | **结构占位** | `ULyraCheatManager` 是空的 `UCheatManager` 子类 | Lyra 提供 GAS、伤害、标签、无敌和相机命令 | 只继承引擎基础作弊能力 |
| Server Cheat RPC（服务器作弊远程调用） | **已复刻** | 两个 Reliable Server RPC（可靠服务器远程调用）及执行体存在，`_Validate()` 恒为 true | Lyra 同名入口采用相同结构 | 非发布联网环境仍需按高权限入口管理 |
| External RPC | **结构占位** | 构建依赖、空子类和 Listener 启动入口存在 | Lyra 有单例、JSON、路由注册/注销和处理函数 | 当前没有 Lyra HTTP Endpoint（端点） |

---

## 类列表

| 类或接口 | 父类或接口 | 生命周期 | 网络位置 | 当前状态 | 当前项目中的实际职责 |
|---|---|---|---|---|---|
| `ALyraPlayerController` | `ACommonPlayerController`、`ILyraTeamAgentInterface`、`ILyraCameraAssistInterface` | Runtime（运行时） | Server 与 Owning Client（拥有者客户端）；复制 Actor | **部分复刻** | 协调 Pawn、PlayerState、ASC、输入、相机、观战和调试入口 |
| `ALyraReplayPlayerController` | `ALyraPlayerController` | Runtime / Replay | Replay World（回放世界） | **部分复刻** | 在回放跳转或 Pawn 变化后恢复对录制者的跟随 |
| `ULyraCameraAssistInterface` / `ILyraCameraAssistInterface` | `UInterface` / C++ 接口 | Runtime | 本地相机协作，不自行复制 | **部分复刻** | 定义穿透忽略、保护目标和穿透通知协议 |
| `ALyraPlayerCameraManager` | `APlayerCameraManager` | Runtime，Transient（瞬态） | Server 与 Owning Client，各自本地存在，不复制 | **结构占位** | 目前只提供 Lyra 命名类型并继承 UE 基础相机 |
| `ALyraPlayerState` | `AModularPlayerState`、`IAbilitySystemInterface`、`ILyraTeamAgentInterface` | Runtime，复制 | Server Authority（服务器权威），复制到相关客户端 | **部分复刻** | 保存玩家级 ASC、PawnData、Team/Squad、StatTag 和观战旋转 |
| `ULyraReplaySubsystem` | `UGameInstanceSubsystem` | Runtime，随 GameInstance | 本地子系统，不自行复制 | **部分复刻** | 当前只判断平台是否支持 Replay |
| `ULyraCheatManager` | `UCheatManager`，`Within=PlayerController` | Non-Shipping Runtime（非发布运行时功能路径） | 由各 PlayerController 本地持有 | **结构占位** | 继承引擎基础 Cheat 命令 |
| `ULyraGameplayRpcRegistrationComponent` | `UExternalRpcRegistrationComponent` | Non-Shipping Runtime 功能路径 | 计划作为 HTTP 注册对象，当前未实例化 | **结构占位** | 只保留 External RPC 类型边界 |

---

## 核心数据流

### 主流程一：Possess、UnPossess 与 ASC Avatar

```text
[服务器调用] AController::Possess(InPawn)
  |
  |-- 检查 Authority（网络权威）
  v
ALyraPlayerController::OnPossess(InPawn)
  |
  |-- Super::OnPossess()
  |     |-- 解除旧 Pawn
  |     |-- InPawn->PossessedBy(this)
  |     |-- SetPawn(InPawn)
  |     |-- SetControlRotation()
  |     `-- Pawn->DispatchRestart()
  |
  |-- [Editor + Server Code] 执行配置的 Possession Cheat
  `-- SetIsAutoRunning(false)
        `-- ASC 移除 Status_AutoRunning

[服务器解除控制] AController::UnPossess()
  v
ALyraPlayerController::OnUnPossess()
  |
  |-- 若 ASC.AvatarActor == 旧 Pawn
  |     `-- ASC::SetAvatarActor(nullptr)
  `-- Super::OnUnPossess()
        `-- Pawn 解除控制，Controller 清空 Pawn
```

这条流程在 Game Thread（游戏线程）同步执行。`Possess()` 拒绝非权威调用；
`OnUnPossess()` 只在旧 Pawn 与 ASC Avatar 相同时清空，避免误清其他
Avatar。当前 Controller 不负责给新 Pawn 设置 Avatar；这一步预期由尚未
完成的 Pawn Extension（Pawn 扩展）初始化链承担。

| 节点 | 输入 | 条件 | 状态变化 | 下一跳或失败行为 |
|---|---|---|---|---|
| `AController::Possess()` | 目标 Pawn | 默认要求 Authority | 进入虚函数控制切换 | 非权威时记录警告并返回 |
| `Super::OnPossess()` | 目标 Pawn | Pawn 有效且可控制 | Controller/Pawn 所有权、旋转、Restart 更新 | 空 Pawn 时直接结束 |
| `ALyraPlayerController::OnPossess()` | 已稳定的 `GetPawn()` | Editor 作弊还要求 `GIsEditor` 和最终 Pawn 匹配 | 清除自动奔跑状态 | ASC 不存在时只无法改变 Loose Tag（松散标签） |
| `OnUnPossess()` | 旧 Pawn、PlayerState ASC | Avatar 必须等于旧 Pawn | Avatar 先清空 | PlayerState 或 ASC 无效时跳过清理，再交给父类 |
| `Super::OnUnPossess()` | 已解除 Avatar 的旧 Pawn | Pawn 仍存在 | Pawn 与 Controller 解除关联 | 引擎完成通知和委托广播 |

### 主流程二：PlayerState 复制补偿

```text
[服务器]
InitPlayerState() / CleanupPlayerState()
  `-- BroadcastOnPlayerStateChanged()
        |-- 解绑旧 PlayerState 的 Team Delegate
        |-- 绑定新 PlayerState 的 Team Delegate
        `-- OldTeam != NewTeam 时广播 Controller Team 变化

[拥有者客户端]
PlayerState 属性复制到达
  v
AController::OnRep_PlayerState()
  |-- PlayerState->ClientInitialize(this)
  v
ALyraPlayerController::OnRep_PlayerState()
  |-- BroadcastOnPlayerStateChanged()
  `-- [仅 NM_Client] ASC::RefreshAbilityActorInfo()
        `-- [当前注释] TryActivateAbilitiesOnSpawn()
```

这条补偿链解决 PlayerState 和 ASC 可能早于 PlayerController 完成解析的
顺序问题。当前只恢复 ActorInfo 中的 PlayerController 与本地控制关系；
生成时能力的补激活仍未接入。

---

## 初始化与运行流程

### 1. 类选择与对象创建

```text
ALyraGameMode 构造
  |-- PlayerControllerClass = ALyraPlayerController
  `-- ReplaySpectatorPlayerControllerClass = ALyraReplayPlayerController

ALyraPlayerController 构造
  |-- PlayerCameraManagerClass = ALyraPlayerCameraManager
  `-- [USING_CHEAT_MANAGER] CheatClass = ULyraCheatManager

UE APlayerController::PostInitializeComponents()
  |-- [非 Client] InitPlayerState()
  |-- SpawnPlayerCameraManager()
  |     |-- World::SpawnActor()
  |     |-- Owner = PlayerController
  |     |-- RF_Transient
  |     `-- InitializeFor(PlayerController)
  |-- [Client] SpawnDefaultHUD()
  `-- AddCheats()
        `-- [允许或强制] NewObject<UCheatManager>(PlayerController)
```

PlayerCameraManager（玩家相机管理器）是 World 中的瞬态 Actor，Owner 为
PlayerController；CheatManager（作弊管理器）是以 PlayerController 为
Outer（外部对象）的 UObject，由 Controller 属性持有并受 GC（垃圾回收）
管理。

### 2. 每帧输入与观战

- `PlayerTick()` 在拥有 `PlayerInput` 的路径执行：先交给父类处理输入，
  再添加自动奔跑输入并读写观战旋转。
- UE 5.7.4 的服务器非本地 Autonomous Proxy（自治代理）
  PlayerController 走 `TickActor()` 的远程专用分支，不进入
  `PlayerTick()`。
- `PostProcessInput()` 已取得 ASC，但 `ProcessAbilityInput()` 仍注释，
  所以 Input Tag（输入标签）到 Ability 激活、Prediction（预测）和服务器
  确认的主链尚未闭合。

### 3. 清理路径

- `OnUnPossess()` 在父类清 Pawn 前清除匹配的 ASC Avatar。
- `CleanupPlayerState()` 会触发 `BroadcastOnPlayerStateChanged()`，
  解绑旧 PlayerState 的队伍委托。
- `EndPlay()` 当前只调用 `Super::EndPlay()`，没有 External RPC 注销。
- Replay Controller 使用 `AddUObject` 和 `AddUniqueDynamic` 绑定委托；
  当前代码依赖 UObject 感知委托避免销毁后执行，没有额外的显式解绑流程。

---

## 逐类详解

### ALyraPlayerController [Runtime，复制 Actor]

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：
  `Source/LyraGame/Player/LyraPlayerController.h/.cpp`。
- Lyra 参考：
  `Source/LyraGame/Player/LyraPlayerController.h/.cpp`。
- 引擎父类：
  `Engine/Source/Runtime/Engine/Private/Controller.cpp`、
  `PlayerController.cpp`。

**继承链：**

`AActor → AController → APlayerController → ACommonPlayerController
→ ALyraPlayerController`

**实现接口：**

- `ILyraTeamAgentInterface`（Lyra 队伍代理接口）。
- `ILyraCameraAssistInterface`（Lyra 相机辅助接口）。

**创建位置与所有权：** `ALyraGameMode` 把
`PlayerControllerClass` 设为该类。服务器为连接创建权威实例，拥有者客户端
接收自己的 PlayerController；其他普通客户端不会拥有该玩家的
PlayerController，而是通过 PlayerState 和 Pawn 观察其状态。

**关键属性：**

| 属性 | 类型 | 复制 | 当前用途 |
|---|---|---|---|
| `bHideViewTargetPawnNextFrame` | `bool` | 否 | 记录下一次视图构建是否隐藏 View Target |
| `OnTeamChangedDelegate` | 动态多播委托 | 否 | 对外转发 PlayerState 队伍变化 |
| `LastSeenPlayerState` | `TObjectPtr<APlayerState>` | 否 | 解绑旧 PlayerState 委托并比较旧队伍 |
| `PlayerCameraManagerClass` | `TSubclassOf<APlayerCameraManager>`，父类字段 | 类选择随对象构造，不作为游戏状态复制 | 指定 `ALyraPlayerCameraManager` |
| `CheatClass` | `TSubclassOf<UCheatManager>`，父类字段 | 否 | 非发布构建选择 `ULyraCheatManager` |

#### `OnPossess()` / `OnUnPossess()`

- **调用者：** UE `AController::Possess()` / `UnPossess()`。
- **调用时机：** 服务器切换 Pawn 控制权时。
- **执行端：** 正常路径为 Server Authority（服务器权威）。
- **前置条件：** `Possess()` 默认要求权威；解除控制时旧 Pawn 仍可读取。
- **当前实现：** 控制成功后执行编辑器作弊并关闭 Auto-run；解除前清理匹配的
  ASC Avatar。
- **副作用：** 改变 Pawn 控制关系、ASC Avatar 和
  `Status_AutoRunning` Loose Gameplay Tag（松散游戏玩法标签）。
- **失败处理：** 非权威 Possess 由引擎记录警告；无 PlayerState/ASC 时跳过
  Avatar 清理。
- **Lyra 差异：** 这部分主体已接入，但新 Pawn 的 ASC Avatar 初始化仍依赖
  未完成的 Pawn Extension。

#### `OnRep_PlayerState()`

- **调用者：** UE 属性复制系统在 PlayerState 引用到达客户端时调用。
- **执行端：** 拥有者客户端。
- **当前实现：** 先执行父类 `ClientInitialize()`，再迁移队伍委托并刷新
  ASC ActorInfo。
- **副作用：** 恢复 ASC 对 PlayerController 和本地控制关系的缓存。
- **失败处理：** PlayerState 或 ASC 不存在时安全跳过。
- **Lyra 差异：** `TryActivateAbilitiesOnSpawn()` 被注释，补激活未完成。
- **引擎机制：** `RefreshAbilityActorInfo()` 只重新从现有 Owner/Avatar
  构造 ActorInfo，不授予或主动激活 Ability。

#### `PlayerTick()`

- **调用者：** UE `APlayerController::TickActor()` 的本地输入分支。
- **执行端：** 有 `PlayerInput` 的本地 Controller；包括本地客户端或监听
  服务器本地玩家。
- **当前实现：** 添加自动奔跑移动输入，写入本地/权威视角旋转，或读取被观战
  Pawn 的 PlayerState 旋转。
- **副作用：** 修改 Pawn 输入、PlayerState 的
  `ReplicatedViewRotation` 或本地 `TargetViewRotation`。
- **当前限制：** Dedicated Server（专用服务器）的远程 Controller
  不进入此函数，权威旋转写入来源需验证。

#### `UpdateHiddenComponents()`

- **调用者：** `ULocalPlayer::CalcSceneView()` →
  `BuildHiddenComponentList()`。
- **执行端：** 构建本地视图的一端，Game Thread。
- **前置条件：** `OnCameraPenetratingTarget()` 已把一次性标志设为 true。
- **当前实现：** 收集 View Target 已注册的
  `UPrimitiveComponent`（图元组件）及其未标记 `NoParentAutoHide` 的
  直接附加图元，把 `FPrimitiveComponentId` 加入本帧隐藏集合。
- **副作用：** 只影响本帧 Scene View（场景视图）；消费后重置标志。
- **Lyra 差异：** 武器隐藏仍注释，Camera Mode 穿透生产端缺失。

#### `PostProcessInput()`

- **调用者：** `APlayerController` 在 PlayerInput Tick 后调用。
- **执行端：** 本地 Controller。
- **当前实现：** 能取得 Lyra ASC，但没有调用 `ProcessAbilityInput()`。
- **影响：** 当前 Controller 不能证明 Input Tag 已进入 Ability、Cost
  （消耗）、Cooldown（冷却）、Prediction Key（预测键）和服务器确认链。

#### 仅保留结构的钩子

`PreInitializeComponents()`、`ReceivedPlayer()` 和 `PreProcessInput()` 当前只
调用 `Super`；`EndPlay()` 也只有父类清理；`OnPlayerStateChanged()` 是空的
派生类扩展点。这些函数可以保留生命周期位置，但不能单独视为“已复刻”的
业务行为，也没有补上 External RPC 注销或共享设置初始化。

### ALyraReplayPlayerController [Runtime，Replay]

**当前状态：** **部分复刻**

**创建位置与所有权：** `ALyraGameMode` 设置
`ReplaySpectatorPlayerControllerClass`；Replay World 由引擎创建该
Controller。

**当前主流程：**

```text
Replay Tick
  |-- FollowedPlayerState 失效
  |     |-- 绑定 GameState::OnRecorderPlayerStateChangedEvent
  |     `-- 读取当前 RecorderPlayerState
  |           `-- 绑定 PlayerState::OnPawnSet
  `-- Pawn 到达或变化
        `-- SetViewTarget(NewPlayerPawn)
```

`SmoothTargetViewRotation()` 只调用父类；`ShouldRecordClientReplay()`
固定返回 false，防止 Replay 观战 Controller 递归录制。

### ILyraCameraAssistInterface [Runtime]

**当前状态：** **部分复刻**

接口默认实现不追加忽略 Actor、不提供防穿透目标、也不处理穿透通知。
`ALyraPlayerController` 只重写 `OnCameraPenetratingTarget()`。

Lyra 原项目的 `LyraCameraMode_ThirdPerson` 会在穿透检测命中后，把
Controller、View Target 和防穿透目标转换为该接口并调用回调。当前项目
C++ 搜索没有对应生产者。

> ⚠️ **注意：**
> Lyra 原接口方法拼写为
> `GetIgnoredActorsForCameraPentration()`；当前项目修正为
> `GetIgnoredActorsForCameraPenetration()`。以后移植调用者时必须统一
> 符号名。

### ALyraPlayerCameraManager [Runtime]

**当前状态：** **结构占位**

UE 5.7.4 父类提供 View Target、镜头缓存、混合、旋转限制和每帧相机更新；
默认 FOV 为 90°，Pitch 范围为 -89.9° 到 89.9°。这些是父类能力，不是
当前项目中该 Lyra 风格子类的复刻成果。

Lyra 原类额外设置 80° FOV 和 -89° 到 89° Pitch，创建
`ULyraUICameraManagerComponent`，并重写 `UpdateViewTarget()` 与
`DisplayDebug()`。当前类没有这些成员和重写。

### ULyraCheatManager [Non-Shipping Runtime 功能路径]

**当前状态：** **结构占位**

`APlayerController::AddCheats()` 在允许或强制时以 PlayerController 为
Outer 创建 CheatManager。当前子类没有 Lyra 专属命令，但继承
`UCheatManager` 基础能力。

`USING_CHEAT_MANAGER` 在 Shipping（发布）构建为 false。类和
`ServerCheat` RPC 声明仍可参与编译；被宏保护的创建选择和执行体在
Shipping 功能路径中不会执行。

### ULyraGameplayRpcRegistrationComponent [Non-Shipping Runtime 功能路径]

**当前状态：** **结构占位**

当前类只有 `GENERATED_BODY()`，没有单例、创建点、路由、Handler
（处理函数）或注销。`BeginPlay()` 中的注册调用被注释，因此
`rpcport` 参数不会让该类型自动提供 Lyra HTTP 端点。

---

## 中文注释与代码证据边界

| 中文注释表达的意图 | 实际代码证据 | 文档结论 |
|---|---|---|
| `OnRep_PlayerState()` “给客户端补刷新和补激活” | `RefreshAbilityActorInfo()` 有效，`TryActivateAbilitiesOnSpawn()` 被注释 | 只能确认补刷新，不能确认补激活 |
| `PlayerTick()` “只会在本地控制的 PlayerController 上调用” | UE 5.7.4 远程服务器分支确实绕过 `PlayerTick()` | 该注释解释了为什么服务器远程观战旋转写入存在疑点 |
| `SetReplicatedViewRotation()` “仅在服务器上有效” | 函数体没有 Authority 检查，本地客户端调用也会写本地值 | 注释说明复制意图，不是运行时权限约束 |
| `ConditionalBroadcastTeamChanged()` “新旧 Team ID 不同才广播” | 函数先比较 `OldTeamID != NewTeamID`，只在变化时记录日志并执行委托 | 该中文注释与代码条件一致，可作为 Team 事件去重规则 |
| `TryToRecordClientReplay()` “由 Game State 逻辑调用” | 当前 C++ 搜索没有调用者，且真实录制调用被注释 | 只确认蓝图可调用入口存在，不能确认自动触发 |
| 相机隐藏注释说明 `FPrimitiveComponentId` 跨线程用途 | 代码确实把 Scene ID 加入 HiddenPrimitives，而非传递 UObject 指针 | 注释与 UE 视图构建机制一致 |

---

## 网络与权限

### 权威模型

这个网络问题的本质是：Pawn 控制、Team、PawnData 和可复制视角状态由
Server Authority 维护，再通过 PlayerState 属性复制或 Server RPC 让目标
客户端获得状态；Owning Client 可以处理本地输入和表现，但不能靠直接写
复制属性改变服务器权威状态。

| 数据或事件 | 权威或产生端 | 同步方式 | 目标 | 客户端行为 |
|---|---|---|---|---|
| Pawn 控制关系 | Server | Controller/Pawn 原生复制与 Restart 流程 | Owning Client 及相关客户端 | 接收控制结果，本地预测移动 |
| PlayerState Team ID | Server | Push Model 属性复制 + `OnRep_MyTeamID()` | 相关客户端 | 更新本地状态并广播队伍变化 |
| `ReplicatedViewRotation` | 设计上应由 Server 持续更新 | Push Model，`COND_SkipOwner` | 非拥有者客户端与 Replay 相关路径 | 观战者读取并平滑表现 |
| Auto-run Loose Tag | 当前 Controller 本地写 ASC | 当前没有单独复制规则说明 | 本地 Controller / ASC | 每帧转成移动输入；不等同于网络权威移动状态 |
| `ServerCheat()` / `ServerCheatAll()` | Owning Client 发起请求，Server 执行 | Reliable Server RPC | Server | 客户端只提交命令字符串 |
| External RPC | 计划由外部 HTTP 客户端发起 | 当前没有 Route | 无 | 当前没有 Lyra 响应 |

### 观战旋转调用链与疑点

```text
[本地 PlayerController::PlayerTick]
  |-- CameraManager::GetViewTargetPawn()
  |-- Authority 或 TargetPawn 本地控制
  |     `-- PlayerState::SetReplicatedViewRotation()
  |           `-- [服务器] COND_SkipOwner 复制给其他客户端
  `-- TargetPawn 非本地控制
        `-- 读取目标 Pawn 的 LyraPlayerState
              `-- TargetViewRotation = ReplicatedViewRotation
                    `-- SmoothTargetViewRotation()
```

UE 5.7.4 的服务器非本地 Autonomous Proxy Controller 在
`TickActor()` 中只更新父类 `TargetViewRotation`，不会调用当前项目
`PlayerTick()`。而父类字段的复制已被
`DISABLE_REPLICATED_PROPERTY` 禁用，当前 C++ 也没有第二处
`SetReplicatedViewRotation()` 调用。

> ⚠️ **推断：**
> Dedicated Server 上远程玩家的 `ReplicatedViewRotation` 可能缺少持续的
> 权威更新来源。该结论由调用链推导，必须用服务器与远程客户端运行验证，
> 不能写成已发生的运行时故障。

### RPC 验证、可靠性与 Shipping 边界

- 两个 Cheat RPC 都是 Client → Server 的 Reliable RPC，并依赖
  PlayerController 的 Owning Connection（所属网络连接）。
- `WithValidation` 存在，但两个 `_Validate()` 都无条件返回 true；
  当前没有命令白名单或额外身份判断。
- Shipping 中 `USING_CHEAT_MANAGER` 为 false，CheatClass 选择和命令
  执行体被功能宏裁剪为空路径；UCLASS 和 UFUNCTION 声明并未由该宏包围。
- `WITH_RPC_REGISTRY=0` 会移除 Controller 的 HTTP 注册路径和引擎
  External RPC 功能分支，但当前模块仍声明相关依赖和类型。

### 晚加入与初始复制

晚加入客户端在 PlayerState 相关 Actor 对其 Relevant（网络相关）时，可以
通过初始属性复制取得服务器当前的 Team、PawnData 和观战旋转；后续变化再走
Delta Replication（增量复制）。但 `COND_SkipOwner` 会让拥有者跳过自己的
`ReplicatedViewRotation`，而且若服务器从未持续更新该字段，晚加入者只能
取得陈旧或默认值。

---

## Camera、Replay 与 External RPC 支线

### 相机穿透到本帧隐藏集合

```text
[Lyra 中存在，当前缺失]
Camera Mode 穿透检测
  v
[当前已实现]
ILyraCameraAssistInterface::OnCameraPenetratingTarget()
  v
bHideViewTargetPawnNextFrame = true
  v
ULocalPlayer::CalcSceneView()
  v
APlayerController::BuildHiddenComponentList()
  v
ALyraPlayerController::UpdateHiddenComponents()
  |-- View Target 的已注册 Primitive Component
  |-- 未标记 NoParentAutoHide 的直接附加图元
  `-- 加入本帧 HiddenPrimitives，随后重置标志
```

这条消费流程是同步的本地视图构建路径，不复制到网络。当前失败方式不是崩溃：
没有生产者时标志始终为 false，隐藏逻辑不会执行。

### Replay 播放与录制必须分开判断

**播放跟随：** Replay Controller 已能在 PlayerState 因 Seek 或 Checkpoint
失效后重新绑定 GameState、Recorder PlayerState 与 Pawn，最后调用
`SetViewTarget()`。

**客户端录制：** `ShouldRecordClientReplay()` 会检查 World、GameInstance、
NetMode、前端地图、本地 Controller 和引擎 Replay 状态；但共享设置判断被
注释，函数最终恒为 false。即使派生类返回 true，
`TryToRecordClientReplay()` 也只设置 Recorder PlayerState，跳过真正的
`RecordClientReplay()`，却返回 true。

### External RPC 当前链

```text
LyraGame.Build.cs
  |-- Shipping: WITH_RPC_REGISTRY=0
  `-- Non-Shipping: WITH_RPC_REGISTRY=1
        v
ALyraPlayerController::BeginPlay()
  |-- FHttpServerModule::StartAllListeners()
  `-- 解析 rpcport
        `-- [注释] GetInstance + Register Routes

ULyraGameplayRpcRegistrationComponent
  `-- 空子类，当前无创建点、Route、Handler、注销
```

UE 5.7.4 的 `StartAllListeners()` 只启动模块中已经存在的 Listener，不创建
Router 或 Endpoint。父类 `RegisterAlwaysOnHttpCallbacks()` 也只广播 RPC
列表变化，本身不添加 Lyra 路由。

---

## 资产、配置与模块依赖

| 来源 | 当前配置 | 对运行时结论的影响 |
|---|---|---|
| `ALyraGameMode` C++ 构造函数 | 选择 `ALyraPlayerController` 和 `ALyraReplayPlayerController` | C++ 默认类已经接入 |
| `Config/DefaultEngine.ini` | `GlobalDefaultGameMode=/Game/B_LyraGameMode...` | 蓝图子类可以覆盖 C++ 默认类，最终运行配置需检查资产 |
| `Content/B_LyraGameMode.uasset` | 资产存在 | 二进制资产的 Class Override（类覆盖）尚未可靠解析 |
| Camera C++ | 没有 `LyraCameraMode_ThirdPerson` 或回调调用者 | 代码侧生产端未复刻 |
| Content 搜索 | 未发现明确命名的 Camera Mode 资产 | 不能据文件名证明所有蓝图都没有间接调用，仍属待资产验证 |
| `ULyraDeveloperSettings` | `bShouldAlwaysPlayForceFeedback` 绑定 `LyraPC.ShouldAlwaysPlayForceFeedback` | Project Settings（项目设置）可改变力反馈过滤 |
| `LyraGame.Build.cs` | 依赖 `GameplayAbilities`、`CommonInput`、`NetworkReplayStreaming`、`ExternalRpcRegistry`、`HTTPServer` | 编译依赖已声明，不等于功能链已实现 |
| `Lyra.uproject` | `ShooterCore`、`ShooterMaps` 等示例玩法插件关闭 | 不应把原项目插件内容默认视为当前运行时能力 |

> 🧪 **待验证：**
> `B_LyraGameMode` 是否覆盖 PlayerController、Replay Controller 或其他
> C++ 默认值，需要通过 Unreal Editor（虚幻编辑器）资产检查或运行日志确认。

---

## 与 Lyra 的差异

### 差异一：GAS 输入与生成能力补偿未闭合

**当前项目：** `PostProcessInput()` 中
`ProcessAbilityInput()` 被注释；`OnRep_PlayerState()` 只刷新 ActorInfo，
不调用 `TryActivateAbilitiesOnSpawn()`。

**Lyra：** 每帧统一消费 Ability 输入，并在客户端 PlayerController 晚复制时
补尝试生成能力。

**影响：** 输入不能从 Controller 侧进入完整的
Input → Ability → Prediction → Server Confirmation（服务器确认）链；
复制顺序补偿也只完成一半。

**建议：** 先完成 Pawn Extension、ASC Avatar 和 AbilitySet（能力集合）授予，
再接回输入消费与生成能力补激活，并用远程客户端验证。

**状态：** **部分复刻**

### 差异二：Camera Assist 只有消费者

**当前项目：** Controller 能消费穿透通知并隐藏图元。

**Lyra：** 第三人称 Camera Mode 负责物理检测、忽略列表、保护目标和回调生产。

**影响：** 当前正常相机路径没有静态可见的触发源。

**建议：** 复刻 Camera Mode 前先确定当前项目是否需要原 Lyra 的瞬时隐藏，
还是改为设计师可调的淡出策略。

**状态：** **部分复刻**

### 差异三：PlayerCameraManager 只保留类型

**当前项目：** 空子类完全沿用 UE 5.7.4 默认值和行为。

**Lyra：** 有 UI Camera、80° FOV、Pitch 覆盖、视图更新优先级和调试显示。

**影响：** UI 接管相机、Lyra 默认视野和相机调试信息不可用。

**建议：** 只有在 UI Camera 与 Camera Component 前置依赖完成后再继续复刻。

**状态：** **结构占位**

### 差异四：客户端 Replay 录制没有最终执行者

**当前项目：** 资格恒为 false，Replay Subsystem 没有录制 API，调用被注释。

**Lyra：** `ULyraReplaySubsystem::RecordClientReplay()` 真正启动录制并处理
本地回放清理。

**影响：** `TryToRecordClientReplay()` 当前不能形成可播放回放；未来派生类
放开资格后还可能产生“返回 true 但未录制”的语义错误。

**建议：** 先补 Subsystem API 和状态验证，再让成功返回值代表引擎确实开始录制。

**状态：** **未复刻**

### 差异五：External RPC 没有实例、路由和清理

**当前项目：** 只有类型、依赖、构建开关和 Listener 启动入口。

**Lyra：** 创建 Root（根引用）单例，解析 JSON，注册作弊、单次开火、
玩家状态和阶段路由，并提供注销。

**影响：** 当前没有 Lyra HTTP 自动化端点。

**建议：** 当前阶段若不需要 Gauntlet（自动化测试框架）外部控制，可保留占位；
若继续复刻，应先定义绑定范围、鉴权、路由白名单和对称注销。

**状态：** **结构占位**

---

## 已识别的 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 当前源码 TODO | 接回 `ProcessAbilityInput()` | `ALyraPlayerController::PostProcessInput()` 注释代码 | ASC 输入缓存、Pawn 输入绑定 |
| 高 | 当前源码 TODO | 接回 `TryActivateAbilitiesOnSpawn()` | `OnRep_PlayerState()` 注释代码与中文说明 | AbilitySet 授予、ASC Avatar 初始化 |
| 高 | Lyra 对比 TODO | 完成 Pawn Extension → ASC Avatar → AbilitySet 主链 | Controller 只清旧 Avatar，不设置新 Avatar | `ULyraPawnExtensionComponent`、`ULyraPawnData` |
| 高 | 验证 TODO | 验证 Dedicated Server 远程玩家的 `ReplicatedViewRotation` 写入来源 | UE `TickActor()` 远程分支绕过 `PlayerTick()` | Dedicated Server、两个客户端、观战入口 |
| 中 | 当前源码 TODO | 恢复 Shared Settings（共享设置）绑定和力反馈初始值 | `SetPlayer()` / `OnSettingsChanged()` 注释 | `ULyraSettingsShared`、LocalPlayer 设置 |
| 中 | Lyra 对比 TODO | 增加 Camera Mode 穿透生产端 | 当前只有接口和隐藏消费者 | Camera Component、碰撞策略 |
| 中 | Lyra 对比 TODO | 实现 Lyra PlayerCameraManager 扩展 | 当前空子类；Lyra 有 UI Camera 与重写 | `ULyraUICameraManagerComponent` |
| 中 | Lyra 对比 TODO | 实现客户端 Replay Subsystem API 和真实录制 | 当前资格恒 false、API 缺失 | 本地设置、Replay 清理策略 |
| 中 | 验证 TODO | 验证 Replay Seek、Checkpoint 与 Pawn 切换后的跟随 | 静态结构已接入但无运行证据 | 可录制或已有 Replay 文件 |
| 中 | 验证 TODO | 验证非发布 Server Cheat 的允许身份和命令范围 | `_Validate()` 恒 true | 联网测试环境、安全约束 |
| 低 | Lyra 对比 TODO | 决定是否继续复刻 External RPC | 当前没有路由；可能暂不需要 | 自动化测试需求与安全设计 |
| 低 | 文档 TODO | 在编辑器中核对 `B_LyraGameMode` 的类覆盖 | 二进制资产无法由当前静态文本确认 | Unreal Editor 资产检查 |

---

## 尚未验证与建议验证方式

| 场景 | 观察点 | 通过标准 |
|---|---|---|
| 构建 | Editor、Development Client/Server、Shipping | 新 UCLASS/UINTERFACE 通过 UHT 与链接；Shipping 功能路径符合宏设计 |
| Possess / UnPossess | ASC OwnerActor、AvatarActor、旧 Pawn | Owner 保持 PlayerState；解除控制前旧 Avatar 被清；新 Pawn 最终成为 Avatar |
| 远程客户端 | `OnRep_PlayerState()` 与 ActorInfo | PlayerState/ASC 先到时，本地控制关系能补刷新 |
| GAS 输入 | Input Tag 按下、保持、释放 | `PostProcessInput()` 每帧消费，预测和服务器确认行为明确 |
| 多人观战 | Dedicated Server、Listen Server、Late Join | 远程玩家旋转由服务器持续更新，非拥有者视角连续 |
| 相机穿透 | 回调、`NoParentAutoHide`、下一帧标志 | 只隐藏预期图元且标志按单帧消费 |
| Replay 播放 | 打开、拖动、Checkpoint、Pawn 更换 | View Target 始终恢复到 Recorder Pawn |
| Replay 录制 | 资格、引擎录制状态、输出文件 | 返回 true 时引擎已进入录制并生成可播放结果 |
| Cheat | Owning Client、Dedicated Server、Shipping | 只在预期非发布环境执行允许命令 |
| External RPC | `rpcport`、路由列表、绑定地址 | 当前没有 Lyra 路由；未来开放后路由和鉴权符合设计 |

---

## 快速回顾

- **一句话职责：** PlayerController 协调本地输入、当前 Pawn、PlayerState、
  ASC、相机与观战，但不保存队伍权威状态。
- **核心入口：** `PostInitializeComponents()`、`Possess()`、
  `OnRep_PlayerState()`、`PlayerTick()`、`UpdateHiddenComponents()`。
- **核心状态：** PlayerState 上的 ASC、Team、PawnData 和
  `ReplicatedViewRotation`；Controller 上的一次性相机隐藏标志。
- **关键依赖：** CommonGame、GameplayAbilities、CommonInput、
  NetworkReplayStreaming、HTTPServer、ExternalRpcRegistry。
- **网络位置：** Server 维护控制权和 PlayerState 权威状态；Owning Client
  处理输入和本地相机；观战状态通过 PlayerState 发给非拥有者。
- **当前完成度：** **部分复刻**。
- **最重要的未完成项：** Pawn Extension / ASC Avatar / AbilitySet /
  GAS 输入主链，以及远程玩家观战旋转验证。

## 复习要点

1. 为什么玩家级 ASC 放在 PlayerState，而 Avatar 指向当前 Pawn？
2. `OnUnPossess()` 为什么必须在 `Super` 前清理匹配的 Avatar？
3. `OnRep_PlayerState()` 当前完成了补刷新，为什么还不能算补激活完成？
4. `DISABLE_REPLICATED_PROPERTY` 对父类 `TargetViewRotation` 做了什么？
5. `COND_SkipOwner` 的目标客户端是谁，晚加入客户端能得到什么？
6. 为什么 `PlayerTick()` 中存在 `HasAuthority()` 仍不能证明所有服务器
   Controller 都会写 `ReplicatedViewRotation`？
7. Camera Assist 当前缺的是生产者还是消费者？
8. Replay 播放跟随和客户端录制为什么必须分开定级？
9. Server Cheat RPC 与 HTTP External RPC 在传输、权限和生命周期上有何不同？

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md)
  — `ULyraReplaySubsystem` 保存回放平台能力边界，当前还没有客户端录制 API。
- [04-Game-Framework.md](04-Game-Framework.md)
  — `ALyraGameMode` 选择普通与 Replay PlayerController 类。
- [05-Player-Framework.md](05-Player-Framework.md)
  — 汇总 Controller、PlayerState、Camera 和出生管理类的长期职责。
- [11-Development-Tools.md](11-Development-Tools.md)
  — 解释 Cheat Manager 与 External RPC 的开发环境边界。
- [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md)
  — 把本篇 Controller 链放回 Experience、PlayerState 和 Pawn 生成时序。
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md)
  — 按来源和优先级汇总本篇识别出的缺口。
- [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md)
  — 查询 UE 5.7.4 生命周期函数的直接调用者和执行时机。
