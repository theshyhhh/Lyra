# 16 - 结构占位与计划功能

> 本文汇总当前项目中的源码 TODO、被注释路径、结构占位、Lyra 对比缺口和
> 运行验证任务。它不是“Lyra 有什么就全部照搬”的清单，而是用于判断当前
> 主流程缺在哪里、为什么缺，以及继续复刻需要哪些前置依赖。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 LocalPlayer（本地玩家）、Settings（设置）、GameInstance（游戏
> 实例）和 Experience（体验）改动。当前项目与 Lyra 参考项目均声明 UE 5.7，
> 底层机制按 UE 5.7.4 核对；未执行编译、PIE、登录、联网、Replay 或 HTTP
> 测试。

---

## 使用方法与状态边界

本文只使用以下实现状态：

- **已复刻：** 核心结构和主要行为与参考实现基本一致。
- **部分复刻：** 主体存在，但缺少重要分支、配置、清理或关联功能。
- **结构占位：** 只有类、函数、`Super` 调用或 TODO。
- **未复刻：** 当前项目没有目标功能的核心实现。
- **有意简化：** 当前项目明确选择更简单且符合目标的实现。
- **项目自定义：** 当前行为是主动设计，不是 Lyra 遗漏。
- **待确认：** 需要资产、运行、网络或打包验证。

TODO 来源分为“当前源码 TODO”“Lyra 对比 TODO”“验证 TODO”和“文档
TODO”。后续优先级只表示对当前学习主链的影响，不表示所有 Lyra 差异都必须
复刻。

## 当前缺口总览

| 系统 | 当前状态 | 最重要的代码证据 | 当前影响 |
|---|---|---|---|
| Player / GAS 初始化 | **部分复刻** | ASC 已在 PlayerState，AbilitySet、Pawn Extension、输入和生成能力调用仍注释 | 玩家能力主链未闭合 |
| Team（队伍） | **部分复刻** | PlayerState、Controller 与 LocalPlayer 已接入，Character 未接入 | 本地桥梁已闭合，Pawn 侧协议仍缺失 |
| LocalPlayer（本地玩家）桥梁 | **部分复刻** | Controller 切换、Team 转发、设置缓存与音频设备消费者存在 | 下游设置实现不完整，运行时未验证 |
| Shared Settings（共享设置） | **部分复刻** | 同步创建 / 加载和 LocalPlayer 缓存存在；异步函数直接返回 `false` | 登录链触发 `ensure(false)`，不会加载或替换真实对象 |
| Local Settings（本地设置） | **结构占位** | 单例、Editor CVar、音频 Event 和 Experience 钩子存在 | 没有主要设置字段、Setter、广播者或重应用行为 |
| 出生与重生 | **部分复刻** | 选择、Claim 和 GameMode 代理存在，重生许可仍总是 true | 缺少死亡、比赛和队伍规则 |
| Camera Assist（相机辅助） | **部分复刻** | 隐藏消费者存在，没有 Camera Mode 生产者 | 正常相机流程不会触发 |
| PlayerCameraManager（玩家相机管理器） | **结构占位** | 空子类只继承 UE 父类 | Lyra UI Camera、FOV 和调试缺失 |
| Replay 播放 | **部分复刻** | Recorder PlayerState / Pawn 跟随存在 | 需要 Seek / Checkpoint 运行验证 |
| 客户端 Replay 录制 | **未复刻** | 资格恒 false，Subsystem API 和调用缺失 | 无法自动录制客户端回放 |
| Cheat Manager 扩展 | **结构占位** | 空 `UCheatManager` 子类 | 没有 Lyra 项目命令 |
| External RPC | **结构占位** | 空子类，没有实例、路由和 Handler | 没有 Lyra HTTP 端点 |

---

## 1. GAS 集成（Gameplay Ability System）

**影响范围:** 多个核心类

| 位置 | 接口/能力 | 当前状态 | 当前实现与影响 |
|---|---|---|---|
| `ALyraGameState` | `IAbilitySystemInterface` | **部分复刻** | 持有并复制比赛级 `ULyraAbilitySystemComponent`，但完整比赛 GAS 消费者尚未接入 |
| `ALyraPlayerState` | `IAbilitySystemInterface` | **部分复刻** | 持有并复制玩家级 ASC；AbilitySet 授予仍是 TODO |
| `ALyraCharacter` | `IAbilitySystemInterface`、`IGameplayCueInterface`、`IGameplayTagAssetInterface` | **未复刻** | 三个接口仍被注释，Character 侧没有进入 GAS 主链 |
| `ULyraAbilitySystemComponent` | Lyra 类型化 ASC | **结构占位** | 只有最小构造函数，没有 Lyra 专属 Ability / Effect 行为 |
| `ULyraAssetManager::InitializeGameplayCueManager()` | GameplayCue 启动初始化 | **结构占位** | 方法体只有 TODO，不产生 Cue 预加载结果 |
| `ULyraGameData` | Damage / Heal / DynamicTag GE 配置 | **部分复刻** | 三个 GE 软类引用已定义，但当前系统没有完整消费者 |

**说明:** GAS 正在从占位走向接入。`ALyraGameState` 已经创建并复制比赛级 ASC，`ALyraPlayerState` 也创建并复制玩家级 ASC；两者都会在 `PostInitializeComponents()` 初始化 ActorInfo。但 PlayerState 的 AbilitySet 授予仍被注释，Character 侧 ASC/GameplayCue/GameplayTag 接口、GameplayCue 管理和 `ULyraGameData` 中全局 GE 的实际应用仍未完成。

---

## 2. 团队系统

**影响范围:** PlayerState 是队伍数据权威来源；PlayerController 和
LocalPlayer 已接入代理，Character 仍待接入。

| 类 | 当前状态 | 当前实现与影响 |
|---|---|---|
| `ALyraPlayerState` | **部分复刻** | 实现 `ILyraTeamAgentInterface`、复制 `MyTeamID` 并广播变化；Squad 通知和完整 Team Subsystem（队伍子系统）仍缺失 |
| `ALyraCharacter` | **未复刻** | `ILyraTeamAgentInterface` 仍被注释，Character 不能参与当前 Team 协议 |
| `ALyraPlayerController` | **部分复刻** | 从 PlayerState 读取 Team ID、转发委托并拒绝直接写入；静态链已接入，多人运行仍待验证 |
| `ULyraLocalPlayer` | **部分复刻** | 已实现 `ILyraTeamAgentInterface`，在 Controller 创建 / 切换时迁移委托并转发 Team；多人和分屏尚未验证 |

---

## 3. Experience 系统 TODO（8 项）

源码位置：
`Source/LyraGame/GameModes/LyraExperienceManagerComponent.cpp` 文件顶部的
Experience 生命周期 TODO 注释；判断当前行为时仍以同文件对应函数体为准。

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

| 位置 | 目标能力 | 当前状态 | 当前实现与影响 |
|---|---|---|---|
| `ULyraGameInstance::HandlerUserInitialized()` | 调用 `LoadSharedSettingsFromDisk()` | **部分复刻** | 成功后按索引调用 LocalPlayer；下游异步函数直接返回 `false`，所以登录链仍无加载结果 |
| `ULyraGameInstance` | `GetPrimaryPlayerController()` | **已复刻** | 已用 `Super::GetPrimaryPlayerController(false)` 获取并转换为 Lyra Controller；当前未发现 C++ 消费者 |
| `ULyraLocalPlayer` | Team、设置和音频本地桥梁 | **部分复刻** | Team 转发与设置缓存存在；异步加载、设置应用和音频事件生产者缺失 |
| `ULyraSettingsShared` | 每用户 SaveGame（存档）设置 | **部分复刻** | `SharedGameSettings` 同步创建 / 加载存在；无字段、版本、Apply、Save，异步函数是占位 |
| `ULyraSettingsLocal` | 机器级设置与事件 | **结构占位** | `Get()`、Editor CVar、空 Experience 回调、恒 false Replay 判断和无生产者音频 Event |
| `ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()` | 通知 `ULyraSettingsLocal::OnExperienceLoaded()` | **结构占位** | `#if !UE_SERVER` 调用已启用，但目标函数为空 |

> ⚠️ **注意：** `ULyraSettingsLocal` 的 Event 宏把 Owner 类型写成
> `ULyraSettingsLocall`。UE 5.7.4 当前宏实现只声明 friend，不强制 Owner
> 行为，因此不能仅凭该拼写断言必然编译失败；但它与真实类名及 Lyra 声明不
> 一致，应在实现广播前修正。

---

## 6. 玩家出生/重生系统接入

| 位置 | 当前状态 | 当前实现 | 后续工作 |
|---|---|---|---|
| `ALyraGameMode` ↔ `ULyraPlayerSpawningManagerComponent` | **部分复刻** | GameMode 已代理出生点选择、重生许可和重生完成钩子 | 扩展具体 Experience 的出生规则并验证组件注入 |
| `ULyraPlayerSpawningManagerComponent::ControllerCanRestart()` | **结构占位** | TODO，当前始终返回 true | 接入死亡状态、比赛状态、队伍 / 观战规则 |
| `ALyraPlayerStart::StartPointTags` 驱动的选择 | **结构占位** | 字段已存在，但默认选择逻辑未读取它 | 用于队伍出生点、模式专属出生点或权重筛选 |
| `ULyraPawnExtensionComponent` | **结构占位** | 只有构造函数，没有 Pawn 初始化状态链 | 接入 PawnData、AbilitySystem、输入和初始化状态 |
| `ALyraPlayerState::SetPawnData()` | **部分复刻** | 已保存并复制 PawnData，AbilitySet 授予仍被注释 | 接入 `ULyraPawnData` 中的 AbilitySet / 初始化数据 |
| `ALyraGameMode::SpawnDefaultPawnAtTransform_Implementation()` | **部分复刻** | 已 Deferred Spawn（延迟完成生成）Pawn，Pawn Extension 初始化仍是 TODO | 在 `FinishSpawning()` 前完成 Pawn Extension 初始化 |

---

## 7. PlayerController、相机与回放

| 位置 | 当前状态 | 提交 `19e6961` 的代码证据 | 与 Lyra 原项目的主要差距 / 风险 |
|---|---|---|---|
| `ALyraPlayerController` | **部分复刻** | 已实现控制/解除控制、ASC Avatar 清理、PlayerState/队伍委托迁移、自动奔跑、力反馈过滤、观战旋转和一次性 View Target 隐藏 | `ProcessAbilityInput()`、`TryActivateAbilitiesOnSpawn()`、共享设置绑定仍被注释 |
| 观战视角复制 | **部分复刻** | 已禁用父类 `TargetViewRotation` 的 `COND_OwnerOnly` 复制，改由 PlayerState 字段以 `COND_SkipOwner` 分发 | UE 5.7.4 服务器远程 Controller 绕过 `PlayerTick()`，权威旋转来源需验证 |
| `ILyraCameraAssistInterface` 与隐藏消费者 | **部分复刻** | 接口、`OnCameraPenetratingTarget()` 和 `UpdateHiddenComponents()` 已存在 | 没有 Camera Mode（相机模式）或其他穿透生产者 |
| `ALyraPlayerCameraManager` | **结构占位** | 空子类已被 Controller 选择；可用相机行为来自 UE 父类 | `ULyraUICameraManagerComponent`、80° FOV、Pitch、`UpdateViewTarget()` 与 `DisplayDebug()` 未接入 |
| `ALyraReplayPlayerController` | **部分复刻** | 已实现 Recorder PlayerState 重绑、Pawn 跟随和 View Target 修复 | 播放跟随没有运行证据；该类按设计拒绝录制 |
| 客户端回放录制 | **未复刻** | 资格检查最终恒为 false；真实录制调用注释，Subsystem 没有对应 API | 未来派生类放开资格后还可能“只设状态便返回 true” |

> 上表是提交 `19e6961` 的专项快照，不含编译、PIE（Play In Editor，编辑器内运行）
> 或联网证据。完整调用链见
> [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)。

---

## 8. Cheat、External RPC 与剩余日志占位

### 已新增类型，但功能仍不完整

| 位置 | 当前状态 | 当前代码证据 | 差距 / 风险 |
|---|---|---|---|
| `ULyraCheatManager` | **结构占位** | 空 `UCheatManager` 子类；非发布功能路径由 PlayerController 选择并强制尝试创建 | 继承引擎基础命令，但缺少 Lyra 的 GAS、伤害 / 治疗、GameplayTag、相机和无敌扩展 |
| `ServerCheat()` / `ServerCheatAll()` | **已复刻** | Reliable Server RPC 已实现；Shipping 中执行体为空路径，UFUNCTION 声明仍存在 | `_Validate()` 恒 true；非发布联网环境缺少白名单和额外权限校验 |
| `ULyraGameplayRpcRegistrationComponent` | **结构占位** | 空 `UExternalRpcRegistrationComponent` 子类 | 没有单例、实例、Route 或 Handler，当前不提供 Lyra HTTP 自动化端点 |
| `ALyraPlayerController::BeginPlay()` 的 External RPC 链 | **部分复刻** | 非发布功能路径调用 `StartAllListeners()` 并解析 `rpcport` | 只启动已有 Listener；注册调用被注释，端口值没有交给注册子类 |

### 日志声明与业务实现必须分开判断

以下头文件本身只声明 Log Category（日志分类）；对应 `.cpp` 至少定义日志
符号，但业务类型是否存在必须继续检查其他文件，不能从日志名推断：

| 文件 | 日志分类 | 说明 |
|------|---------|------|
| `Settings/LyraGameSettingRegistry.h` | `LogLyraGameSettingRegistry` | 当前项目没有同名 Registry 类；通用注册能力来自 GameSettings 插件 |
| `System/LyraReplicationGraph.h` | `LogLyraRepGraph` | `.cpp` 只定义日志分类，当前没有 Lyra ReplicationGraph（复制图谱）类 |
| `AbilitySystem/Phases/LyraGamePhaseLog.h` | `LogLyraGamePhase` | `.cpp` 只定义日志分类，当前没有 Game Phase（游戏阶段）业务类 |
| `LyraEditor/LyraEditor.h` | `LogLyraEditor` | 头文件只有日志声明；`FLyraEditorModule` 实际定义在 `LyraEditor.cpp`，`ULyraEditorEngine` 位于独立文件 |

---

## 9. Gameplay Message / Cue 转换 TODO

| 位置 | 当前状态 | 当前实现 | 后续工作 |
|---|---|---|---|
| `FLyraVerbMessage` | **已复刻** | 通用 Payload（载荷）与 `ToString()` 已实现 | 接入真实 Damage、Elimination、Assist 和 UI 生产者 / 消费者 |
| `ULyraVerbMessageHelpers::VerbMessageToCueParameters()` | **部分复刻** | 已映射 Verb、Instigator、Target、Source / Target Tags 和 Magnitude | 补充 `ContextTags` 到 `FGameplayCueParameters` 的映射 |
| `ULyraVerbMessageHelpers::CueParametersToVerbMessage()` | **部分复刻** | 已从 Cue 参数还原基础消息字段 | 补充 `ContextTags` 还原 |
| `ALyraGameState` 的两种消息 Multicast | **已复刻** | Reliable / Unreliable 路径都能转入客户端 `UGameplayMessageSubsystem` | 按事件持久性和频率选择可靠性，不能用 RPC 保存持续状态 |

---

## 10. 编辑器

| 问题 | 说明 |
|------|------|
| `ULyraEditorEngine::FirstTickSetup()` | 强制 Content Browser 显示插件文件夹的代码"doesn't really work due to engine iteration" — 作为钩子点示例 |

---

## 已识别的 TODO（按来源）

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 当前源码 TODO | 完成 Pawn Extension → ASC Avatar → AbilitySet 初始化链 | GameMode、PlayerState 和 Controller 中均有注释或 TODO | PawnData、`ULyraPawnExtensionComponent`、`ULyraAbilitySet` |
| 高 | 当前源码 TODO | 接回 `ProcessAbilityInput()` 与 `TryActivateAbilitiesOnSpawn()` | `ALyraPlayerController.cpp` 两处注释 | ASC 输入缓存与 Ability 授予 |
| 高 | 验证 TODO | 验证远程玩家观战旋转的服务器权威写入 | UE 5.7.4 远程 Controller 绕过 `PlayerTick()` | Dedicated Server、两个客户端、观战入口 |
| 高 | Lyra 对比 TODO | 完成 GameplayCue Manager 与全局 GE 消费链 | `InitializeGameplayCueManager()` 为空，GameData 只有引用 | Ability / Effect / Attribute 基础实现 |
| 高 | 当前源码 TODO | 实现 Shared Settings 异步加载、设置字段、Apply / Save | 登录入口已接通，但异步函数返回 `false`，类中没有可应用字段 | `ULocalPlayerSaveGame` API 与项目设置范围 |
| 中 | Lyra 对比 TODO | 实现 Local Settings 的 Experience 重应用与音频 Event 生产者 | 调用 / 事件消费者存在，目标行为与生产者缺失 | Device Profile、Audio Mixer 和设置 UI |
| 中 | 当前源码 TODO | 完善 Experience 异步失败、停用和清理 | Experience 源码 TODO 与现有 `check` 路径 | GameFeature Action 生命周期测试 |
| 中 | Lyra 对比 TODO | 增加 Camera Mode 穿透生产端和 UI Camera | 当前只有接口消费者与空 Manager | Camera Component、UI Camera |
| 中 | Lyra 对比 TODO | 实现客户端 Replay 录制 | 资格恒 false，Subsystem API 缺失 | 本地设置与回放清理策略 |
| 中 | 验证 TODO | 验证 Replay Seek / Checkpoint / Pawn 更换 | 播放跟随仅有静态代码证据 | 可用 Replay 文件 |
| 中 | 验证 TODO | 验证 Spawn Manager 注入和委托清理边界 | 组件不固定创建，且没有显式 EndPlay 解绑 | Experience 资产、多 PIE / 地图切换 |
| 中 | 验证 TODO | 验证非发布 Cheat RPC 权限 | `_Validate()` 恒 true | 受控联网测试环境 |
| 低 | Lyra 对比 TODO | 决定是否继续 External RPC | 当前只有结构占位，可能不属于主线 | 外部自动化需求与安全设计 |
| 低 | 文档 TODO | 核对 `B_LyraGameMode`、Experience 和 GameFeature 资产覆盖 | 二进制资产无法由文本源码完全确认 | Unreal Editor 资产检查 |

---

## 开发建议

在继续推进 Lyra 复刻时，建议按以下优先级：

1. **GAS 集成** — 在 GameState/PlayerState ASC 的基础上继续接入 Character、AbilitySet 授予、GameplayCue 管理，让 `ULyraGameData` 中的 GE 引用发挥作用
2. **Pawn 初始化链** — 完成 `ULyraPawnExtensionComponent` 和 PlayerState PawnData/AbilitySystem 与 Pawn 的协作
3. **重生规则扩展** — 让 `ControllerCanRestart()`、`StartPointTags` 和 Experience 自定义出生规则发挥作用
4. **Experience 系统完善** — 特别关注异步反激活 (#4) 和 GameFeature 停用 (#6) 的 TODO
5. **Experience Ready 时序整理** — 把 `UAsyncAction_ExperienceReady` 的下一帧扰动策略统一到 Experience 加载系统
6. **Gameplay Message 落地** — 把 `FLyraVerbMessage` 接入伤害、淘汰、助攻、UI 通知和 GameplayCue 转换链
7. **团队系统** — PlayerState、PlayerController 与 LocalPlayer 已接入；继续接入 Character 的 `ILyraTeamAgentInterface` 并验证多人 / 分屏委托迁移
8. **玩家设置加载** — 入口已经解除注释；下一步实现 Shared Settings 异步加载、字段、Apply / Save，以及 Local Settings 的 Experience 重应用
9. **相机系统** — 补齐 Camera Mode 穿透检测生产端、UI Camera 组件和 Lyra 相机管理器重写
10. **回放与自动化** — 先补齐客户端录制 Subsystem（子系统）和真实录制调用，再决定是否开放 External RPC 路由；开放前设计鉴权、命令白名单与生命周期注销

---

## 快速回顾

- **主线缺口：** Pawn Extension、ASC Avatar、AbilitySet 和 GAS 输入。
- **网络高风险验证：** 远程玩家观战旋转、Cheat RPC、PlayerState 延迟到达。
- **结构占位：** Local Settings 行为、Shared Settings 异步端点、
  PlayerCameraManager、Lyra Cheat Manager、External RPC。
- **明确未复刻：** 客户端 Replay 录制。
- **资产待确认：** GameMode 类覆盖、Spawn Manager 注入、蓝图 Camera 调用者。

## 复习要点

1. “结构占位”和“部分复刻”的区别是什么？
2. 为什么父类有功能不能让空 Lyra 子类标记为“已复刻”？
3. 当前玩家 GAS 主链具体断在哪四个节点？
4. 哪些缺口来自源码 TODO，哪些来自 Lyra 对比？
5. 哪些结论必须通过 Dedicated Server 或 Replay 运行验证？
6. External RPC 为什么不是当前玩家主链的高优先级前置？
7. 资产或蓝图未验证时应使用什么状态？
8. 当前用户登录设置链已经接入到哪里，又准确停止在哪里？

## 关联框架

- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 状态机与 Experience Ready 节点
- [03-System-Framework.md](03-System-Framework.md) — InitializeGameplayCueManager 空实现
- [06-Character-Framework.md](06-Character-Framework.md) — Character 中的注释掉接口
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerState 的 ASC、PawnData、Team/Squad 与 StatTag 接入状态
- [11-Development-Tools.md](11-Development-Tools.md) — Cheat Manager 与 External RPC 的权限、构建和生命周期边界
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — 提交 `19e6961` PlayerController 主链的代码证据与验证方案
- [08-UI-Framework.md](08-UI-Framework.md) — LocalPlayer、本地/共享设置、SaveGame 所有权与登录加载差异
