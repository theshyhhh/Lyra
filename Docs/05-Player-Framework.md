# 05 - Player 框架

> 本地玩家、玩家控制器、玩家状态。这些类管理玩家从登录到游戏中操作的全流程。

---

## 框架概述

Player 框架涵盖所有代表"玩家"的对象层次：
- `ULyraLocalPlayer` — 本地玩家的引擎级表示（UI 绑定等）
- `ALyraPlayerController` — 控制 Pawn 的控制器（输入、相机等）
- `ALyraPlayerState` — 复制到所有客户端的玩家状态数据

**设计意图:**
- 通过 CommonUI 插件基类（`UCommonLocalPlayer`、`ACommonPlayerController`）实现增强的 UI 和本地玩家集成
- PlayerState 使用 Modular 基类以实现 GameFeature 组件注入
- 提供类型化访问器以在 Lyra 类之间安全导航

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ULyraLocalPlayer` | `UCommonLocalPlayer` | [Runtime] | 本地玩家包装器（CommonUser 集成） |
| `ALyraPlayerController` | `ACommonPlayerController` | [Runtime] | 玩家控制器（类型化访问器） |
| `ALyraReplayPlayerController` | `ALyraPlayerController` | [Runtime] | 回放控制器 |
| `ALyraPlayerState` | `AModularPlayerState` | [Runtime] 🧩 | 可复制的玩家状态数据 |

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
```

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 管理 ULyraLocalPlayer
- [06-Character-Framework.md](06-Character-Framework.md) — PlayerController 控制的 Pawn 通常是 ALyraCharacter
- [08-UI-Framework.md](08-UI-Framework.md) — PlayerController 持有对 ALyraHUD 的引用
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — GAS 和 Team 接口尚未激活
