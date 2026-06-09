# 09 - GameplayTags 系统

> Lyra 的原生 GameplayTag 声明全部集中在 `LyraGameplayTags` 命名空间中。所有 Tag 使用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 声明以保证编译时安全。

---

## 架构说明

- **声明位置:** `Source/LyraGame/LyraGameplayTags.h`（命名空间 `LyraGameplayTags`）
- **定义位置:** `Source/LyraGame/LyraGameplayTags.cpp`（通过 `UE_DEFINE_GAMEPLAY_TAG` 宏）
- **设计目的:** 用原生 C++ Tag 代替字符串 Tag，避免拼写错误，提供 IDE 自动补全支持

---

## Tag 分类

### 1. 技能激活失败原因 (Ability Activate Fail)

| Tag | 说明 |
|-----|------|
| `Ability_ActivateFail_IsDead` | 角色已死亡 |
| `Ability_ActivateFail_Cooldown` | 技能冷却中 |
| `Ability_ActivateFail_Cost` | 资源不足（法力/体力等） |
| `Ability_ActivateFail_TagsBlocked` | 被某个 GameplayTag 阻止 |
| `Ability_ActivateFail_TagsMissing` | 缺少必需的 GameplayTag |
| `Ability_ActivateFail_Networking` | 网络条件不满足 |
| `Ability_ActivateFail_ActivationGroup` | 激活组冲突（同组技能互斥） |

### 2. 技能行为 (Ability Behavior)

| Tag | 说明 |
|-----|------|
| `Ability_Behavior_SurvivesDeath` | 技能在角色死亡后仍然存活（不被移除） |

### 3. 输入 (Input)

| Tag | 说明 |
|-----|------|
| `InputTag_Move` | 移动输入 |
| `InputTag_Look_Mouse` | 鼠标视角输入 |
| `InputTag_Look_Stick` | 手柄摇杆视角输入 |
| `InputTag_Crouch` | 蹲伏输入 |
| `InputTag_AutoRun` | 自动奔跑输入 |

### 4. 初始化状态 (InitState)

这些 Tag 被 `UGameFrameworkComponentManager` 使用，构成所有 Modular Actor 的初始化流水线：

| Tag | 说明 | 依赖 |
|-----|------|------|
| `InitState_Spawned` | Actor 已生成 | 无 |
| `InitState_DataAvailable` | 数据已可用 | Spawned |
| `InitState_DataInitialized` | 数据已初始化 | DataAvailable |
| `InitState_GameplayReady` | 游戏玩法已就绪 | DataInitialized |

> **关键:** 这些状态转移关系在 `ULyraGameInstance::Init()` 中注册到 `UGameFrameworkComponentManager`。没有这个注册，所有 Modular Actor 的初始化都会中断。

### 5. 游戏事件 (Gameplay Event)

| Tag | 说明 |
|-----|------|
| `GameplayEvent_Death` | 死亡事件 |
| `GameplayEvent_Reset` | 重置事件 |
| `GameplayEvent_RequestReset` | 请求重置事件 |

### 6. SetByCaller（GAS 数值传递）

| Tag | 说明 |
|-----|------|
| `SetByCaller_Damage` | 伤害数值（由调用者在执行时设定） |
| `SetByCaller_Heal` | 治疗数值（由调用者在执行时设定） |

### 7. 作弊 (Cheat)

| Tag | 说明 |
|-----|------|
| `Cheat_GodMode` | 无敌模式 |
| `Cheat_UnlimitedHealth` | 无限生命 |

### 8. 状态 (Status)

| Tag | 说明 |
|-----|------|
| `Status_Crouching` | 正在蹲伏 |
| `Status_AutoRunning` | 正在自动奔跑 |
| `Status_Death` | 死亡中（通用） |
| `Status_Death_Dying` | 死亡中（濒死阶段） |
| `Status_Death_Dead` | 已死亡 |

### 9. 移动模式 (Movement Mode)

| Tag | 说明 |
|-----|------|
| `Movement_Mode_Walking` | 行走 |
| `Movement_Mode_NavWalking` | 导航行走 |
| `Movement_Mode_Falling` | 下落 |
| `Movement_Mode_Swimming` | 游泳 |
| `Movement_Mode_Flying` | 飞行 |
| `Movement_Mode_Custom` | 自定义移动模式 |

---

## 移动模式映射表

```cpp
extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;
```

这两个静态 `TMap` 将引擎的 `EMovementMode` 枚举值映射到对应的 GameplayTag。用于将 C++ 层的移动状态暴露给 GameplayTag 系统（例如让技能根据移动模式产生不同效果）。

---

## 辅助函数

### `FindTagByString(const FString& TagString, bool bMatchPartialString = false)`

```cpp
LYRAGAME_API FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);
```

通过字符串查找 GameplayTag：
- `bMatchPartialString = false`：精确匹配
- `bMatchPartialString = true`：部分匹配（寻找第一个名称包含 `TagString` 的 Tag）

---

## GameplayTag Stack

`Source/LyraGame/System/GameplayTagStack.*` 新增了用于运行时状态计数的 GameplayTag 栈结构。它不声明新的 Tag，而是把已有 `FGameplayTag` 变成“带数量、可复制、可查询”的玩家状态容器。

| 类型 | 基类 / 机制 | 作用 |
|------|-------------|------|
| `FGameplayTagStack` | `FFastArraySerializerItem` | 单个 Tag + StackCount，可输出 `TagxCount` 调试字符串 |
| `FGameplayTagStackContainer` | `FFastArraySerializer` | 管理 Tag Stack 数组，支持 FastArray 增量复制 |

**关键行为:**
- `AddStack(Tag, Count)`：Tag 有效且 Count > 0 时增加数量；已有项调用 `MarkItemDirty()`，新增项加入数组并标脏。
- `RemoveStack(Tag, Count)`：数量扣到 0 以下时删除数组项并 `MarkArrayDirty()`，否则更新数量并 `MarkItemDirty()`。
- `GetStackCount(Tag)` / `ContainsTag(Tag)`：通过本地 `TagToCountMap` 快速查询。
- `PreReplicatedRemove()` / `PostReplicatedAdd()` / `PostReplicatedChange()`：客户端收到 FastArray 增量后同步维护 `TagToCountMap`。

当前接入点是 `ALyraPlayerState::StatTags`。服务器通过 `AddStatTagStack()` / `RemoveStatTagStack()` 改变玩家统计标签，客户端读取 `GetStatTagStackCount()` / `HasStatTag()`。

---

### 10. 新增 Tag（分散定义）

以下 GameplayTag 在非 `LyraGameplayTags` 命名空间的其他文件中定义：

| Tag | 定义位置 | 用途 |
|-----|---------|------|
| `Platform.Trait.Input.HardwareCursor` | `Source/LyraGame/UI/LyraGameViewportClient.cpp` (`GameViewportTags` 命名空间) | 检测平台是否支持硬件光标，用于决定使用硬件光标还是软件光标部件 |
| `Platform.Trait.ReplaySupport` | `Source/LyraGame/Replays/LyraReplaySubsystem.cpp` | 检测平台是否支持回放；`ULyraUserFacingExperienceDefinition` 只有在该 Trait 存在时才会把 `DemoRec` 写入 Session 参数 |

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 注册 InitState Tag 依赖
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerState 使用 GameplayTag Stack 复制玩家统计状态
- [06-Character-Framework.md](06-Character-Framework.md) — Status_* Tag 用于角色状态管理
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — AbilitySet、Character 侧 GAS 与更多 GameplayTag 生产者仍在持续接入
