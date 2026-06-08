# 16 - Stub 与计划功能

> 所有标记为 TODO、被注释掉、或存在但仅有空实现的功能清单。在开始新功能开发前，先查阅此处了解当前缺口。

---

## 1. GAS 集成（Gameplay Ability System）

**影响范围:** 多个核心类

| 位置 | 接口/能力 | 状态 |
|------|------------|------|
| `ALyraGameState` | `IAbilitySystemInterface` | 已启用；当前持有比赛级 `ULyraAbilitySystemComponent` |
| `ALyraPlayerState` | `IAbilitySystemInterface` | 注释掉 |
| `ALyraCharacter` | `IAbilitySystemInterface`, `IGameplayCueInterface`, `IGameplayTagAssetInterface` | 全部注释掉 |
| `ULyraAbilitySystemComponent` | — | 最小封装，仅有构造函数，尚无 Lyra 专属 Ability/Effect 初始化逻辑 |
| `ULyraAssetManager::InitializeGameplayCueManager()` | — | 方法体为空，标记 TODO |
| `ULyraGameData` | — | Damage/Heal/DynamicTag GE 引用已定义，但应用它们的系统未接入 |

**说明:** GAS 正在从占位走向接入。`ALyraGameState` 已经创建并复制比赛级 ASC，`PostInitializeComponents()` 中会初始化 ActorInfo；但 PlayerState/Character 的 ASC、AbilitySet 授予、GameplayCue 管理和 `ULyraGameData` 中全局 GE 的实际应用仍未完成。

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
| 3 | **分阶段 Action 执行** | 非空 Action 在单次遍历中按 Register→Load→Activate 顺序执行；nullptr 会被跳过 | 无法实现 Action 之间的依赖排序 |
| 4 | **完整停用/卸载 Action** | 异步反激活框架已搭建，但未测试 | `OnGameFeatureDeactivating` 注册的异步 pauser 可能无法正确完成 |
| 5 | **预加载资产清理策略** | 预加载资产列表（`PreloadAssetList`）为空 | 未定义预加载完成后如何处理这些资产 |
| 6 | **GameFeature 停用** | 插件卸载引用计数存在，但实际停用不完整 | "泄漏"已加载的插件，它们在 Experience 之间保持活跃 |
| 7 | **切换 Experience 的差异比较** | 无差异比较 | 切换 Experience 时全部卸载然后重新加载，而非只卸载变化部分 |
| 8 | **内置插件 vs URL 式插件** | 统一按名称查找 | 未区分处理通过 `.uplugin` 加载的插件和通过 URL 加载的插件 |

---

## 4. Experience Ready 蓝图节点 TODO

源文件: `AsyncAction_ExperienceReady.cpp`

| TODO | 当前行为 | 影响 |
|------|----------|------|
| 区分启动加载期和运行期 | Experience 已 Loaded 时始终延迟到下一帧广播 `OnReady` | 运行期动态创建对象本可以立即收到 Ready，当前为了稳定时序统一延迟 |
| 将时序扰动下沉到 Experience 加载系统 | 每个等待 Experience 的 async action 自己决定是否延迟 | 后续如果出现更多等待节点，容易产生重复逻辑和不一致时序 |

---

## 5. 玩家定制

| 位置 | 问题 | 状态 |
|------|------|------|
| `ULyraGameInstance::HandlerUserInitialized()` | 调用 `LoadSharedSettingsFromDisk()` 的代码 | 注释掉，等待自定义 LocalPlayer 实现 |
| `ULyraGameInstance` | `GetPrimaryPlayerController()` | 注释掉的 TODO |
| `ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()` | `ULyraSettingsLocal::Get()->OnExperienceLoaded()` | 注释掉 |

---

## 6. 玩家出生/重生系统接入

| 位置 | 当前状态 | 后续工作 |
|------|----------|---------|
| `ALyraGameMode` ↔ `ULyraPlayerSpawningManagerComponent` | GameMode 已代理出生点选择、重生许可和重生完成钩子 | 继续扩展具体 Experience 的出生规则 |
| `ULyraPlayerSpawningManagerComponent::ControllerCanRestart()` | TODO，当前始终返回 true | 接入死亡状态、比赛状态、队伍/观战规则 |
| `ALyraPlayerStart::StartPointTags` | 字段已存在，但默认选择逻辑未使用 | 后续可用于队伍出生点、模式专属出生点或权重筛选 |
| `ULyraPawnExtensionComponent` | 新增占位组件，仅有构造函数 | 接入 PawnData、AbilitySystem、输入和初始化状态链 |
| `ALyraPlayerState::GetPawnData<T>()` | 模板占位实现，当前始终返回 nullptr | 后续保存玩家级 PawnData 覆盖，支持不同玩家使用不同 Pawn 配置 |
| `ALyraGameMode::SpawnDefaultPawnAtTransform_Implementation()` | 已 deferred spawn Pawn，但 PawnExtension 逻辑仍是 TODO | 在 `FinishSpawning()` 前完成 PawnExtension 初始化 |

---

## 7. 相机系统

| 类 | 注释掉的接口 |
|-----|------------|
| `ALyraPlayerController` | `ILyraCameraAssistInterface` |

---

## 8. 日志通道（仅有声明，无实现）

以下文件仅包含 `DECLARE_LOG_CATEGORY_EXTERN`，无类定义：

| 文件 | 日志分类 | 说明 |
|------|---------|------|
| `Player/LyraCheatManager.h` | `LogLyraCheat` | 作弊管理器类本身可能在其他地方或计划创建 |
| `Settings/LyraGameSettingRegistry.h` | `LogLyraGameSettingRegistry` | 实际注册表实现在 GameSettings 插件框架中 |
| `System/LyraReplicationGraph.h` | `LogLyraRepGraph` | 复制图谱类可能存在于 .cpp 中或计划创建 |
| `AbilitySystem/Phases/LyraGamePhaseLog.h` | `LogLyraGamePhase` | 游戏阶段逻辑尚未实现 |
| `LyraEditor/LyraEditor.h` | `LogLyraEditor` | 模块日志（类定义在 LyraEditorEngine.h） |

---

## 9. Gameplay Message / Cue 转换 TODO

| 位置 | 当前状态 | 后续工作 |
|------|----------|---------|
| `FLyraVerbMessage` | 通用 payload 已定义，`ToString()` 使用 Reflection 导出调试文本 | 后续需要接入真实 Damage、Elimination、Assist、UI 通知等生产者/消费者 |
| `ULyraVerbMessageHelpers::VerbMessageToCueParameters()` | 已映射 Verb、Instigator、Target、Source/Target Tags、Magnitude | `ContextTags` 尚未映射到 `FGameplayCueParameters` |
| `ULyraVerbMessageHelpers::CueParametersToVerbMessage()` | 已从 Cue 参数还原基础消息字段 | `ContextTags` 还原仍是 TODO |
| `ALyraGameState::MulticastMessageToClients()` | 已能把服务器消息广播到客户端 `UGameplayMessageSubsystem` | 需要明确哪些事件用 Reliable，避免把高频可丢事件做成可靠 RPC |

---

## 10. 编辑器

| 问题 | 说明 |
|------|------|
| `ULyraEditorEngine::FirstTickSetup()` | 强制 Content Browser 显示插件文件夹的代码"doesn't really work due to engine iteration" — 作为钩子点示例 |

---

## 开发建议

在继续推进 Lyra 复刻时，建议按以下优先级：

1. **GAS 集成** — 在 GameState ASC 的基础上继续接入 PlayerState/Character、AbilitySet 授予、GameplayCue 管理，让 `ULyraGameData` 中的 GE 引用发挥作用
2. **Pawn 初始化链** — 完成 `ULyraPawnExtensionComponent`、PlayerState PawnData 和 AbilitySystem 的接入
3. **重生规则扩展** — 让 `ControllerCanRestart()`、`StartPointTags` 和 Experience 自定义出生规则发挥作用
4. **Experience 系统完善** — 特别关注异步反激活 (#4) 和 GameFeature 停用 (#6) 的 TODO
5. **Experience Ready 时序整理** — 把 `UAsyncAction_ExperienceReady` 的下一帧扰动策略统一到 Experience 加载系统
6. **Gameplay Message 落地** — 把 `FLyraVerbMessage` 接入伤害、淘汰、助攻、UI 通知和 GameplayCue 转换链
7. **团队系统** — 解除 4 个类中被注释掉的 `ILyraTeamAgentInterface`
8. **玩家设置加载** — 解除 `HandlerUserInitialized` 和 `OnExperienceFullLoadCompleted` 中被注释掉的设置代码
9. **相机系统** — 解除 `ILyraCameraAssistInterface`

---

## 关联框架

- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 状态机与 Experience Ready 节点
- [03-System-Framework.md](03-System-Framework.md) — InitializeGameplayCueManager 空实现
- [06-Character-Framework.md](06-Character-Framework.md) — Character 中的注释掉接口
- [05-Player-Framework.md](05-Player-Framework.md) — Player 中的注释掉接口
