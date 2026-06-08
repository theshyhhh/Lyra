# 16 - Stub 与计划功能

> 所有标记为 TODO、被注释掉、或存在但仅有空实现的功能清单。在开始新功能开发前，先查阅此处了解当前缺口。

---

## 1. GAS 集成（Gameplay Ability System）

**影响范围:** 多个核心类

| 位置 | 注释掉的接口 | 状态 |
|------|------------|------|
| `ALyraGameState` | `IAbilitySystemInterface` | 注释掉 |
| `ALyraPlayerState` | `IAbilitySystemInterface` | 注释掉 |
| `ALyraCharacter` | `IAbilitySystemInterface`, `IGameplayCueInterface`, `IGameplayTagAssetInterface` | 全部注释掉 |
| `ULyraAssetManager::InitializeGameplayCueManager()` | — | 方法体为空，标记 TODO |
| `ULyraGameData` | — | Damage/Heal/DynamicTag GE 引用已定义，但应用它们的系统未接入 |

**说明:** GAS 是 Lyra 核心玩法层级的缺失部分。`ULyraGameData` 中对伤害/治疗效果的软引用已定义，`ALyraCharacterWithAbilities` 子类已创建，但整个 AbilitySystemComponent 的集成尚未激活。

---

## 2. 团队系统

**影响范围:** 4 个类

| 类 | 注释掉的接口 |
|-----|------------|
| `ALyraCharacter` | `ILyraTeamAgentInterface` |
| `ALyraPlayerState` | `ILyraTeamAgentInterface` |
| `ALyraPlayerController` | `ILyraTeamAgentInterface` |
| `ULyraLocalPlayer` | `ILyraTeamAgentInterface` |

---

## 3. Experience 系统 TODO（8 项）

源文件: `LyraExperienceManagerComponent.cpp` 第 16-23 行

| # | TODO | 当前行为 | 影响 |
|---|------|---------|------|
| 1 | **异步加载 Experience Definition 本身** | 使用 `TryLoad()` 同步加载 | `SetCurrentExperience` 会短暂阻塞游戏线程 |
| 2 | **显式失败处理** | 使用 `check()` 断言，无效状态会导致崩溃 | 生产线中 Experience 配置错误会导致致命错误 |
| 3 | **分阶段 Action 执行** | 所有 Action 在单次遍历中按 Register→Load→Activate 顺序执行 | 无法实现 Action 之间的依赖排序 |
| 4 | **完整停用/卸载 Action** | 异步反激活框架已搭建，但未测试 | `OnGameFeatureDeactivating` 注册的异步 pauser 可能无法正确完成 |
| 5 | **预加载资产清理策略** | 预加载资产列表（`PreloadAssetList`）为空 | 未定义预加载完成后如何处理这些资产 |
| 6 | **GameFeature 停用** | 插件卸载引用计数存在，但实际停用不完整 | "泄漏"已加载的插件，它们在 Experience 之间保持活跃 |
| 7 | **切换 Experience 的差异比较** | 无差异比较 | 切换 Experience 时全部卸载然后重新加载，而非只卸载变化部分 |
| 8 | **内置插件 vs URL 式插件** | 统一按名称查找 | 未区分处理通过 `.uplugin` 加载的插件和通过 URL 加载的插件 |

---

## 4. 玩家定制

| 位置 | 问题 | 状态 |
|------|------|------|
| `ULyraGameInstance::HandlerUserInitialized()` | 调用 `LoadSharedSettingsFromDisk()` 的代码 | 注释掉，等待自定义 LocalPlayer 实现 |
| `ULyraGameInstance` | `GetPrimaryPlayerController()` | 注释掉的 TODO |
| `ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()` | `ULyraSettingsLocal::Get()->OnExperienceLoaded()` | 注释掉 |

---

## 5. 玩家出生/重生系统接入

| 位置 | 当前状态 | 后续工作 |
|------|----------|---------|
| `ALyraGameMode` ↔ `ULyraPlayerSpawningManagerComponent` | GameMode 已代理出生点选择、重生许可和重生完成钩子 | 继续扩展具体 Experience 的出生规则 |
| `ULyraPlayerSpawningManagerComponent::ControllerCanRestart()` | TODO，当前始终返回 true | 接入死亡状态、比赛状态、队伍/观战规则 |
| `ALyraPlayerStart::StartPointTags` | 字段已存在，但默认选择逻辑未使用 | 后续可用于队伍出生点、模式专属出生点或权重筛选 |
| `ULyraPawnExtensionComponent` | 新增占位组件，仅有构造函数 | 接入 PawnData、AbilitySystem、输入和初始化状态链 |
| `ALyraPlayerState::GetPawnData<T>()` | 模板占位实现，当前始终返回 nullptr | 后续保存玩家级 PawnData 覆盖，支持不同玩家使用不同 Pawn 配置 |
| `ALyraGameMode::SpawnDefaultPawnAtTransform_Implementation()` | 已 deferred spawn Pawn，但 PawnExtension 逻辑仍是 TODO | 在 `FinishSpawning()` 前完成 PawnExtension 初始化 |

---

## 6. 相机系统

| 类 | 注释掉的接口 |
|-----|------------|
| `ALyraPlayerController` | `ILyraCameraAssistInterface` |

---

## 7. 日志通道（仅有声明，无实现）

以下文件仅包含 `DECLARE_LOG_CATEGORY_EXTERN`，无类定义：

| 文件 | 日志分类 | 说明 |
|------|---------|------|
| `Player/LyraCheatManager.h` | `LogLyraCheat` | 作弊管理器类本身可能在其他地方或计划创建 |
| `Settings/LyraGameSettingRegistry.h` | `LogLyraGameSettingRegistry` | 实际注册表实现在 GameSettings 插件框架中 |
| `System/LyraReplicationGraph.h` | `LogLyraRepGraph` | 复制图谱类可能存在于 .cpp 中或计划创建 |
| `AbilitySystem/Phases/LyraGamePhaseLog.h` | `LogLyraGamePhase` | 游戏阶段逻辑尚未实现 |
| `LyraEditor/LyraEditor.h` | `LogLyraEditor` | 模块日志（类定义在 LyraEditorEngine.h） |

---

## 8. 编辑器

| 问题 | 说明 |
|------|------|
| `ULyraEditorEngine::FirstTickSetup()` | 强制 Content Browser 显示插件文件夹的代码"doesn't really work due to engine iteration" — 作为钩子点示例 |

---

## 开发建议

在继续推进 Lyra 复刻时，建议按以下优先级：

1. **GAS 集成** — 解除多个核心类中被注释掉的能力系统接口，让 `ULyraGameData` 中的 GE 引用发挥作用
2. **Pawn 初始化链** — 完成 `ULyraPawnExtensionComponent`、PlayerState PawnData 和 AbilitySystem 的接入
3. **重生规则扩展** — 让 `ControllerCanRestart()`、`StartPointTags` 和 Experience 自定义出生规则发挥作用
4. **Experience 系统完善** — 特别关注异步反激活 (#4) 和 GameFeature 停用 (#6) 的 TODO
5. **团队系统** — 解除 4 个类中被注释掉的 `ILyraTeamAgentInterface`
6. **玩家设置加载** — 解除 `HandlerUserInitialized` 和 `OnExperienceFullLoadCompleted` 中被注释掉的设置代码
7. **相机系统** — 解除 `ILyraCameraAssistInterface`

---

## 关联框架

- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 状态机中的 8 个 TODO 来源
- [03-System-Framework.md](03-System-Framework.md) — InitializeGameplayCueManager 空实现
- [06-Character-Framework.md](06-Character-Framework.md) — Character 中的注释掉接口
- [05-Player-Framework.md](05-Player-Framework.md) — Player 中的注释掉接口
