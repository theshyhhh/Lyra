# 05 - Player 框架

> 本地玩家、玩家控制器、玩家状态、出生点和重生管理。这些类管理玩家从登录、生成到游戏中操作的全流程。

---

## 框架概述

Player 框架涵盖所有代表"玩家"的对象层次：
- `ULyraLocalPlayer` — 本地玩家的引擎级表示（UI 绑定等）
- `ALyraPlayerController` — 控制 Pawn 的控制器（输入、相机等）
- `ALyraPlayerState` — 复制到所有客户端的玩家状态数据
- `ALyraPlayerStart` / `ULyraPlayerSpawningManagerComponent` — 出生点占用检测、缓存和重生扩展点

**设计意图:**
- 通过 CommonUI 插件基类（`UCommonLocalPlayer`、`ACommonPlayerController`）实现增强的 UI 和本地玩家集成
- PlayerState 使用 Modular 基类以实现 GameFeature 组件注入
- 提供类型化访问器以在 Lyra 类之间安全导航
- 将重生选择逻辑从 GameMode 中拆出为 GameStateComponent，方便不同 Experience 注入不同出生规则

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ULyraLocalPlayer` | `UCommonLocalPlayer` | [Runtime] | 本地玩家包装器（CommonUser 集成） |
| `ALyraPlayerController` | `ACommonPlayerController` | [Runtime] | 玩家控制器（类型化访问器） |
| `ALyraReplayPlayerController` | `ALyraPlayerController` | [Runtime] | 回放控制器 |
| `ALyraPlayerState` | `AModularPlayerState` | [Runtime] 🧩 | 可复制的玩家状态数据 |
| `ALyraPlayerStart` | `APlayerStart` | [Runtime] | Lyra 出生点，占用检测与短期 Claim |
| `ULyraPlayerSpawningManagerComponent` | `UGameStateComponent` | [Runtime] | 出生点缓存、选择和重生扩展点 |

> 🧩 = 使用 Modular 基类，GameFeature 可注入组件

---

## 逐类详解

### ULyraLocalPlayer [Runtime]

**继承链:** `UObject → ULocalPlayer → UCommonLocalPlayer → ULyraLocalPlayer`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 本地玩家的引擎侧表示。当前为最小实现。

**接口:** 注释掉了 `ILyraTeamAgentInterface`（团队系统计划但尚未激活）。

**与 CommonUser 的关系:**
`UCommonLocalPlayer`（CommonUI 插件）提供了与 CommonUser 框架的集成。`ULyraGameInstance::HandlerUserInitialized()` 中有注释掉的代码调用 `LoadSharedSettingsFromDisk()` 来为此 LocalPlayer 加载设置。

---

### ALyraPlayerController [Runtime]

**继承链:** `AActor → AController → APlayerController → ACommonPlayerController → ALyraPlayerController`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "..."))`

**职责:** 基础玩家控制器。

**类型化访问器:**
- `GetLyraPlayerState()` — 将 `PlayerState` 转换为 `ALyraPlayerState*`
- `GetLyraHUD()` — 将 `MyHUD` 转换为 `ALyraHUD*`

**接口:** 注释掉了 `ILyraCameraAssistInterface`、`ILyraTeamAgentInterface`。

**子类:** `ALyraReplayPlayerController` — 用于回放系统。

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

### ALyraPlayerState [Runtime] 🧩

**继承链:** `AActor → AInfo → APlayerState → AModularPlayerState → ALyraPlayerState`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 可复制的玩家状态数据。

**关键方法:**
- `GetLyraPlayerController()` — 将 Owner 转换为 `ALyraPlayerController*`

**接口:** 注释掉了 `IAbilitySystemInterface`、`ILyraTeamAgentInterface`。

**Modular 基类的好处:**
`AModularPlayerState` 注册到 `UGameFrameworkComponentManager`。GameFeature Action 可以在 Experience 加载期间向 PlayerState 添加组件（例如 GAS 的 AbilitySystemComponent）。

---

### ALyraPlayerStart [Runtime]

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

**继承链:** `UObject → UActorComponent → UGameStateComponent → ULyraPlayerSpawningManagerComponent`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 出生点管理组件。它设计为挂在 GameState 上，由 Experience/GameFeature 按需注入，再由 `ALyraGameMode` 代理调用，从而让不同 Experience 能替换出生规则。

**初始化行为 (`InitializeComponent()`):**
1. 绑定 `FWorldDelegates::LevelAddedToWorld`，监听流式关卡加入。
2. 绑定 `UWorld::AddOnActorSpawnedHandler()`，监听运行时生成的 `ALyraPlayerStart`。
3. 遍历当前 World 中已有的 `ALyraPlayerStart`，缓存到 `CachedPlayerStarts`。

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
组件内部已经实现了 `ChoosePlayerStart()`、`ControllerCanRestart()`、`FinishRestartPlayer()`，并将 `ALyraGameMode` 声明为 friend；但当前 `ALyraGameMode` C++ 类还没有覆盖引擎的同名重生/出生点函数来代理到这个组件。因此这套能力目前是“组件侧已准备好，GameMode 接线仍待完成”。

---

## 框架内部关系

```
ULyraLocalPlayer
  └── 由 ULyraGameInstance 管理（1 个本地玩家 = 1 个控制器）

ALyraPlayerController
  ├── PlayerState → ALyraPlayerState
  ├── Pawn → ALyraCharacter
  └── MyHUD → ALyraHUD

ALyraPlayerState (复制到所有客户端)
  ├── GetLyraPlayerController() → ALyraPlayerController
  └── [计划] IAbilitySystemInterface → AbilitySystemComponent

ULyraPlayerSpawningManagerComponent (GameStateComponent)
  ├── CachedPlayerStarts[] → ALyraPlayerStart
  ├── ChoosePlayerStart() → Empty/Partial 优先级选择
  └── [待接入] ALyraGameMode 出生/重生代理

ALyraPlayerStart
  └── ClaimingController → 短期防止多个玩家抢同一出生点
```

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 管理 ULyraLocalPlayer
- [06-Character-Framework.md](06-Character-Framework.md) — PlayerController 控制的 Pawn 通常是 ALyraCharacter
- [08-UI-Framework.md](08-UI-Framework.md) — PlayerController 持有对 ALyraHUD 的引用
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — GAS 和 Team 接口尚未激活
- [04-Game-Framework.md](04-Game-Framework.md) — ALyraWorldSettings 在 Map Check 中要求使用 ALyraPlayerStart
