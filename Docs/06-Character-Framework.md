# 06 - Character 框架

> 角色 Pawn 和 Pawn 生成配置。Lyra 的角色系统通过 Modular 基类和 GAS 就绪子类支持 GameFeature 扩展。

---

## 框架概述

Character 框架包含角色 Pawn 类层次结构和控制哪个 Pawn 类生成的配置数据资产。

**设计意图:**
- 基类 `ALyraCharacter` 使用 `AModularCharacter` 以启用 GameFeature 组件注入
- `ALyraCharacterWithAbilities` 作为 GAS 集成的指定子类
- `ULyraPawnData` 将"生成哪个 Pawn 类"决策从代码中抽离到数据资产中

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ALyraCharacter` | `AModularCharacter` | [Runtime] 🧩 | 基础角色 Pawn |
| `ALyraCharacterWithAbilities` | `ALyraCharacter` | [Runtime] Blueprintable | GAS 就绪角色子类 |
| `ULyraPawnData` | `UPrimaryDataAsset` | [Runtime] | 定义生成哪个 Pawn 类的数据资产 |

> 🧩 = 使用 Modular 基类，GameFeature 可注入组件

---

## 逐类详解

### ALyraCharacter [Runtime] 🧩

**继承链:** `AActor → APawn → ACharacter → AModularCharacter → ALyraCharacter`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))`

**职责:** 项目基础角色 Pawn 类。

**类型化访问器:**
- `GetLyraPlayerController()` — 将 Controller 转换为 `ALyraPlayerController*`
- `GetLyraPlayerState()` — 将 PlayerState 转换为 `ALyraPlayerState*`

**注释掉的接口（4 个）:**
- `IAbilitySystemInterface` — GAS 集成（计划）
- `IGameplayCueInterface` — GameplayCue 视觉反馈（计划）
- `IGameplayTagAssetInterface` — GameplayTag 查询接口（计划）
- `ILyraTeamAgentInterface` — 团队归属（计划）

**Modular 基类的好处:**
`AModularCharacter` 注册到 `UGameFrameworkComponentManager`。Experience 的 GameFeatureAction 可以动态添加组件（如 GAS 的 `UAbilitySystemComponent`、输入组件等）。

---

### ALyraCharacterWithAbilities [Runtime]

**继承链:** `ALyraCharacter → ALyraCharacterWithAbilities`

**UCLASS:** `UCLASS(MinimalAPI, Blueprintable)`

**职责:** 为 GAS 集成指定的角色子类。当前为最小实现（空类体），作为基于蓝图的角色蓝图需要 Ability System 集成时的基类。

**Blueprintable:** 此标记允许在蓝图中派生，是创建具体角色蓝图的入口点。

> **重命名历史:** 此类原名为 `ALyraCharacterWithAbilities`，已通过 CoreRedirect 重命名为 `LyraCharacterWithAbilities`。

---

### ULyraPawnData [Runtime]

**继承链:** `UObject → UDataAsset → UPrimaryDataAsset → ULyraPawnData`

**UCLASS:** `UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "Lyra Pawn Data", ShortTooltip = "..."))`

**职责:** 定义特定游戏玩法上下文中生成哪个 Pawn 类的数据资产。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `PawnClass` | `TSubclassOf<APawn>` | 要实例化的 Pawn 类（通常为 `ALyraCharacter` 或 `ALyraCharacterWithAbilities`） |

**引用位置:**
- `ULyraExperienceDefinition::DefaultPawnData` — 此 Experience 中玩家默认使用的 Pawn
- `ULyraAssetManager::DefaultPawnData` — PlayerState 未指定 PawnData 时的全局保底

---

## 框架内部关系

```
ULyraExperienceDefinition
  └── DefaultPawnData → ULyraPawnData
        └── PawnClass (TSubclassOf<APawn>)
              ├── ALyraCharacter (基类)
              └── ALyraCharacterWithAbilities (GAS 就绪)

运行时:
  PlayerState 未指定 PawnData?
    ├── 是 → AssetManager::GetDefaultPawnData() (全局保底)
    └── 否 → Experience 的 DefaultPawnData
           └── 生成 PawnClass → ALyraCharacter (或子类)
```

---

## 关联框架

- [04-Game-Framework.md](04-Game-Framework.md) — GameMode 管理角色生成
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerController 控制生成的角色
- [07-Experience-Framework.md](07-Experience-Framework.md) — PawnData 由 Experience 指定
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — GAS 集成：4 个接口被注释掉
