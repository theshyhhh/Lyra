# 05 - Player 框架

> Player（玩家）框架把本地用户、PlayerController（玩家控制器）、
> PlayerState（玩家状态）、Pawn（受控实体）、Camera（相机）和出生管理
> 连接起来；当前项目已经接入多条协作路径，但玩家 GAS（Gameplay Ability
> System，游戏玩法能力系统）初始化仍不是完整流程。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 `ULyraLocalPlayer`、Settings（设置）、GameInstance（游戏实例）
> 和 Experience（体验）改动。当前项目与 Lyra 参考项目均声明 UE 5.7，底层
> 机制按 Unreal Engine 5.7.4 源码核对；未执行编译、PIE、登录或多人运行验证。

---

## 框架概述

**问题：** 玩家对象分布在不同生命周期和网络位置：LocalPlayer（本地玩家）
只存在于本机，PlayerController 只在服务器和拥有者客户端完整存在，
PlayerState 需要向其他客户端复制，而 Pawn 会随死亡、重生和观战切换。

**当前项目的解决方案：** PlayerState 保存可持续的 ASC、PawnData、Team /
Squad（队伍 / 小队）、StatTag（统计标签）和观战旋转；PlayerController
协调本地输入、当前 Pawn、队伍代理和相机；LocalPlayer 缓存本地 / 共享设置，
并继续转发 Controller 的队伍变化；出生点选择由 GameState 上的
`ULyraPlayerSpawningManagerComponent` 统一管理。

**设计意图：**

- 通过 CommonUI / CommonGame 基类连接本地用户、输入和 UI。
- 让 ASC 的 OwnerActor（拥有者 Actor）保持为 PlayerState，而
  AvatarActor（化身 Actor）随 Pawn 切换。
- 让 PlayerController 只转发 Team 状态，不直接改写服务器权威 Team ID。
- 让 LocalPlayer 在 Controller 更换时迁移 Team Delegate（队伍委托），并在
  跨 World 生命周期中缓存每用户设置。
- 将出生选择从 GameMode 拆到可由 Experience / GameFeature 注入的组件。

**当前完成度：** PlayerState 数据与复制、出生选择和 Controller 多数协调
钩子已经存在；Pawn Extension（Pawn 扩展）、AbilitySet（能力集合）授予、
GAS 输入、相机穿透生产端和客户端 Replay（回放）录制仍未完成；共享设置的
同步外壳和登录入口已经存在，但异步加载与实际字段 / 应用仍未完成。

---

## 当前复刻状态

| 模块 | 当前状态 | 当前实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| LocalPlayer（本地玩家） | **部分复刻** | 已转发 Controller Team、缓存 Local / Shared Settings、监听音频设备并响应登录加载入口 | Lyra 的对应桥梁基本相同，但下游设置类具备完整实现 | Team 桥梁已接通；异步设置端点和音频事件生产者仍缺失 |
| PlayerController 协调 | **部分复刻** | 控制切换、PlayerState 补刷新、Team 代理、自动奔跑、观战、相机隐藏消费者已实现 | Lyra 还接入完整 GAS 输入、生成能力和设置 | 玩家主链仍在 ASC 输入与 Pawn 初始化处断开 |
| Replay Controller（回放控制器） | **部分复刻** | 能重新跟随 Recorder PlayerState 的 Pawn | Lyra 同时有可用的客户端录制 Subsystem | 播放跟随需运行验证，客户端录制不可用 |
| Camera Assist（相机辅助） | **部分复刻** | 接口和隐藏消费者存在 | 第三人称 Camera Mode 产生穿透通知 | 正常游戏中缺少静态可见触发者 |
| PlayerCameraManager（玩家相机管理器） | **结构占位** | 空子类继承 UE 基础相机 | Lyra 有 UI Camera、80° FOV 和调试重写 | Lyra 相机专属能力未接入 |
| PlayerState（玩家状态） | **部分复刻** | ASC、PawnData、Team/Squad、StatTag 和观战旋转已复制 | Lyra 还授予 AbilitySet 并完成 Pawn Extension 协作 | 数据宿主已存在，能力与 Pawn 初始化不完整 |
| 出生点与重生 | **部分复刻** | 缓存、占用检测、Claim（占用声明）和 GameMode 代理已接入 | Lyra 可按 Experience 扩展队伍和模式规则 | 当前 `ControllerCanRestart()` 仍总是允许 |

---

## 类列表

| 类 | 父类或接口 | 生命周期 | 网络位置 | 当前状态 | 职责 |
|---|---|---|---|---|---|
| `ULyraLocalPlayer` | `UCommonLocalPlayer`、`ILyraTeamAgentInterface` | Runtime，跨 World 本地对象 | 仅本机，不复制 | **部分复刻** | 转发 Team、缓存设置、响应登录并切换音频设备 |
| `ALyraPlayerController` | `ACommonPlayerController`、两个 Lyra 接口 | Runtime | Server 与 Owning Client；复制 Actor | **部分复刻** | 协调输入、Pawn、PlayerState、队伍、相机和观战 |
| `ALyraReplayPlayerController` | `ALyraPlayerController` | Runtime / Replay | Replay World | **部分复刻** | 回放播放期间恢复录制者 Pawn 跟随 |
| `ULyraCameraAssistInterface` / `ILyraCameraAssistInterface` | `UInterface` / C++ 接口 | Runtime | 本地调用，不自行复制 | **部分复刻** | 定义相机穿透协作协议 |
| `ALyraPlayerCameraManager` | `APlayerCameraManager` | Runtime，瞬态 Actor | Server 与 Owning Client 本地存在，不复制 | **结构占位** | 当前只继承 UE 基础相机行为 |
| `ALyraPlayerState` | `AModularPlayerState`、`IAbilitySystemInterface`、`ILyraTeamAgentInterface` | Runtime 🧩 | Server Authority，复制到相关客户端 | **部分复刻** | 保存玩家级 ASC 和持续状态 |
| `ALyraPlayerStart` | `APlayerStart` | Runtime | 服务器游戏规则主用 | **部分复刻** | 出生点占用检测与短期 Claim |
| `ULyraPlayerSpawningManagerComponent` | `UGameStateComponent` | Runtime | 服务器选择出生点，不复制自定义状态 | **部分复刻** | 缓存、选择出生点并提供重生扩展点 |

> 🧩 = 使用 Modular 基类，GameFeature 可注入组件

---

## 核心数据流

### PlayerController、PlayerState、ASC 与 Pawn

```text
Experience Loaded（Experience 加载完成）
  |-- ALyraPlayerState::SetPawnData()
  |     `-- [当前注释] AbilitySet 授予
  `-- ALyraGameMode::RestartPlayer()
        |-- 选择 ALyraPlayerStart
        |-- 根据 PawnData 选择 PawnClass
        |-- Deferred Spawn（延迟完成生成）
        `-- [当前 TODO] PawnExtension 初始化

Server AController::Possess(Pawn)
  `-- ALyraPlayerController::OnPossess()
        `-- 清除 Status_AutoRunning

Server AController::UnPossess()
  `-- ALyraPlayerController::OnUnPossess()
        |-- ASC.Avatar == 旧 Pawn 时先清 Avatar
        `-- Super 清除 Pawn 控制关系

Owning Client 收到 PlayerState
  `-- ALyraPlayerController::OnRep_PlayerState()
        |-- 迁移 Team Delegate
        |-- ASC::RefreshAbilityActorInfo()
        `-- [当前注释] TryActivateAbilitiesOnSpawn()

Server PlayerState Team 变化
  `-- 属性复制到 Owning Client
        `-- ALyraPlayerState::OnRep_MyTeamID()
              `-- PlayerState Team Delegate
                    `-- ALyraPlayerController::OnPlayerStateChangedTeam()
                          `-- Controller Team Delegate
                                `-- ULyraLocalPlayer::OnControllerChangedTeam()
                                      `-- LocalPlayer Team Delegate
```

这条主流程包含同步的服务器控制切换和异步到达的客户端属性复制。失败时，
`SetPawnData()` 对重复设置记录错误，`Possess()` 拒绝非权威调用；
PlayerState 或 ASC 暂时无效时，Controller 的补刷新路径安全跳过。

### 出生点选择

```text
ALyraGameMode::ChoosePlayerStart_Implementation()
  `-- ULyraPlayerSpawningManagerComponent::ChoosePlayerStart()
        |-- 清理失效弱引用
        |-- Spectator（旁观者）随机选择且不 Claim
        |-- 子类 OnChoosePlayerStart() 扩展
        |-- Empty 优先，Partial 次选
        `-- ALyraPlayerStart::TryClaim()
              `-- 定时检查 Pawn 离开后释放 Claim
```

---

## 逐类详解

### ULyraLocalPlayer [Runtime]

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：`Source/LyraGame/Player/LyraLocalPlayer.h/.cpp`。
- Lyra 参考：`Source/LyraGame/Player/LyraLocalPlayer.h/.cpp`。
- 引擎父类：`Engine/Source/Runtime/Engine/Classes/Engine/LocalPlayer.h`、
  `Engine/Source/Runtime/Engine/Private/LocalPlayer.cpp`。

**继承链:**

`UObject → UPlayer → ULocalPlayer → UCommonLocalPlayer → ULyraLocalPlayer`

**UCLASS:** `UCLASS(MinimalAPI)`

**实现接口：** `ILyraTeamAgentInterface`

**职责:** 本地玩家的引擎侧表示。当前对象把三类仅本机职责连接起来：

- 观察当前 `PlayerController` 的 Team（队伍），并向 LocalPlayer 监听者转发。
- 获取机器级 `ULyraSettingsLocal`，缓存每用户 `ULyraSettingsShared`。
- 监听音频输出设备事件，并把设备 ID 交给 Audio Mixer（音频混合器）。

**创建位置与所有权：** `DefaultEngine.ini` 把 `LocalPlayerClassName` 配置为
`ULyraLocalPlayer`，GameInstance 按本地用户创建并管理它。它跨 World 存在但
不复制；`SharedSettings` 是 `UPROPERTY(Transient)` 强引用，`LastBoundPC` 是
弱引用，避免仅为观察 Controller 而延长 Actor 生命周期。

**初始化与 Controller 切换：**

| 方法 | 调用时机 | 当前行为 |
|---|---|---|
| `PostInitProperties()` | C++ 构造和 Config 属性初始化后、序列化前，由 UObject 创建链调用一次 | 调用 `Super` 后，以 `AddUObject` 监听音频设备事件 |
| `SpawnPlayActor()` | `ULocalPlayer` 创建并关联 PlayerController 时 | 父类完成生成后迁移 Team 监听 |
| `SwitchController()` | 引擎更换当前 `UPlayer` 绑定的 Controller 时 | 父类完成绑定后迁移 Team 监听 |
| `InitOnlineSession()` | `APlayerController::SetPlayer()` 关联 LocalPlayer 后 | 先同步 Team 监听，再调用父类在线会话初始化 |

> 🧩 **引擎机制：** `PostInitProperties()` 不是 BeginPlay；此时对象已经经过
> 构造 / Config 属性初始化，但可能还未加载序列化数据。`AddUObject` 会在
> UObject 失效后跳过回调，不会把 LocalPlayer 变成永久根对象。

**Team 转发：** `SetGenericTeamId()` 有意不写状态，因为 LocalPlayer 只是观察者；
`GetGenericTeamId()` 转发到实现 Team 接口的当前 Controller。每次 Controller
变化时，代码从旧 Controller `RemoveAll(this)`，向新 Controller 注册动态委托，
然后比较并广播旧 / 新 Team。LocalPlayer 自身不复制，网络来源仍是复制到本机
的 PlayerState。

**设置入口：**

- `GetLocalSettings()` 返回引擎持有的 `ULyraSettingsLocal::Get()`。
- `GetSharedSettings()` 首次访问时，桌面平台同步调用
  `LoadOrCreateSettings()`；其他平台创建临时对象，随后等待登录后替换。
- `LoadSharedSettingsFromDisk()` 由 GameInstance 的用户登录回调调用；相同
  Unique Net ID 已加载时跳过，否则请求异步加载。
- `OnSharedSettingsLoaded()` 设计为替换缓存并记录 Unique Net ID；但当前
  `AsyncLoadOrCreateSettings()` 直接返回 `false`，所以这条登录链不会到达回调。

**音频输出：** `OnAudioOutputDeviceChanged()` 调用
`UAudioMixerBlueprintLibrary::SwapAudioOutputDevice()`；完成回调只识别 Failure
（失败），当前失败分支为空。`ULyraSettingsLocal` 尚无 Setter（设置函数）或
其他 C++ 广播者，因此只能确认消费者存在，不能确认运行时会触发。

**清理流程：** Controller 更换时显式解绑旧 Team 委托；音频事件使用
`AddUObject` 的 UObject 生命周期保护。当前没有自定义 `BeginDestroy()` 或
LocalPlayer 移除钩子，也没有进行异步请求，因此尚无异步句柄可清理。

**当前限制：** LocalPlayer 桥梁与 Lyra 基本同形，但完整行为依赖尚未复刻的
Settings Shared（共享设置）字段、异步加载、应用 / 保存，以及 Local Settings
（本地设置）的事件生产者。完整设置流程见
[08-UI-Framework.md](08-UI-Framework.md#核心数据流)。

---

### ALyraPlayerController [Runtime]

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：`Source/LyraGame/Player/LyraPlayerController.h/.cpp`。
- Lyra 参考：`Source/LyraGame/Player/LyraPlayerController.h/.cpp`。
- 引擎父类：`Engine/Source/Runtime/Engine/Private/Controller.cpp`、
  `PlayerController.cpp`。

**继承链:** `AActor → AController → APlayerController → ACommonPlayerController → ALyraPlayerController`

**UCLASS:**
`UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "该项目使用的PlayerController基类"))`

**职责:** 基础玩家控制器（PlayerController）。它把“本地输入与相机”
“可复制的玩家状态（PlayerState）”和“当前 Pawn”连接起来；提交 `19e6961` 还让
它承担 Team 转发、Auto-run（自动奔跑）和观战旋转同步的部分职责。

**创建位置与所有权：** `ALyraGameMode` 的 C++ 构造函数设置
`PlayerControllerClass`。服务器创建权威 Controller，拥有者客户端接收
自己的 Controller；其他客户端通常通过该玩家的 PlayerState 和 Pawn 观察
状态。`B_LyraGameMode` 蓝图资产可以覆盖 C++ 默认类，最终配置仍需资产验证。

**类型化访问器:**

- `GetLyraPlayerState()` — 将 `PlayerState` 转换为 `ALyraPlayerState*`
- `GetLyraAbilitySystemComponent()` — 从 `ALyraPlayerState` 取得玩家级能力系统组件（Ability System Component，ASC）
- `GetLyraHUD()` — 将 `MyHUD` 转换为 `ALyraHUD*`

**当前接口状态（提交 `19e6961`，未运行验证）:**

- 已实现 `ILyraTeamAgentInterface`：`GetGenericTeamId()` 从 `PlayerState` 读取队伍编号；`SetGenericTeamId()` 明确拒绝写入；`BroadcastOnPlayerStateChanged()` 负责迁移队伍变化委托（Delegate）。因此队伍数据的权威来源仍是 `ALyraPlayerState`，控制器只是面向控制器使用者的代理。
- 已实现 `ILyraCameraAssistInterface`：`OnCameraPenetratingTarget()` 会把一次性隐藏标志设为 `true`，`UpdateHiddenComponents()` 在下一次视图构建时隐藏当前 View Target（视图目标）及其直接附加的 Primitive Component（图元组件），然后消费并重置该标志。
- 当前项目没有 `LyraCameraMode_ThirdPerson` 等调用 `OnCameraPenetratingTarget()` 的相机碰撞生产端，所以隐藏消费者虽然已写入，正常游戏中仍没有静态可见的触发来源。
- 构造函数设置 `PlayerCameraManagerClass = ALyraPlayerCameraManager::StaticClass()`。当前自定义类没有 Lyra 专属成员或重写，应按“结构占位”判断；可用的 View Target、镜头缓存和默认 90° FOV 来自 `APlayerCameraManager` 父类。

**当前关键调用点（提交 `19e6961`）:**

| 引擎钩子 | 当前代码行为 | 理解时的重点 |
|------|------|------|
| 构造函数 / `PostInitializeComponents()` | 选择 `ALyraPlayerCameraManager` 和非发布构建功能路径中的 `ULyraCheatManager`；引擎稍后生成相机管理器并尝试添加 Cheat Manager（作弊管理器） | 选择子类不等于 Lyra 专属行为已实现；两个子类都属于结构占位 |
| `BeginPlay()` | 非发布构建调用 `StartAllListeners()`、解析 `rpcport`，但注册对象和路由调用被注释；最后取消隐藏 Controller Actor（控制器 Actor） | 启动已有 HTTP Listener（监听器）不等于已经创建路由或端点 |
| `GetLifetimeReplicatedProps()` | 禁用父类 `TargetViewRotation` 的 `COND_OwnerOnly` 复制 | 观战旋转改由 `PlayerState::ReplicatedViewRotation` 承载，并以 `COND_SkipOwner` 复制给非拥有者 |
| `OnPossess()` | 先调用 `Super`，编辑器服务器条件下执行配置的作弊命令，随后关闭自动奔跑 | `Super` 完成 Pawn 所有权与控制关系；其后才能以 `GetPawn()` 作为已控制角色读取 |
| `OnUnPossess()` | 在 `Super` 之前，若 ASC 的 Avatar（化身）正是即将解除控制的 Pawn，则将 Avatar 清空 | ASC 的 OwnerActor（拥有者）仍是 PlayerState，AvatarActor（化身）才是会切换的 Pawn；两者不能混为一谈 |
| `OnRep_PlayerState()` | 广播 PlayerState 改变；Owning Client（拥有者客户端）刷新 ASC 的 ActorInfo | `TryActivateAbilitiesOnSpawn()` 仍被注释，因此这里只是补刷新，不是完整的生成能力补激活 |
| `PlayerTick()` | 自动奔跑时添加前向移动输入；同步或消费 `ReplicatedViewRotation` | UE 5.7.4 的服务器远程 Controller 会走 `TickActor()` 的专用分支并绕过 `PlayerTick()`；`HasAuthority()` 不等于所有服务器 Controller 都会执行这里 |
| `UpdateHiddenComponents()` | 一次性把 View Target 的已注册图元及未标记 `NoParentAutoHide` 的直接附加图元加入隐藏集合 | UE 5.7.4 在 `ULocalPlayer::CalcSceneView()` 构建每个视图时调用该链；当前缺少相机碰撞触发者 |
| `PostProcessInput()` | 取得 ASC，但 `ProcessAbilityInput()` 调用仍被注释，之后调用 `Super` | 说明 GAS 输入分发尚未完成；自动奔跑标签不等同于技能输入链完整 |

> 详细的引擎入口、Lyra 原项目差异、状态变化和验证清单见 [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)。

**子类:** `ALyraReplayPlayerController` 已实现 `Tick()`、
`SmoothTargetViewRotation()`、`ShouldRecordClientReplay()` 和录制者 Pawn
变化回调。它会在回放拖动或检查点恢复导致旧 PlayerState 失效后，重新绑定
`ALyraGameState::OnRecorderPlayerStateChangedEvent`，再把 View Target
切到录制者的新 Pawn；状态为**部分复刻**，因为尚无运行验证。客户端录制则
单独定为**未复刻**：基础控制器的资格检查恒为 false，真正的
`RecordClientReplay()` API 和调用都不存在。

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

**当前状态：** **部分复刻**

**组成:** `ULyraCameraAssistInterface` 提供 Unreal Reflection（虚幻反射）类型，`ILyraCameraAssistInterface` 提供 C++ 协议。

**当前默认行为:** `GetIgnoredActorsForCameraPenetration()` 不追加 Actor，`GetCameraPreventPenetrationTarget()` 返回未设置的 `TOptional`，`OnCameraPenetratingTarget()` 为空。`ALyraPlayerController` 只重写最后一个回调。

**与 Lyra 原项目的差异:** 原项目第三人称相机模式在穿透检测命中时，把 Controller（控制器）、View Target（视图目标）和防穿透目标转换为该接口并调用回调。当前项目没有对应 Camera Mode（相机模式），所以接口只能视为“消费者已接入、生产者缺失”。

---

### ALyraPlayerCameraManager [Runtime]

**当前状态：** **结构占位**

**继承链:** `AActor → APlayerCameraManager → ALyraPlayerCameraManager`

**当前行为:** 类体没有自定义字段或重写，`.cpp` 只包含头文件。UE 5.7.4 父类仍负责 View Target（视图目标）、镜头缓存、混合、旋转限制和相机更新，默认 FOV 为 90°、Pitch（俯仰）范围约为 -89.9° 到 89.9°。

**Lyra 原设计:** 构造时改为 80° FOV 和 -89° 到 89° Pitch，创建 `ULyraUICameraManagerComponent`，并重写 `UpdateViewTarget()` 与 `DisplayDebug()`。这些 Lyra 专属能力尚未进入当前项目。

---

### ALyraPlayerState [Runtime] 🧩

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：`Source/LyraGame/Player/LyraPlayerState.h/.cpp`。
- Lyra 参考：`Source/LyraGame/Player/LyraPlayerState.h/.cpp`。
- 引擎父类：`Engine/Source/Runtime/Engine/Private/PlayerState.cpp`。
- GAS 引擎机制：
  `GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp`。

**继承链:** `AActor → AInfo → APlayerState → AModularPlayerState → ALyraPlayerState`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 可复制的玩家状态数据。当前已经接入玩家级 `ULyraAbilitySystemComponent`、复制的 `PawnData`、连接类型、Team/Squad、StatTag Stack、观战视角旋转，以及面向客户端的 gameplay message 转发。

**创建位置与所有权：** `ALyraGameMode` 把 `PlayerStateClass` 设为该类，
UE 在服务器为玩家创建 PlayerState 并把 Controller 设为 Owner；PlayerState
随后复制到相关客户端。ASC 是 PlayerState 的 Default Subobject（默认子对象），
由 `UPROPERTY` 持有并随 PlayerState 生命周期存在。

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

> 🌐 **网络：**
> `GetLifetimeReplicatedProps()` 只注册复制规则，不是主动发送函数。
> Team、PawnData 和观战旋转应由服务器写入；拥有者客户端跳过自己的
> `ReplicatedViewRotation`。晚加入客户端可在初始复制时取得服务器当前值，
> 但服务器远程 Controller 是否持续更新该值仍需运行验证。

**Team Agent 接口:**
`ULyraTeamAgentInterface` 继承自 `UGenericTeamAgentInterface`，禁止蓝图实现。辅助函数 `GenericTeamIdToInteger()` / `IntegerToGenericTeamId()` 在 `FGenericTeamId::NoTeam` 和 `INDEX_NONE` 之间转换；`ConditionalBroadcastTeamChanged()` 只有在队伍实际变化时打印 `LogLyraTeams` 并广播委托。

**StatTag Stack:**
`FGameplayTagStackContainer` 使用 `FFastArraySerializer` 复制 `FGameplayTagStack` 数组，并维护本地 `TagToCountMap` 加速查询。`AddStack()` / `RemoveStack()` 只接受有效 GameplayTag，数量小于 1 时不改变状态；客户端通过 FastArray 的 `PreReplicatedRemove()`、`PostReplicatedAdd()`、`PostReplicatedChange()` 同步查询 Map。

**Modular 基类的好处:**
`AModularPlayerState` 注册到 `UGameFrameworkComponentManager`。GameFeature Action 仍可以在 Experience 加载期间向 PlayerState 添加额外组件；当前 ASC 已作为默认子对象直接存在，后续更适合把 AbilitySet 授予、初始化状态和 PawnExtension 协作接到这条链上。

---

### ALyraPlayerStart [Runtime]

**当前状态：** **部分复刻**

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

**当前状态：** **部分复刻**

**继承链:** `UObject → UActorComponent → UGameStateComponent → ULyraPlayerSpawningManagerComponent`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 出生点管理组件。它设计为挂在 GameState 上，由 Experience/GameFeature 按需注入，再由 `ALyraGameMode` 代理调用，从而让不同 Experience 能替换出生规则。

**创建位置与所有权：** 当前 C++ 不在 GameState 构造函数中固定创建它，
而是通过 Experience / GameFeature 的组件注入机制挂到 GameState。Owner
为 GameState，组件随 Owner 和 World 生命周期销毁；具体资产是否已经注入
该组件仍需资产验证。

**初始化行为 (`InitializeComponent()`):**
1. 绑定 `FWorldDelegates::LevelAddedToWorld`，监听流式关卡加入。
2. 绑定 `UWorld::AddOnActorSpawnedHandler()`，监听运行时生成的 `ALyraPlayerStart`。
3. 遍历当前 World 中已有的 `ALyraPlayerStart`，缓存到 `CachedPlayerStarts`。

**清理流程：** 当前类没有重写 `EndPlay()` 或
`UninitializeComponent()`，也没有保存 `AddOnActorSpawnedHandler()` 返回的
句柄。`AddUObject` / UObject 感知委托可以避免对象销毁后继续执行回调，
但“组件在 World 仍存活时被动态移除”是否需要显式解绑仍应验证。

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

## 网络与权限

**网络问题一句话定位：** 玩家控制权、PawnData、Team/Squad、StatTag 和
可复制观战旋转由服务器权威产生；PlayerState 通过属性复制把持续状态发送给
相关客户端，PlayerController 只在拥有者本地处理输入、相机和预测表现。

| 状态或事件 | 修改端 | 复制方式 | 接收端与表现 |
|---|---|---|---|
| Pawn 控制关系 | Server Authority | UE Controller / Pawn 原生复制 | Owning Client 恢复本地控制；其他客户端观察 Pawn |
| `PawnData`、连接类型、Team/Squad | Server | Push Model 属性复制 | 客户端更新持续状态，Team 通过 RepNotify 广播 |
| `StatTags` | Server 设计路径 | `FFastArraySerializer` 增量复制 | 客户端回调维护 `TagToCountMap` |
| `ReplicatedViewRotation` | 设计上应由 Server 更新 | Push Model + `COND_SkipOwner` | 非拥有者观战表现；服务器写入来源仍需验证 |
| `ClientBroadcastMessage()` | Server 调用 Client RPC | Unreliable Client RPC（不可靠客户端远程调用） | 目标客户端转发到 Gameplay Message Subsystem（游戏玩法消息子系统） |
| Auto-run | 本地 Controller 通过 ASC Loose Tag 保存 | 没有独立持久复制规则 | 每帧转成 Pawn 移动输入，最终移动仍由 UE 移动复制处理 |
| LocalPlayer Team | 不直接修改；观察 Controller | LocalPlayer 本身不复制 | 复制到本机的 PlayerState 先驱动 Controller，再由 LocalPlayer 转发 |
| Local / Shared Settings | 本地用户或设置 UI | 本地 Config / SaveGame，不复制 | 每个 LocalPlayer 独立缓存和消费 |

**服务端验证：** `Possess()` 由引擎拒绝非 Authority 调用；
`SetPawnData()` 和 `SetGenericTeamId()` 有权威检查。`AddStatTagStack()`
虽然标记 `BlueprintAuthorityOnly`，但 C++ 函数体没有额外 Authority
检查，调用方仍需遵守服务器写入约定。

**晚加入与延迟生成：** PlayerState 的持续属性可通过 Initial Replication
（初始复制）把服务器当前值交给晚加入客户端；一次性 RPC 不应代替这些持续
状态。PlayerState 和 ASC 先于 PlayerController 到达时，
`OnRep_PlayerState()` 会补刷新 ActorInfo，但生成能力补激活仍缺失。

> 🧪 **待验证：**
> Dedicated Server 上远程 Controller 不进入当前 `PlayerTick()`，
> `ReplicatedViewRotation` 是否有其他权威写入来源，需要两客户端观战与
> Late Join（中途加入）测试。

---

## 资产、配置与模块依赖

| 来源 | 当前项目配置 | 边界 |
|---|---|---|
| `ALyraGameMode` 构造函数 | 选择 Lyra Controller、Replay Controller、PlayerState 和 Character | C++ 默认关系已建立 |
| `Config/DefaultEngine.ini` | `GlobalDefaultGameMode` 指向 `B_LyraGameMode` | 蓝图可覆盖 C++ 默认类，需资产验证 |
| `Config/DefaultEngine.ini` | `LocalPlayerClassName` 指向 `ULyraLocalPlayer`；`GameUserSettingsClassName` 指向 `ULyraSettingsLocal` | 引擎创建自定义本地玩家和机器级设置 |
| `Content/B_LyraGameMode.uasset` | 资产存在 | 当前未可靠解析其 Class Override |
| Experience / GameFeature | 计划向 GameState 注入 `ULyraPlayerSpawningManagerComponent` | 具体 Experience 资产是否已注入仍需确认 |
| Camera 源码与资产名搜索 | 没有 `LyraCameraMode_ThirdPerson` C++ 调用者，也未发现明确命名资产 | 只能确认代码侧生产端缺失，不能排除蓝图间接调用 |
| `ULyraDeveloperSettings` | 力反馈设置绑定 `LyraPC.ShouldAlwaysPlayForceFeedback` | 可在 Project Settings 中改变本地过滤 |
| 模块依赖 | CommonGame、CommonUser、CommonUI、CommonInput、GameplayAbilities、ModularGameplayActors、AudioMixer、NetworkReplayStreaming | 依赖存在不代表关联功能已经闭合 |

---

## 与 Lyra 的差异

### 差异：玩家 GAS 初始化链未闭合

**当前项目：** PlayerState 已持有 Mixed Replication Mode（混合复制模式）的
ASC，但 AbilitySet 授予、Pawn Extension 初始化、`ProcessAbilityInput()`
和 `TryActivateAbilitiesOnSpawn()` 仍被注释或未实现。

**Lyra：** PawnData / Experience 决定 AbilitySet，Pawn Extension 负责
Owner / Avatar 协作，Controller 每帧消费 Ability 输入，并处理客户端复制
顺序补偿。

**影响：** 当前只能确认 ASC 数据宿主和部分 ActorInfo 生命周期，不能确认
Input → Ability → Effect（效果）→ Attribute（属性）→ GameplayCue
（游戏玩法提示）完整链。

**状态：** **部分复刻**

### 差异：相机只接入隐藏消费者和空 Manager

**当前项目：** Camera Assist 回调和单帧隐藏存在；
`ALyraPlayerCameraManager` 没有自定义实现。

**Lyra：** 第三人称 Camera Mode 产生穿透事件，PlayerCameraManager 还提供
UI Camera、FOV / Pitch 默认值和调试显示。

**影响：** 当前正常相机路径没有静态可见的穿透触发者，也没有 Lyra UI 相机
接管能力。

**状态：** Camera Assist 为**部分复刻**；PlayerCameraManager 为
**结构占位**。

### 差异：Replay 播放跟随与客户端录制完成度不同

**当前项目：** 播放侧已有 Recorder PlayerState / Pawn 跟随；录制侧资格
恒为 false，Replay Subsystem 没有 `RecordClientReplay()`。

**Lyra：** 播放和客户端录制两条路径都存在。

**影响：** 不能用“Replay Controller 已接入”推导“自动客户端录制可用”。

**状态：** 播放跟随为**部分复刻**；客户端录制为**未复刻**。

### 差异：LocalPlayer 桥梁已接入，但设置实现未闭合

**当前项目：** LocalPlayer 的 Team 迁移、设置缓存、登录入口和音频设备消费
已经存在；Shared Settings 的异步方法直接返回 `false`，Local Settings 的
Experience 回调为空，音频事件没有生产者。

**Lyra：** Shared Settings 会异步加载、应用并回传，包含输入、字幕、文化、
力反馈等字段；Local Settings 在 Experience 完成时重应用设备配置相关设置。

**影响：** Team 监听链可以静态确认闭合；设置相关函数名虽然已接入，仍不能
推导真实用户偏好已从磁盘载入或应用。

**状态：** LocalPlayer 为**部分复刻**；其设置依赖分别为**部分复刻**或
**结构占位**。

### 暂不直接复刻的内容

External RPC、完整 Cheat 命令和 ShooterCore（射击玩法核心）专属能力并非
当前 Player 主链的前置条件。若当前学习目标先完成 Pawn / ASC / Input，
这些内容可以保持低优先级；影响是自动化测试和完整 Lyra 调试能力暂不可用。

---

## 已识别的 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 当前源码 TODO | 完成 Pawn Extension 与新 Pawn ASC Avatar 初始化 | Controller 只清理旧 Avatar；Spawn 路径留有 TODO | `ULyraPawnExtensionComponent`、PawnData |
| 高 | 当前源码 TODO | 恢复 AbilitySet 授予 | `ALyraPlayerState::SetPawnData()` 注释代码 | `ULyraAbilitySet` 与 PawnData 数据 |
| 高 | 当前源码 TODO | 恢复 `ProcessAbilityInput()` 和生成能力补激活 | Controller 两处注释 | ASC 输入缓存、Ability 激活策略 |
| 高 | 验证 TODO | 验证远程玩家 `ReplicatedViewRotation` 的服务器写入 | UE 5.7.4 `TickActor()` 调用链疑点 | Dedicated Server、观战入口 |
| 高 | 当前源码 TODO | 实现 Shared Settings 异步加载、应用和完成回调 | 当前包装函数直接返回 `false` | `ULocalPlayerSaveGame` 异步 API |
| 中 | 验证 TODO | 验证 Controller 更换、重新登录和分屏时 LocalPlayer Team / Settings 缓存 | 静态链已存在，未运行验证 | 两个本地用户与多人 PIE |
| 中 | Lyra 对比 TODO | 接入 Camera Mode 穿透生产端 | 当前只有消费者 | 相机组件与碰撞策略 |
| 中 | Lyra 对比 TODO | 实现 UI Camera 和 PlayerCameraManager 重写 | 当前空子类 | `ULyraUICameraManagerComponent` |
| 中 | 验证 TODO | 验证 Replay Seek / Checkpoint / Pawn 更换 | 代码存在但无运行证据 | 可用 Replay 文件 |
| 中 | Lyra 对比 TODO | 实现客户端 Replay 录制 API | 当前 Subsystem 只有平台能力判断 | 本地设置、回放清理策略 |
| 中 | 验证 TODO | 确认出生管理组件在目标 Experience 中已注入 | 当前只确认 C++ 类型和 GameMode 查找 | Experience / GameFeature 资产 |
| 低 | 文档 TODO | 核对 `B_LyraGameMode` 是否覆盖 C++ 类 | 二进制资产边界 | Unreal Editor 资产检查 |

---

## 快速回顾

- **一句话职责：** Player 框架把本地用户、Controller、PlayerState、Pawn、
  相机和出生规则连接起来。
- **核心入口：** 用户初始化、Controller 切换、Experience Loaded、
  `RestartPlayer()`、`Possess()`、`OnRep_PlayerState()`、`PlayerTick()`。
- **核心状态：** PlayerState 上的 ASC、PawnData、Team/Squad、StatTag 和
  观战旋转，以及 LocalPlayer 的 Team 委托和 Shared Settings 缓存。
- **网络位置：** Server 维护权威状态；Owning Client 处理输入与本地相机；
  其他客户端通过 PlayerState / Pawn 观察。
- **当前完成度：** **部分复刻**。
- **最重要的未完成项：** Pawn Extension → ASC Avatar → AbilitySet →
  Ability Input 主链，以及 Shared Settings 异步加载 / 应用链。

## 复习要点

1. LocalPlayer、PlayerController、PlayerState 和 Pawn 分别在哪些网络端存在？
2. 为什么 ASC 的 OwnerActor 适合放在 PlayerState，AvatarActor 指向 Pawn？
3. `OnRep_PlayerState()` 解决了哪种复制到达顺序问题？
4. Team ID 为什么不能从 PlayerController 直接设置？
5. `COND_SkipOwner` 如何影响观战旋转和晚加入客户端？
6. Camera Assist 当前缺少生产者还是消费者？
7. 出生管理组件为何放在 GameState，而不是把规则固定在 GameMode？
8. 当前项目与 Lyra 最大差异为什么集中在 Pawn / GAS 初始化链？
9. LocalPlayer 为什么只观察 Controller Team，而不直接拥有权威 Team ID？
10. 登录设置链目前在哪个函数停止，停止后哪些状态不会更新？

---

## 框架内部关系

```
ULyraLocalPlayer
  |-- 由 ULyraGameInstance 管理（1 个本地玩家对应当前 Controller）
  |-- ILyraTeamAgentInterface → 转发 Controller Team
  |-- SharedSettings → 缓存每用户 SaveGame
  `-- LocalSettings 音频事件 → Audio Mixer 设备切换

ALyraPlayerController
  ├── PlayerState → ALyraPlayerState
  │     └── ASC OwnerActor = PlayerState，AvatarActor = 当前 Pawn
  ├── Pawn → ALyraCharacter
  ├── PlayerCameraManager → ALyraPlayerCameraManager [结构占位]
  ├── Team Agent → 转发 PlayerState Team 状态
  └── MyHUD → ALyraHUD

ALyraReplayPlayerController
  └── RecorderPlayerState → OnPawnSet → SetViewTarget()

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

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 管理 ULyraLocalPlayer，并把 CommonUser 登录结果交给它
- [06-Character-Framework.md](06-Character-Framework.md) — PlayerController 控制的 Pawn 通常是 ALyraCharacter
- [08-UI-Framework.md](08-UI-Framework.md) — PlayerController 持有 HUD；LocalPlayer 连接本地/共享设置和音频设备
- [09-GameplayTags-System.md](09-GameplayTags-System.md) — StatTag Stack 使用 GameplayTag 表达玩家统计状态
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — 按当前源码、Lyra 对比和验证来源汇总 Player / GAS 缺口
- [04-Game-Framework.md](04-Game-Framework.md) — ALyraGameMode 代理出生/重生流程，ALyraWorldSettings 在 Map Check 中要求使用 ALyraPlayerStart
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — 逐节点核对提交 `19e6961` 中的 Controller、Camera、Replay 和网络调用链
