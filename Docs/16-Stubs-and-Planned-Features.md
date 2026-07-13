# 16 - Stub 与计划功能

> 所有标记为 TODO、被注释掉、或存在但仅有空实现的功能清单。在开始新功能开发前，先查阅此处了解当前缺口。

---

## 1. GAS 集成（Gameplay Ability System）

**影响范围:** 多个核心类

| 位置 | 接口/能力 | 状态 |
|------|------------|------|
| `ALyraGameState` | `IAbilitySystemInterface` | 已启用；当前持有比赛级 `ULyraAbilitySystemComponent` |
| `ALyraPlayerState` | `IAbilitySystemInterface` | 已启用；当前持有玩家级 `ULyraAbilitySystemComponent`，AbilitySet 授予仍是 TODO |
| `ALyraCharacter` | `IAbilitySystemInterface`, `IGameplayCueInterface`, `IGameplayTagAssetInterface` | 全部注释掉 |
| `ULyraAbilitySystemComponent` | — | 最小封装，仅有构造函数，尚无 Lyra 专属 Ability/Effect 初始化逻辑 |
| `ULyraAssetManager::InitializeGameplayCueManager()` | — | 方法体为空，标记 TODO |
| `ULyraGameData` | — | Damage/Heal/DynamicTag GE 引用已定义，但应用它们的系统未接入 |

**说明:** GAS 正在从占位走向接入。`ALyraGameState` 已经创建并复制比赛级 ASC，`ALyraPlayerState` 也创建并复制玩家级 ASC；两者都会在 `PostInitializeComponents()` 初始化 ActorInfo。但 PlayerState 的 AbilitySet 授予仍被注释，Character 侧 ASC/GameplayCue/GameplayTag 接口、GameplayCue 管理和 `ULyraGameData` 中全局 GE 的实际应用仍未完成。

---

## 2. 团队系统

**影响范围:** PlayerState 是队伍数据权威来源；PlayerController 已接入代理，Character 与 LocalPlayer 仍待接入

| 类 | 状态 |
|-----|------|
| `ALyraPlayerState` | 已实现 `ILyraTeamAgentInterface`，复制 `MyTeamID`，队伍变化时广播 `FOnLyraTeamIndexChangedDelegate` |
| `ALyraCharacter` | `ILyraTeamAgentInterface` 仍注释掉 |
| `ALyraPlayerController` | 已实现 `ILyraTeamAgentInterface`；只从 `PlayerState` 读取 TeamID、转发队伍变化委托，拒绝直接设置 TeamID。静态源码已接入，尚未完成多人运行验证 |
| `ULyraLocalPlayer` | `ILyraTeamAgentInterface` 仍注释掉 |

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
| `ALyraPlayerState::SetPawnData()` | 已保存并复制玩家级 PawnData，但 AbilitySet 授予逻辑仍被注释 | 接入 `ULyraPawnData` 中后续扩展出的 AbilitySet / 初始化数据 |
| `ALyraGameMode::SpawnDefaultPawnAtTransform_Implementation()` | 已 deferred spawn Pawn，但 PawnExtension 逻辑仍是 TODO | 在 `FinishSpawning()` 前完成 PawnExtension 初始化 |

---

## 7. PlayerController、相机与回放

| 位置 | 当前工作区状态 | 与 Lyra 原项目的主要差距 / 风险 |
|------|--------------|--------------------------------|
| `ALyraPlayerController` | 已实现控制/解除控制、ASC Avatar 清理、PlayerState/队伍委托迁移、自动奔跑、力反馈过滤、观战旋转和一次性 View Target 隐藏 | `ProcessAbilityInput()`、`TryActivateAbilitiesOnSpawn()`、共享设置绑定仍被注释；这些是明确的 GAS/设置缺口 |
| 观战视角复制 | 已禁用父类 `TargetViewRotation` 的 `COND_OwnerOnly` 复制，改由 `PlayerState::ReplicatedViewRotation` 以 `COND_SkipOwner` 分发 | Setter（设置函数）没有服务器权威断言；UE 5.7.4 的服务器远程 Controller 绕过 `PlayerTick()`，当前也没有第二处 Setter 调用，远程玩家的服务器权威旋转来源需重点验证 |
| `ILyraCameraAssistInterface` 与隐藏消费者 | 接口已创建，PlayerController 已实现 `OnCameraPenetratingTarget()` 和 `UpdateHiddenComponents()` | 当前源码没有调用回调的 Camera Mode（相机模式）或穿透检测生产者；功能链处于“消费者完成、生产者缺失” |
| `ALyraPlayerCameraManager` | 已被 PlayerController 指定为相机管理器；空子类继承 UE 5.7.4 的基础相机更新 | 原项目的 `ULyraUICameraManagerComponent`、80° 默认 FOV、Pitch（俯仰）范围、`UpdateViewTarget()` 与 `DisplayDebug()` 均未接入 |
| `Camera/LyraPlayerCameraManager.cpp` | 当前只包含 `#include "LyraPlayerCameraManager.h"` | 不再是空预处理指令；翻译单元有效，但没有 Lyra 专属实现 |
| `ALyraReplayPlayerController` | 已实现录制者 PlayerState 重新绑定、Pawn 变化跟随和 View Target 修复；`SmoothTargetViewRotation()` 沿用父类 | 回放播放跟随路径静态对齐，但没有运行证据；`ShouldRecordClientReplay()` 按设计始终返回 `false` |
| 客户端回放录制 | `TryToRecordClientReplay()` 已检查网络模式、地图、Replay 状态和本地控制器，并准备写入 `RecorderPlayerState` | 本地设置判断被注释，当前资格检查必定为 `false`；`ULyraReplaySubsystem::RecordClientReplay()` 尚不存在且调用被注释，函数仍可能在只设置状态后返回 `true` |

> 上表按当前工作区源码核对，不含编译、PIE（Play In Editor）或联网运行证据。完整调用链和源码阅读入口见 [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)。

---

## 8. Cheat、External RPC 与剩余日志占位

### 已新增类型，但功能仍不完整

| 位置 | 当前状态 | 差距 / 风险 |
|------|----------|-------------|
| `ULyraCheatManager` | 已定义为 `UCheatManager` 最小子类，非发布构建由 PlayerController 选择并强制尝试创建 | 继承引擎基础命令，但缺少 Lyra 原项目的 GAS、伤害/治疗、GameplayTag（游戏玩法标签）、相机和无敌等扩展 |
| `ServerCheat()` / `ServerCheatAll()` | Reliable Server RPC（可靠服务器远程过程调用）已声明；实际命令执行体在发布构建中由宏编译为空操作 | `WithValidation` 的验证函数无条件返回 `true`；非发布联网环境缺少白名单和额外权限校验 |
| `ULyraGameplayRpcRegistrationComponent` | 已定义为 `UExternalRpcRegistrationComponent` 空子类 | 没有单例、实例化、路由或 Handler（处理函数），当前不提供 Lyra HTTP 自动化端点 |
| `ALyraPlayerController::BeginPlay()` | 非发布构建调用 `StartAllListeners()` 并解析 `rpcport` | 只会启动已经存在的 Listener（监听器）；注册调用被注释，端口值也没有交给当前子类使用 |

### 仍只有日志声明

以下文件仍只有 `DECLARE_LOG_CATEGORY_EXTERN`，没有对应功能类：

| 文件 | 日志分类 | 说明 |
|------|---------|------|
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

1. **GAS 集成** — 在 GameState/PlayerState ASC 的基础上继续接入 Character、AbilitySet 授予、GameplayCue 管理，让 `ULyraGameData` 中的 GE 引用发挥作用
2. **Pawn 初始化链** — 完成 `ULyraPawnExtensionComponent` 和 PlayerState PawnData/AbilitySystem 与 Pawn 的协作
3. **重生规则扩展** — 让 `ControllerCanRestart()`、`StartPointTags` 和 Experience 自定义出生规则发挥作用
4. **Experience 系统完善** — 特别关注异步反激活 (#4) 和 GameFeature 停用 (#6) 的 TODO
5. **Experience Ready 时序整理** — 把 `UAsyncAction_ExperienceReady` 的下一帧扰动策略统一到 Experience 加载系统
6. **Gameplay Message 落地** — 把 `FLyraVerbMessage` 接入伤害、淘汰、助攻、UI 通知和 GameplayCue 转换链
7. **团队系统** — PlayerState 与 PlayerController 已接入；继续接入 Character、LocalPlayer 的 `ILyraTeamAgentInterface`
8. **玩家设置加载** — 解除 `HandlerUserInitialized` 和 `OnExperienceFullLoadCompleted` 中被注释掉的设置代码
9. **相机系统** — 补齐 Camera Mode 穿透检测生产端、UI Camera 组件和 Lyra 相机管理器重写
10. **回放与自动化** — 先补齐客户端录制 Subsystem（子系统）和真实录制调用，再决定是否开放 External RPC 路由；开放前设计鉴权、命令白名单与生命周期注销

---

## 关联框架

- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 状态机与 Experience Ready 节点
- [03-System-Framework.md](03-System-Framework.md) — InitializeGameplayCueManager 空实现
- [06-Character-Framework.md](06-Character-Framework.md) — Character 中的注释掉接口
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerState 的 ASC、PawnData、Team/Squad 与 StatTag 接入状态
