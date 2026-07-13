# 17 - 引擎生命周期函数参考

> 项目中所有被重写的引擎生命周期函数详解。对每个函数说明：**引擎何时调用**、**适合写什么逻辑**、**当前 Lyra 做了什么**、**开发注意事项**。

---

## 目录

1. [引擎与编辑器生命周期](#1-引擎与编辑器生命周期)
2. [GameInstance 生命周期](#2-gameinstance-生命周期)
3. [AActor 生命周期（Character / HUD 等）](#3-aactor-生命周期)
4. [AController 生命周期](#4-acontroller-生命周期)
5. [UActorComponent 生命周期](#5-uactorcomponent-生命周期)
6. [UObject 生命周期（设置/编辑器）](#6-uobject-生命周期)
7. [AssetManager 生命周期](#7-assetmanager-生命周期)
8. [Subsystem 生命周期](#8-subsystem-生命周期)
9. [UDataAsset / UPrimaryDataAsset](#9-udataasset--uprimarydataasset)
10. [模块加载生命周期](#10-模块加载生命周期)
11. [游戏会话生命周期](#11-游戏会话生命周期)
12. [AWorldSettings](#12-aworldsettings)
13. [UGameViewportClient](#13-ugameviewportclient)
14. [AGameModeBase](#14-agamemodebase)
15. [AGameStateBase](#15-agamestatebase)
16. [APlayerState](#16-aplayerstate)
17. [UBlueprintAsyncActionBase](#17-ublueprintasyncactionbase)

---

## 1. 引擎与编辑器生命周期

### UEngine::Init(IEngineLoop* InEngineLoop)

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraGameEngine::Init()` / `ULyraEditorEngine::Init()` |
| **调用时机** | 引擎启动最早期的阶段。在所有模块加载完毕、UObject 系统初始化之后，但在任何 World 创建之前。编辑器路径和游戏路径都会走到这里。 |
| **调用次数** | 整个进程生命周期中仅调用一次 |
| **此时可用** | UObject 系统 ✓、模块系统 ✓、Slate ✗、World ✗、Actor ✗ |
| **适合写的逻辑** | — 项目级引擎初始化（例如注册自定义委托、修改引擎全局设置）<br>— 在 Super::Init 之前：极少需要，仅当需要在引擎自身初始化前修改某些静态配置<br>— 在 Super::Init 之后：大部分项目初始化逻辑的位置<br>— ⚠️ 不要在这里访问任何 World 或 Actor |
| **Lyra 行为** | 仅调用 `Super::Init(InEngineLoop)`，透传无额外逻辑。这是一个预留的扩展点。 |
| **注意事项** | 如果在这里做耗时操作，会影响引擎启动时间。优先考虑懒加载，而非在 Init 中做所有事。 |

---

### UEngine::Start()

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraEditorEngine::Start()`（仅编辑器） |
| **调用时机** | 在 `Init()` 之后，引擎准备开始主循环之前。 |
| **调用次数** | 一次 |
| **此时可用** | UObject 系统 ✓、模块 ✓、Slate ✓（编辑器） |
| **适合写的逻辑** | — 编辑器 UI 注册（如注册菜单扩展、工具栏按钮）<br>— 启动时需要显示的 UI 初始化<br>— 日志输出确认引擎版本/配置已加载 |
| **Lyra 行为** | 调用 Super::Start() 后输出日志 |
| **注意事项** | `UGameEngine::Start()` 在这个阶段加载全局地图。`UEditorEngine::Start()` 则创建编辑器环境。Lyra 只在 `ULyraEditorEngine` 中重写，`ULyraGameEngine` 未重写。 |

---

### UEngine::Tick(float DeltaSeconds, bool bIdleMode)

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraEditorEngine::Tick()`（仅编辑器） |
| **调用时机** | 引擎主循环中每帧调用。`bIdleMode` 表示编辑器是否处于空闲状态（无焦点、无 PIE）。 |
| **调用次数** | 每帧（~60fps = 每 16ms） |
| **此时可用** | 所有系统都已就绪 |
| **适合写的逻辑** | — 编辑器全局状态轮询<br>— 延迟一次性初始化（如 Lyra 的 FirstTickSetup）<br>— ⚠️ 避免在这里做耗时操作，会阻塞编辑器 UI 响应 |
| **Lyra 行为** | 每帧调用 `FirstTickSetup()`（内部通过 `bFirstTickSetup` 布尔标志保证只在第一帧执行一次），强制 Content Browser 显示插件文件夹。 |
| **注意事项** | 不要重写游戏的 Tick —— 游戏逻辑应放在 Actor/Component 的 Tick 中。编辑器 Tick 仅用于编辑器全局行为。 |

---

### UEditorEngine::PreCreatePIEInstances(...)

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraEditorEngine::PreCreatePIEInstances()` |
| **调用时机** | 用户点击 PIE（Play In Editor）后，在 PIE 实例真正创建**之前**。此时还没加载 PIE 地图，还没有 PIE World。 |
| **调用次数** | 每次 PIE 启动时调用一次 |
| **此时可用** | 编辑器 World ✓、编辑器设置 ✓、PIE World ✗ |
| **适合写的逻辑** | — 修改 PIE 网络模式（如强制 Standalone）<br>— 根据关卡/项目设置决定 PIE 参数<br>— 通知其他系统 PIE 即将开始（应用开发设置、平台模拟）<br>— 阻止特定条件下的 PIE 启动（通过返回值）<br>— 修改 `InNumOnlinePIEInstances` 参数 |
| **Lyra 行为** | 1. 检查 `ALyraWorldSettings::ForceStandaloneNetMode` → 强制 PIE 为 Standalone 模式并弹通知<br>2. 调用 `ULyraDeveloperSettings::OnPlayInEditorStarted()` — 若 ExperienceOverride 有效则弹通知<br>3. 调用 `ULyraPlatformEmulationSettings::OnPlayInEditorStarted()` — 平台模拟配置变更通知<br>4. 最后调用 `Super::PreCreatePIEInstances(...)` 返回结果。⚠️ 自定义处理在 Super 之前执行。 |
| **注意事项** | 这是编辑器开发中最常用的钩子之一。典型模式：在 DeveloperSettings 中配置调试选项，在 PreCreatePIEInstances 中应用它们。 |

---

## 2. GameInstance 生命周期

### UGameInstance::Init()

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraGameInstance::Init()` |
| **调用时机** | GameInstance 被创建时调用。在引擎启动的 `UGameEngine::Init()` → `UGameInstance::InitializeStandalone()` 中触发，或由无缝旅行/网络连接触发。 |
| **调用顺序** | 在游戏的第一个 World 创建**之前**，也在任何 Actor 的 BeginPlay 之前 |
| **调用次数** | 每个 GameInstance 一次 |
| **此时可用** | Subsystem ✓、World ✗、Actor ✗ |
| **适合写的逻辑** | — 注册 Subsystem（通常由引擎自动处理）<br>— 初始化全局状态（如注册 InitState 状态转移）<br>— 绑定网络/旅行相关委托<br>— 准备加密密钥<br>— 初始化项目级设置 |
| **Lyra 行为** | 1. 向 `UGameFrameworkComponentManager` 注册 InitState 状态转移链（Spawned → DataAvailable → DataInitialized → GameplayReady）——这是所有 Modular Actor 初始化的基础设施<br>2. 生成调试加密密钥<br>3. 绑定 `OnPreClientTravelToSession` 委托 |
| **注意事项** | ⚠️ 必须调用 `Super::Init()`！否则引擎的 GameInstance 初始化不会完成。 |

---

### UGameInstance::Shutdown()

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraGameInstance::Shutdown()` |
| **调用时机** | GameInstance 被销毁时。进程关闭、退出到桌面、或引擎正常关闭时调用。 |
| **调用次数** | 一次 |
| **此时可用** | 大部分系统仍可用，但 World 可能已部分销毁 |
| **适合写的逻辑** | — 清理 Init 中分配的资源<br>— 保存持久化状态<br>— 解除绑定的委托 |
| **Lyra 行为** | 调用 Super::Shutdown()，当前无额外清理逻辑 |
| **注意事项** | Shutdown 和析构函数不同——Shutdown 发生得更早，可以安全地访问 UObject 系统。 |

---

### UGameInstance::ReceivedNetworkEncryptionToken / ReceivedNetworkEncryptionAck

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraGameInstance::ReceivedNetworkEncryptionToken()` / `ReceivedNetworkEncryptionAck()` |
| **调用时机** | 客户端连接服务器时，在网络加密握手阶段调用。Token 由服务器发送，客户端用 Ack 确认。 |
| **调用次数** | 每次连接握手时 |
| **适合写的逻辑** | — 处理 DTLS/加密握手<br>— 验证服务器身份<br>— 设置加密密钥 |
| **Lyra 行为** | 使用硬编码测试密钥实现 DTLS 加密握手（仅用于测试，非生产安全）。由 CVar `Lyra.TestEncryption` 和 `Lyra.UseDTLSEncryption` 控制。 |
| **注意事项** | Lyra 的实现有 `DebugTestEncryptionKey` —— 仅用于本地测试，生产环境需要真正的密钥交换。 |

---

### UCommonGameInstance::HandlerUserInitialized(...)

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraGameInstance::HandlerUserInitialized()` |
| **调用时机** | CommonUser 框架中，用户登录成功后回调。这是异步回调，不在固定的引擎帧中。 |
| **调用次数** | 每次用户成功登录时 |
| **适合写的逻辑** | — 为用户加载本地设置（`LoadSharedSettingsFromDisk`）<br>— 初始化用户特定的数据<br>— 应用用户偏好（语言、控制方案等） |
| **Lyra 行为** | 调用 Super 后，有注释掉的代码：原本计划调用 `LyraLocalPlayer::LoadSharedSettingsFromDisk()`，等待自定义 LocalPlayer 实现。 |
| **注意事项** | 此函数来自 CommonUser 插件，不是原生 UE。必须在调用 Super::HandlerUserInitialized 之后执行自定义逻辑。 |

---

### UGameInstance::CanJoinRequestedSession() const

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraGameInstance::CanJoinRequestedSession()` |
| **调用时机** | 玩家尝试通过会话系统加入在线/局域网游戏时，引擎查询此函数以决定是否允许加入。 |
| **调用次数** | 每次尝试加入会话时 |
| **适合写的逻辑** | — 检查游戏版本兼容性<br>— 检查用户权限/登录状态<br>— 检查是否已在游戏中 |
| **Lyra 行为** | 始终返回 `true`（占位实现），不做任何限制。 |
| **注意事项** | 生产环境应实现实际检查逻辑。返回 false 会阻止玩家加入会话。 |

---

## 3. AActor 生命周期

> `ALyraHUD`、`ALyraGameState` 和 `ALyraPlayerState` 重写了以下 Actor 生命周期函数。`ALyraCharacter` 当前仍主要依赖 Modular 组件注入或 Experience Action 添加自定义行为。

### AActor::PreInitializeComponents()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraHUD::PreInitializeComponents()` / `ALyraGameState::PreInitializeComponents()` / `ALyraPlayerState::PreInitializeComponents()` |
| **调用时机** | Actor 生成（Spawn）流程中，在组件初始化之前、`BeginPlay()` 之前调用。 |
| **调用顺序** | 构造函数 → **PreInitializeComponents** → 组件 Initialize → PostInitializeComponents → BeginPlay |
| **调用次数** | 每个 Actor 一次 |
| **此时可用** | Actor 的默认子对象已创建，但组件还没有完成初始化，也尚未 BeginPlay |
| **适合写的逻辑** | — 在组件 BeginPlay 之前对组件进行最后配置<br>— 设置组件之间的依赖关系<br>— 初始化需要在 BeginPlay 之前就绪的状态 |
| **Lyra 行为** | `ALyraHUD`、`ALyraGameState` 和 `ALyraPlayerState` 在此处调用 Super，无额外逻辑 |
| **注意事项** | 此时组件还未调用它们自己的 BeginPlay。如果你需要组件已完全初始化，考虑在 BeginPlay 中操作。 |

---

### AActor::PostInitializeComponents()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraGameState::PostInitializeComponents()` / `ALyraPlayerState::PostInitializeComponents()` |
| **调用时机** | Actor 组件创建、注册和初始化后，`BeginPlay()` 之前。 |
| **调用顺序** | PreInitializeComponents → 组件 Initialize → **PostInitializeComponents** → BeginPlay |
| **调用次数** | 每个 Actor 一次 |
| **适合写的逻辑** | — 初始化依赖组件已存在的运行时状态<br>— 绑定需要在 BeginPlay 前就绪的委托<br>— 初始化 AbilitySystem ActorInfo |
| **Lyra 行为** | `ALyraGameState` 初始化比赛级 ASC：`InitAbilityActorInfo(this, this)`；`ALyraPlayerState` 初始化玩家级 ASC：`InitAbilityActorInfo(this, GetPawn())`，并在服务器 GameWorld 中注册 Experience Loaded 回调来设置 PawnData。 |
| **注意事项** | `ALyraPlayerState` 在这里监听 Experience 状态，但不会直接加载 Experience；真正的加载仍由 `ULyraExperienceManagerComponent` 驱动。 |

---

### AActor::BeginPlay()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraHUD::BeginPlay()` |
| **调用时机** | Actor 初始化流程的最后一步。在所有组件的 BeginPlay 完成后调用。 |
| **调用顺序** | PreInitializeComponents → PostInitializeComponents → 所有组件的 BeginPlay → **Actor 的 BeginPlay** |
| **调用次数** | 每个 Actor 一次（但可能在关卡流式加载/卸载时多次触发） |
| **此时可用** | 所有组件已注册并 BeginPlay ✓、World ✓、其他 Actor ✓（但不保证顺序） |
| **适合写的逻辑** | — 游戏逻辑初始化（最常见的入口点）<br>— 获取其他 Actor/组件的引用<br>— 绑定委托<br>— 生成子 Actor<br>— 设置初始状态<br>— ⚠️ 不要假设其他 Actor 的初始化顺序 |
| **Lyra 行为** | `ALyraHUD` 在此处调用 Super，无额外逻辑 |
| **注意事项** | 这是最常用的游戏逻辑初始化函数。如果你需要在所有 Actor 都完成初始化之后再执行逻辑，使用 `SetTimer` 延迟一帧，或依赖 Experience 加载完成的委托。 |

---

### AActor::EndPlay(const EEndPlayReason::Type EndPlayReason)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraHUD::EndPlay()` / `ALyraGameState::EndPlay()` / `ULyraExperienceManagerComponent::EndPlay()`（Component） |
| **调用时机** | Actor 被销毁或关卡被卸载时。在组件被销毁**之前**。 |
| **调用次数** | 每个 Actor 一次 |
| **参数说明** | `EndPlayReason` 枚举说明销毁原因：`Destroyed`（手动删除）、`LevelTransition`（关卡切换）、`EndPlayInEditor`（编辑器停止）、`RemovedFromWorld`（流式卸载）、`Quit`（退出） |
| **适合写的逻辑** | — 清理 BeginPlay 中创建的资源<br>— 解绑委托<br>— 保存持久化状态<br>— 优雅地停止正在进行的异步操作<br>— 根据 `EndPlayReason` 做不同处理（关卡切换 vs 退出） |
| **Lyra 行为** | — `ALyraHUD`: 调用 Super<br>— `ALyraGameState`: 调用 Super，保留 GameState 清理扩展点<br>— `ULyraExperienceManagerComponent`: 复杂的反激活逻辑（卸载插件引用计数检查 → 执行 Action 的 Deactivating/Unregistering → 异步反激活跟踪） |
| **注意事项** | 对于 Component 重写：Component 的 EndPlay 早于其 Owner Actor 的 EndPlay。这是 Experience 卸载逻辑的触发点。 |

---

### AHUD::GetDebugActorList(TArray<AActor*>& InOutList)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraHUD::GetDebugActorList()` |
| **调用时机** | 当显示调试信息时（如 `show debug` 控制台命令或引擎的调试绘制系统）。 |
| **调用次数** | 每次刷新调试显示时 |
| **适合写的逻辑** | — 将项目特定的 Actor 添加到调试列表中以显示其调试信息<br>— 在屏幕上显示自定义 Actor 的调试文字 |
| **Lyra 行为** | 调用 Super 后在列表中添加 Lyra 特定的 Actor |
| **注意事项** | 性能影响较小（且仅在调试时调用），可以放心添加 Actor。 |

---

## 4. AController 生命周期

> 当前工作区的 `ALyraPlayerController` 已重写 Actor（Actor 对象）、Controller（控制器）和 PlayerController（玩家控制器）的多组钩子，并加入回放、相机隐藏、作弊与 HTTP 自动化协作。以下说明以 UE 5.7.4 源码和全部未提交代码为准；尚未运行验证。

### APlayerController::PostInitializeComponents()

| 项目 | 说明 |
|------|------|
| **UE 5.7.4 入口** | Actor 组件初始化完成后调用。服务器先 `InitPlayerState()`，随后服务器与拥有者客户端通过 `SpawnPlayerCameraManager()` 生成 `PlayerCameraManagerClass` 指定的相机管理器，重置 Camera Mode（相机模式）；客户端再生成 HUD，最后调用 `AddCheats()`。 |
| **当前项目行为** | 构造函数选择 `ALyraPlayerCameraManager`；非发布构建选择 `ULyraCheatManager`。`InitPlayerState()` 的 Lyra 重写会迁移 PlayerState/队伍委托，`AddCheats()` 在宏启用时强制把请求传给父类。 |
| **阅读结论** | `ALyraPlayerCameraManager` 虽无自定义实现，仍继承 `APlayerCameraManager` 的基础相机功能；当前缺少的是 Lyra 原项目的 UI Camera（界面相机）、80° FOV（视野角）与调试扩展。 |
| **注意事项** | 类被生成只能证明引擎相机链存在，不能证明 Camera Mode 穿透检测、UI 相机接管或 Lyra 调试显示已经复刻。 |

### AController::OnPossess(APawn* InPawn)

| 项目 | 说明 |
|------|------|
| **UE 5.7.4 入口** | `AController::Possess()` 先检查网络权威（Authority），再调用 `OnPossess()`；基础实现负责解除旧 Pawn、调用 `Pawn::PossessedBy()`、设置 Pawn、控制旋转并触发 Restart。 |
| **当前项目行为** | `ALyraPlayerController::OnPossess()` 先调用 `Super`，再在编辑器服务器条件下执行配置的作弊命令，并关闭自动奔跑。 |
| **调用时机** | 正常 `Possess()` 路径由拥有网络权威的一端发起；客户端通常通过 Pawn/控制器复制、`ClientRestart()` 等路径获得控制结果，不能把“客户端看见 Pawn”简单等同于“客户端执行了 OnPossess”。 |
| **适合写的逻辑** | 在 `Super` 之后处理“已拥有新 Pawn”这一稳定状态：清理控制器临时状态、应用只属于该 Pawn 的控制器规则。 |
| **注意事项** | 在 `Super` 前 `GetPawn()` 仍可能是旧 Pawn；不要在此重复设置 ASC Avatar 或复制关系，除非先说明与基础实现的顺序关系。 |

### 当前 PlayerController 其余关键钩子

| 钩子 | UE 5.7.4 底层职责 | 当前项目行为 | 阅读结论 |
|------|------------------|--------------|----------|
| `BeginPlay()` | Actor 完成初始化并进入游戏；父类推进通用 BeginPlay | 非发布构建先调用 `StartAllListeners()` 并解析 `rpcport`，但注册对象/路由调用被注释；最后取消隐藏 Controller Actor | HTTP Server（HTTP 服务器）模块只会启动已有 Listener（监听器）；当前没有 Lyra Endpoint（端点） |
| `GetLifetimeReplicatedProps()` | 父类把 `TargetViewRotation` 和 `SpawnLocation` 以 `COND_OwnerOnly` 注册 | 使用 `DISABLE_REPLICATED_PROPERTY` 禁用继承的 `TargetViewRotation` | 旋转数据改走 PlayerState 的 `ReplicatedViewRotation`；不是“继续复制默认值” |
| `OnUnPossess()` | `AController::UnPossess()` 先进入此钩子；基础实现会让 Pawn 解除控制并清空 `Pawn` 指针 | 在调用 `Super` 前，若 ASC Avatar 正是旧 Pawn 则 `SetAvatarActor(nullptr)` | 先清 Avatar、后清 Pawn 的顺序是必要的；OwnerActor 仍可保持为 PlayerState |
| `InitPlayerState()` / `CleanupPlayerState()` | 引擎创建或销毁 PlayerState；`CleanupPlayerState()` 会销毁并清空它 | 在 `Super` 后广播 PlayerState 改变，迁移队伍变化委托 | 所有保存 PlayerState 指针的系统都必须处理“旧对象解绑 → 新对象绑定” |
| `OnRep_PlayerState()` | 基类调用 `PlayerState->ClientInitialize(this)` | 广播改变；仅在客户端刷新 Lyra ASC 的 ActorInfo | 解决复制到达顺序问题的补偿路径已存在，但出生技能激活仍未接入 |
| `PlayerTick()` | 本地控制路径在存在 `PlayerInput` 时执行；服务器非本地 Autonomous Proxy（自治代理）Controller 走 `TickActor()` 的另一分支 | 追加自动奔跑输入，以及观战视角旋转写入/读取 | `HasAuthority()` 分支只对实际进入 `PlayerTick()` 的对象生效；当前没有第二处 `SetReplicatedViewRotation()` 调用，远程玩家服务器旋转来源需验证 |
| `SetPlayer()` | 绑定本地或网络 `UPlayer`，随后会进入 `ReceivedPlayer()` 等初始化 | 父类行为存在，但 Shared Settings（共享设置）的委托绑定与首次应用被注释 | `bForceFeedbackEnabled` 当前沿用引擎默认值，不受 Lyra 共享设置驱动 |
| `UpdateForceFeedback()` | 每帧把计算后的通道值交给输入设备 | 仅在启用且输入类型为 Gamepad（手柄）/Touch（触摸）或 CVar 强制开启时输出，否则写零 | 设备过滤静态接入；仍需真机验证 |
| `UpdateHiddenComponents()` | `ULocalPlayer::CalcSceneView()` 调用 `BuildHiddenComponentList()`，其末尾进入此虚函数 | 一次性隐藏当前 View Target 及未标记 `NoParentAutoHide` 的直接附加图元，然后重置标志 | 消费链已实现；当前没有相机穿透代码调用 `OnCameraPenetratingTarget()` 设置标志 |
| `PostProcessInput()` | 基类会在忽略视角输入时清空 `RotationInput` | ASC 输入处理仍注释，随后调用 `Super` | 未来若接入 `ProcessAbilityInput()`，应保持“能力输入处理”和 `Super` 的顺序与原项目一致，并测试暂停状态 |
| `ALyraReplayPlayerController::Tick()` | 普通 Actor Tick（逐帧更新） | 当被跟随 PlayerState 失效时，重新绑定 GameState 录制者事件并恢复 View Target | 与 Lyra 原项目的回放播放跟随逻辑静态对齐；不代表客户端录制已可用 |

> 从 `Possess()` 到 ASC Avatar 切换、以及 PlayerState 复制补刷新的完整主调用链见 [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)。

### AController::SetupInputComponent()

| 项目 | 说明 |
|------|------|
| **Lyra 状态** | **未重写** |
| **调用时机** | Controller 控制 Pawn 后，在 `OnPossess` 之后、`BeginPlay` 之后。由 `APlayerController::SpawnPlayerInputComponent` 触发。 |
| **适合写的逻辑** | — 绑定输入 Action/Key 到处理函数（最常见的用途）<br>— 配置输入映射上下文（Enhanced Input）<br>— ⚠️ 必须调用 Super::SetupInputComponent() |
| **注意事项** | Lyra 使用 Enhanced Input 系统，输入映射通常通过 Experience 的 GameFeatureAction 添加，而非直接在 Controller 中绑定。 |

---

## 5. UActorComponent 生命周期

### UActorComponent::InitializeComponent()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraPlayerSpawningManagerComponent::InitializeComponent()` |
| **调用时机** | Component 注册并完成基础初始化时调用，早于 BeginPlay。要求组件 `bWantsInitializeComponent = true`。 |
| **调用次数** | 每个组件实例通常一次；组件重新注册时可能再次经历初始化流程 |
| **适合写的逻辑** | — 缓存同 World 中已有 Actor<br>— 绑定 World/Engine 级委托<br>— 建立运行时索引或管理器状态<br>— ⚠️ 如果绑定全局委托，应考虑对应的解绑路径 |
| **Lyra 行为** | `ULyraPlayerSpawningManagerComponent` 绑定 `LevelAddedToWorld` 与 `OnActorSpawned`，并遍历当前 World 中所有 `ALyraPlayerStart` 缓存到 `CachedPlayerStarts`。 |
| **注意事项** | 这个组件依赖 `ALyraPlayerStart` 缓存来避免每次出生都全图搜索；流式关卡和动态生成出生点会通过委托继续补充缓存。 |

---

### UActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction*)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraPlayerSpawningManagerComponent::TickComponent()` |
| **调用时机** | 组件 Tick 启用后每帧调用。 |
| **适合写的逻辑** | — 需要按帧更新的组件状态<br>— 定时轮询、调试可视化或渐进式处理 |
| **Lyra 行为** | 当前仅调用 `Super::TickComponent()`；构造函数中 `bCanEverTick = true`，但 `bStartWithTickEnabled = false`，默认不会开始 Tick。 |
| **注意事项** | 当前出生点缓存由事件驱动，不依赖 Tick。若后续启用 Tick，应避免在 Dedicated Server 上做不必要的全图扫描。 |

---

### UActorComponent::EndPlay(const EEndPlayReason::Type)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraExperienceManagerComponent::EndPlay()` |
| **调用时机** | Component 被销毁时。可能早于其 Owner Actor 的 EndPlay。 |
| **调用顺序** | Component::EndPlay → Actor::EndPlay |
| **适合写的逻辑** | — 清理 Component 特有的资源<br>— 取消异步操作<br>— 通知关联系统停止服务<br>— ⚠️ 此时 Owner Actor 仍存在，可以安全访问 |
| **Lyra 行为** | 触发 Experience 卸载流程：遍历 GameFeaturePluginURLs → 引用计数检查 → 卸载插件 → 执行 Action 反激活 → 异步反激活跟踪。这是整个 Experience 系统生命周期中最复杂的函数。 |
| **注意事项** | `UGameStateComponent` 继承自 `UActorComponent`。由于 GameState 的生命周期通常与关卡相同，这个 EndPlay 在关卡切换或游戏退出时触发。 |

---

### ILoadingProcessInterface::ShouldShowLoadingScreen(FString& OutReason) const

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraExperienceManagerComponent::ShouldShowLoadingScreen()` |
| **调用时机** | 由 CommonLoadingScreen 插件或 `ULoadingScreenManager` 每帧查询，决定是否显示加载画面。 |
| **调用次数** | 每帧轮询 |
| **适合写的逻辑** | — 返回 true 并设置 OutReason 来显示加载画面<br>— 基于加载状态决定：`LoadState != Loaded` → true |
| **Lyra 行为** | 当 Experience 加载状态不为 `Loaded` 时返回 true（显示加载画面），否则返回 false。 |
| **注意事项** | 这个接口来自 `LoadingProcessInterface` 插件接口。任何实现此接口的 Component 都可以控制加载画面的显示。 |

---

### UActorComponent::GetLifetimeReplicatedProps

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraExperienceManagerComponent::GetLifetimeReplicatedProps()`（通过 UHT 隐式声明） |
| **调用时机** | 引擎在 Actor Channel 打开时调用一次，决定需要复制哪些属性。 |
| **调用次数** | 每个 Component 一次 |
| **适合写的逻辑** | — 使用 `DOREPLIFETIME` 宏注册需要复制的属性<br>— 使用 `DOREPLIFETIME_CONDITION` 设置复制条件 |
| **Lyra 行为** | 注册 `CurrentExperience` 为复制属性 |
| **注意事项** | 必须调用 `Super::GetLifetimeReplicatedProps()`。不需要在 .h 中声明 `override`——UHT 自动生成。 |

---

## 6. UObject 生命周期

> `ULyraDeveloperSettings` 重写了以下函数。这些是 `UDeveloperSettings` 和基于 Config 的类的通用模式。

### UObject::PostInitProperties()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraDeveloperSettings::PostInitProperties()` |
| **调用时机** | 对象构造完成、属性从 CDO 或 Config 文件加载后调用。在所有属性初始化完成**之后**。 |
| **适合写的逻辑** | — 基于加载的属性值做二次初始化<br>— 验证配置一致性<br>— 初始化派生数据<br>— ⚠️ 必须调用 Super::PostInitProperties() |
| **Lyra 行为** | [Editor-Only] 调用 ApplySettings() 应用开发者设置 + 弹出作弊状态提醒 |
| **注意事项** | 与构造函数不同——此时所有 Config/EditDefaultsOnly 属性已经加载完毕。是验证和应用的理想时机。 |

---

### UObject::PostEditChangeProperty(FPropertyChangedEvent&)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraDeveloperSettings::PostEditChangeProperty()` |
| **调用时机** | 用户在编辑器 Details 面板中修改一个属性后。`PropertyChangedEvent` 包含哪个属性被修改了、修改类型等信息。 |
| **调用次数** | 每次属性变更时 |
| **适合写的逻辑** | — 属性联动：修改 A 属性时自动调整 B 属性<br>— 实时验证用户输入<br>— 预览效果（如修改颜色时实时更新视口）<br>— ⚠️ 通过 `PropertyChangedEvent.MemberProperty` 判断哪个属性变了，避免不必要的更新 |
| **Lyra 行为** | [Editor-Only] 调用 ApplySettings() 应用变更 |
| **注意事项** | 在 PIE 中修改设置时不会触发——只在编辑器编辑时触发。运行时修改请用 Setter 函数。 |

---

### UObject::PostReloadConfig(FProperty*)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraDeveloperSettings::PostReloadConfig()` |
| **调用时机** | 配置文件被热重载后（如修改 .ini 后编辑器自动重载，或调用 `ReloadConfig()`）。 |
| **适合写的逻辑** | — 在 Config 热重载后重新应用设置<br>— 响应外部配置变更 |
| **Lyra 行为** | [Editor-Only] 调用 ApplySettings() |
| **注意事项** | 当多个属性通过 Config 重载变更时，每个属性触发一次 PostReloadConfig。 |

---

### UDeveloperSettings::GetCategoryName() const

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraDeveloperSettings::GetCategoryName()` / `ULyraPlatformEmulationSettings::GetCategoryName()` |
| **调用时机** | 编辑器构建 Settings 面板 UI 时调用，决定此设置类在 Project Settings 中的显示位置。 |
| **适合写的逻辑** | — 返回自定义分类名称（如 "Lyra"），让设置出现在 Project Settings 的对应分类下 |
| **Lyra 行为** | 返回 "Lyra"，所有 Lyra 开发者设置出现在 Project Settings → Lyra 面板下 |
| **注意事项** | 纯分类显示逻辑，对运行时无影响。 |

---

## 7. AssetManager 生命周期

### UAssetManager::StartInitialLoading()

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraAssetManager::StartInitialLoading()` |
| **调用时机** | 引擎启动过程中，在所有模块加载之后、第一个 World 创建之前。由 `UEngine::Init()` → `UAssetManager::Initialize` 链触发。 |
| **调用次数** | 一次 |
| **此时可用** | 文件系统 ✓、AssetRegistry ✓（可能仍在扫描）、World ✗ |
| **适合写的逻辑** | — **预加载**关键游戏数据（GameData、PawnData 等）<br>— 初始化 GameplayCue 管理器<br>— 启动游戏内容加载进度跟踪<br>— ⚠️ 这里做的同步加载会增加引擎启动时间。使用 FLyraAssetManagerStartupJob 包装以支持进度报告 |
| **Lyra 行为** | 创建启动作业列表：1. `InitializeGameplayCueManager()`（TODO 空实现）2. `LoadGameDataOfClass<ULyraGameData>()`（同步加载全局游戏数据）。记录启动耗时。 |
| **注意事项** | 必须调用 `Super::StartInitialLoading()`！DedicatedServer 不需要进度 UI——Lyra 在 DoAllStartupJobs 中处理了这一点。 |

---

### UAssetManager::PreBeginPIE(bool bStartSimulate)

| 项目 | 说明 |
|------|------|
| **Lyra 类** | `ULyraAssetManager::PreBeginPIE()` |
| **调用时机** | PIE 启动前，在 World 创建之前。`bStartSimulate` 指示是否即将开始模拟模式。 |
| **调用次数** | 每次 PIE 启动前 |
| **适合写的逻辑** | — 预加载编辑器/PIE 需要的资产<br>— 清理或重置编辑器状态<br>— 确保游戏数据在 PIE 环境中的一致性 |
| **Lyra 行为** | [Editor-Only] PIE 启动时的预加载钩子 |
| **注意事项** | `#if WITH_EDITOR` 保护——在运行时构建中不存在。 |

---

## 8. Subsystem 生命周期

### USubsystem::Initialize(FSubsystemCollectionBase& Collection) / Deinitialize()

| 项目 | 说明 |
|------|------|
| **Lyra 状态** | `ULyraUIManagerSubsystem::Initialize()` / `Deinitialize()` 已重写；`ULyraExperienceManager` 中**注释掉了**这两个重写；`ULyraReplaySubsystem` 当前未重写 Initialize/Deinitialize |
| **调用时机** | Initialize: 在 Subsystem 被创建并注册后立即调用。所有同类型 Subsystem 的 Initialize 之间没有固定顺序。<br>Deinitialize: Subsystem 被销毁时调用，在引擎关闭流程中。 |
| **适合写的逻辑** | — Initialize: 注册委托、初始化内部状态、缓存引用<br>— Deinitialize: 清理资源、解绑委托、保存状态 |
| **Lyra 行为** | `ULyraUIManagerSubsystem` 调用 Super 后注册 `FTSTicker`，每帧同步 `PrimaryGameLayout` 和 `AHUD::bShowHUD`；反初始化时移除 Tick 句柄。 |
| **注意事项** | `ULyraExperienceManager` 不依赖 Subsystem 生命周期——它的功能通过静态方法 `NotifyOfPluginActivation` / `RequestToDeactivatePlugin` 和由外部调用的 `OnPlayInEditorBegun` 来驱动。`ULyraReplaySubsystem` 当前仅提供静态能力查询，也不需要实例初始化逻辑。 |

---

## 9. UDataAsset / UPrimaryDataAsset

### UObject::IsDataValid(FDataValidationContext& Context) const

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraExperienceDefinition::IsDataValid()` / `ULyraExperienceActionSet::IsDataValid()` |
| **调用时机** | 在编辑器中手动触发数据验证（Validate Data 命令）、Cook 前检查、或 `FDataValidation` 自动化流程中。 |
| **调用次数** | 按需（仅在编辑器中） |
| **适合写的逻辑** | — 验证数据资产的配置完整性<br>— 检查必须引用的资产是否存在<br>— 检查值范围是否合理<br>— 对蓝图子类验证其继承关系是否正确 |
| **Lyra 行为** | [Editor-Only] 验证 Actions 配置是否有效，确保蓝图资产直接继承自对应的 C++ 类 |
| **注意事项** | 返回 `EDataValidationResult::Invalid` 会在编辑器中产生警告/错误标记。 |

---

### UPrimaryDataAsset::UpdateAssetBundleData()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraExperienceDefinition::UpdateAssetBundleData()` / `ULyraExperienceActionSet::UpdateAssetBundleData()` |
| **调用时机** | Asset Manager 扫描资产时调用，用于向 Asset Manager 注册间接引用的资源。 |
| **调用次数** | 每次 Asset Registry 扫描时 |
| **适合写的逻辑** | — 将数据资产间接引用的资源添加到 `AssetBundleData` 中<br>— 确保 Cook/Chunk 打包时正确包含所有依赖资源 |
| **Lyra 行为** | [Editor-Only] 将 Experience/ActionSet 间接引用的资源注册到 Asset Manager，确保打包时正确包含 |
| **注意事项** | 如果没有正确注册间接依赖，打包后的游戏中可能缺少关键资产。 |

---

## 10. 模块加载生命周期

### IModuleInterface::StartupModule()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `FLyraEditorModule::StartupModule()` / `FLyraGameModule::StartupModule()` |
| **调用时机** | 模块被引擎加载时。加载时机取决于模块的 `LoadingPhase`：`Default` 阶段在引擎核心初始化后，`PostEngineInit` 在引擎完全初始化后，`PreLoadingScreen` 最早。 |
| **适合写的逻辑** | — 注册委托（如 PIE 开始/结束）<br>— 注册编辑器扩展（工具栏按钮、菜单项）<br>— 初始化模块级单例<br>— ⚠️ 不要在 Default 阶段访问 Slate/UI 系统 |
| **Lyra 行为** | `FLyraEditorModule`: 当不作为游戏运行时（`!IsRunningGame()`），绑定 `BeginPIE` / `EndPIE` 委托到 `ULyraExperienceManager::OnPlayInEditorBegun()` |
| **注意事项** | Lyra 的游戏模块 `FlLyraGameModule` 的 StartupModule/ShutdownModule 都是空实现——仅作为标准的模块生命周期占位。 |

---

### IModuleInterface::ShutdownModule()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `FLyraEditorModule::ShutdownModule()` |
| **调用时机** | 引擎关闭时，模块卸载前调用。 |
| **适合写的逻辑** | — 解绑 StartupModule 中注册的委托<br>— 注销编辑器扩展<br>— 保存模块状态<br>— ⚠️ 此时某些系统可能已经部分销毁 |
| **Lyra 行为** | 当前为空实现 |

---

## 11. 游戏会话生命周期

### AGameSession::ProcessAutoLogin()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraGameSession::ProcessAutoLogin()` |
| **调用时机** | GameMode 初始化后，引擎尝试为本地玩家自动登录时调用。在 `AGameMode::HandleMatchIsWaitingToStart` 附近触发。 |
| **适合写的逻辑** | — 返回 false 禁用引擎默认的自动登录行为<br>— 实现自定义的玩家加入逻辑 |
| **Lyra 行为** | 返回 `true`，表示已处理自动登录（防止引擎再次执行默认流程）。实际登录逻辑在 `LyraGameMode::TryDedicatedServerLogin` 中。 |
| **注意事项** | 返回 `true` 告诉引擎"不需要再执行默认的自动登录"，与 CommonUser 框架配合使用。 |

---

### AGameSession::HandleMatchHasStarted() / HandleMatchHasEnded()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraGameSession::HandleMatchHasStarted()` / `HandleMatchHasEnded()` |
| **调用时机** | GameMode 的 MatchState 变化时调用。`HandleMatchHasStarted` 在比赛开始时（`MatchState == InProgress`），`HandleMatchHasEnded` 在比赛结束时（`MatchState == WaitingPostMatch` 或 `MatchState == LeavingMap`）。 |
| **适合写的逻辑** | — 记录比赛开始/结束时间<br>— 统计比赛数据<br>— 通知外部系统（如游戏服务器后端）<br>— 触发奖励发放等逻辑 |
| **Lyra 行为** | 仅调用 Super，作为预留钩子 |
| **注意事项** | Lyra 使用 `AGameModeBase` 而非 `AGameMode`，所以 MatchState 状态机本身不活跃。这些钩子存在的意义是当使用完全版 `AGameMode` 子类或通过其他机制驱动比赛状态时提供扩展点。 |

---

## 12. AWorldSettings

### AWorldSettings::CheckForErrors()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ALyraWorldSettings::CheckForErrors()` |
| **调用时机** | 编辑器 Map Check 功能触发时。用户点击 Build → Map Check 或 PIE 启动前的自动检查时调用。 |
| **调用次数** | 每次 Map Check 时 |
| **适合写的逻辑** | — 检查关卡中 Actor 的配置是否正确<br>— 验证 PlayerStart 类型是否符合预期<br>— 检查缺少的必需配置<br>— 输出警告/错误信息 |
| **Lyra 行为** | [Editor-Only] 验证关卡中所有 PlayerStart 都是 `ALyraPlayerStart` 类型 |
| **注意事项** | 只在编辑器中编译（`#if WITH_EDITOR`）。是关卡设计验证的第一道防线。 |

---

## 13. UGameViewportClient

### UGameViewportClient::Init(FWorldContext&, UGameInstance*, bool)

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `ULyraGameViewportClient::Init()` |
| **调用时机** | 视口客户端被创建时。由 `UGameInstance` 或 `UGameEngine` 在 World 创建之前调用。 |
| **适合写的逻辑** | — 配置视口显示参数<br>— 注册自定义视口渲染回调<br>— 设置分辨率/窗口模式策略<br>— ⚠️ 必须调用 Super::Init() |
| **Lyra 行为** | 调用 Super::Init()，项目级视口初始化 |
| **注意事项** | 通过 `DefaultEngine.ini` 的 `GameViewportClientClassName` 注册。每个 World 有一个视口客户端。 |

---

## 14. AGameModeBase

`ALyraGameMode` 当前承担 Experience 分配、玩家初始化门控、PawnData 选择、Pawn 生成和出生/重生代理。它仍然继承 `AGameModeBase`，不启用传统 `AGameMode` MatchState 状态机。

| 函数 | 调用时机 | Lyra 行为 |
|------|----------|-----------|
| `InitGame(MapName, Options, ErrorMessage)` | 地图加载后、GameState 初始化前，GameMode 初始化游戏规则时 | 调用 Super 后，下一帧触发 `HandleMatchAssignmentIfNotExpectingOne()` 分配 Experience |
| `InitGameState()` | GameState 创建并初始化后 | 获取 `ULyraExperienceManagerComponent`，通过 `CallOrRegister_OnExperienceLoaded()` 注册 Normal Priority（普通优先级）的 `OnExperienceLoaded()` 回调 |
| `UpdatePlayerStartSpot()` | `InitNewPlayer()` 期间，传统流程会根据 Portal 更新起点 | Lyra 直接返回 true，不在此阶段决定出生点；出生点选择延后到 RestartPlayer |
| `GenericPlayerInitialization()` | `PostLogin()` 和玩家通用初始化流程中 | 调用 Super 后广播 `OnGameModePlayerInitialized` |
| `HandleStartingNewPlayer_Implementation()` | 新玩家登录后，GameMode 准备启动玩家时 | 只有 Experience 已加载才调用 Super 继续 RestartPlayer；未加载时等待 `OnExperienceLoaded()` |
| `PlayerCanRestart_Implementation()` | 引擎判断玩家是否允许重生时 | 转发到 `ControllerCanRestart()` |
| `ShouldSpawnAtStartSpot()` | 引擎判断是否使用 Controller 的 StartSpot 时 | 返回 false，始终走 Lyra 自定义出生点选择 |
| `ChoosePlayerStart_Implementation()` | `RestartPlayer()` 选择出生点时 | 优先转发给 `ULyraPlayerSpawningManagerComponent`，否则回退 Super |
| `GetDefaultPawnClassForController_Implementation()` | 生成 Pawn 前查询 PawnClass 时 | 通过 `GetPawnDataForController()` 读取 PlayerState PawnData、Experience DefaultPawnData 或 AssetManager 默认 PawnData |
| `SpawnDefaultPawnAtTransform_Implementation()` | 引擎在选定出生变换后生成 Pawn 时 | 使用 deferred spawn；`ULyraPawnExtensionComponent` 初始化仍是 TODO；最后调用 `FinishSpawning()` |
| `FailedToRestartPlayer()` | `RestartPlayer()` 生成失败时 | 如果仍有 PawnClass 且允许重启，下一帧调用 `RequestPlayerRestartNextFrame()` 重试 |
| `FinishRestartPlayer()` | Pawn 已生成并完成控制器重启收尾时 | 先通知 `ULyraPlayerSpawningManagerComponent::FinishRestartPlayer()`，再调用 Super |

**Experience 分配辅助函数:**
- `HandleMatchAssignmentIfNotExpectingOne()` — 按 URL、PIE DeveloperSettings、命令行、WorldSettings、Dedicated Server、默认值的优先级选择 Experience。
- `OnMatchAssignmentGiven()` — 将最终 ExperienceId 交给 `ULyraExperienceManagerComponent::SetCurrentExperience()`。
- `TryDedicatedServerLogin()` / `HostDedicatedServerMatch()` — Dedicated Server 默认地图启动时，登录在线服务并根据 `-UserExperience=` / `-Playlist=` 选择 UserFacingExperience 来 Host Session。

---

## 15. AGameStateBase

`ALyraGameState` 现在是比赛级共享运行时状态的集中点。它仍继承 `AModularGameStateBase`，同时实现 `IAbilitySystemInterface`，并持有 Experience 状态机、比赛级 ASC、复制状态和客户端消息广播入口。

| 函数 | 调用时机 | Lyra 行为 |
|------|----------|-----------|
| `PreInitializeComponents()` | Actor 组件初始化前的 GameState 准备阶段 | 当前调用 Super，保留扩展点 |
| `PostInitializeComponents()` | 组件创建并初始化后、BeginPlay 前 | 检查 `AbilitySystemComponent`，调用 `InitAbilityActorInfo(this, this)` 初始化比赛级 ASC |
| `EndPlay(EndPlayReason)` | GameState 销毁或关卡切换时 | 当前调用 Super；Experience 反激活仍由 `ULyraExperienceManagerComponent::EndPlay()` 执行 |
| `Tick(DeltaSeconds)` | GameState 每帧 Tick | Authority 上把 `GAverageFPS` 写入 `ServerFPS`，再通过复制同步给客户端 |
| `AddPlayerState(PlayerState)` | PlayerState 加入 `PlayerArray` 时 | 当前调用 Super，保留扩展点 |
| `RemovePlayerState(PlayerState)` | PlayerState 从 `PlayerArray` 移除时 | 当前调用 Super，并记录注释说明 `AGameModeBase` 路径下调用时机有限 |
| `SeamlessTravelTransitionCheckpoint(bToTransitionMap)` | 无缝旅行进入 transition map 和目标 map 的 checkpoint | 从 `PlayerArray` 移除 Bot 或 inactive PlayerState，避免被带到下一张地图 |
| `GetAbilitySystemComponent()` | GAS 查询 Owner 的 ASC 时 | 返回 GameState 持有的 `ULyraAbilitySystemComponent` |
| `GetLifetimeReplicatedProps()` | Actor Channel 初始化复制属性时 | 注册 `ServerFPS`，并以 `COND_ReplayOnly` 复制 `RecorderPlayerState` |

**GameState 辅助能力:**
- `GetLyraAbilitySystemComponent()` — 返回类型化 ASC。
- `MulticastMessageToClients()` — Unreliable NetMulticast，客户端收到后用 `Message.Verb` 广播到 `UGameplayMessageSubsystem`。
- `MulticastReliableMessageToClients()` — Reliable NetMulticast，适合不能丢失的客户端通知。
- `SetRecorderPlayerState()` / `GetRecorderPlayerState()` — 管理 Replay 录制者 PlayerState；复制回调会广播 `OnRecorderPlayerStateChangedEvent`。

---

## 16. APlayerState

`ALyraPlayerState` 现在是玩家级运行时状态集中点。它继承 `AModularPlayerState`，同时实现 `IAbilitySystemInterface` 与 `ILyraTeamAgentInterface`，并复制 PawnData、连接类型、队伍、小队、StatTag Stack 和观战视角旋转。

| 函数 | 调用时机 | Lyra 行为 |
|------|----------|-----------|
| `PreInitializeComponents()` | Actor 组件初始化前 | 当前调用 Super，保留扩展点 |
| `PostInitializeComponents()` | 组件初始化后、BeginPlay 前 | 初始化玩家级 ASC ActorInfo；服务器 GameWorld 中注册 Experience Loaded 回调 |
| `GetLifetimeReplicatedProps()` | Actor Channel 初始化复制属性时 | 使用 Push Model 复制 `PawnData`、连接类型、Team/Squad；复制 `StatTags`；以 `COND_SkipOwner` 复制 `ReplicatedViewRotation` |
| `Reset()` | PlayerState 被重置时 | 当前调用 Super，保留扩展点 |
| `ClientInitialize(AController*)` | PlayerState 在客户端关联 Controller 时 | 调用 Super；保留 PawnExtension 默认初始化 TODO |
| `CopyProperties(APlayerState*)` | 无缝旅行或 inactive PlayerState 复制状态时 | 调用 Super；统计数据复制仍是 TODO |
| `OnDeactivated()` | 玩家断线、离开或 PlayerState 停用时 | 将连接类型设为 `InactivePlayer`，当前策略下销毁该 PlayerState |
| `OnReactivated()` | inactive PlayerState 被重新激活时 | 如果当前是 `InactivePlayer`，恢复为 `Player` |
| `SetGenericTeamId()` | 服务器改变玩家队伍时 | 标记 `MyTeamID` dirty，写入新队伍，并在队伍实际变化时广播 TeamChanged 委托 |
| `OnRep_MyTeamID(OldTeamID)` | 客户端收到队伍 ID 复制时 | 条件广播 TeamChanged 委托 |
| `OnRep_MySquadID()` | 客户端收到小队 ID 复制时 | 当前只有 TODO，等待 Squad subsystem |
| `OnRep_PawnData()` | 客户端收到 PawnData 复制时 | 当前为空实现，后续可接入 Pawn/Ability 初始化链 |

**PlayerState 辅助能力:**
- `SetPawnData()` — Authority-only，一次性写入 PawnData，预留 AbilitySet 授予 TODO，并发送 `LyraAbilitiesReady` 扩展事件。
- `AddStatTagStack()` / `RemoveStatTagStack()` — Authority-only 修改玩家统计标签栈。
- `ClientBroadcastMessage()` — Unreliable Client RPC，把服务器发给单个玩家的 `FLyraVerbMessage` 转进客户端 `UGameplayMessageSubsystem`。
- `GetReplicatedViewRotation()` / `SetReplicatedViewRotation()` — 为观战同步玩家视角。

---

## 17. UBlueprintAsyncActionBase

`UAsyncAction_ExperienceReady` 是蓝图异步节点，用来等待当前 World 的 Experience 加载完成。它不直接加载 Experience，只监听 `ULyraExperienceManagerComponent` 的状态。

### UBlueprintAsyncActionBase::Activate()

| 项目 | 说明 |
|------|------|
| **Lyra 重写** | `UAsyncAction_ExperienceReady::Activate()` |
| **调用时机** | 蓝图异步节点被创建并激活时。由蓝图虚拟机在调用 `WaitForExperienceReady()` 返回节点后触发。 |
| **适合写的逻辑** | — 绑定异步回调<br>— 注册生命周期保持逻辑<br>— 在条件已经满足时安全地延迟或立即广播<br>— 失败路径中调用 `SetReadyToDestroy()` |
| **Lyra 行为** | 若 World 已有 GameState，直接监听 ExperienceManager；若 GameState 尚未创建，先绑定 `World::GameStateSetEvent`；若 Experience 已 Loaded，则下一帧广播 `OnReady`；否则注册 `CallOrRegister_OnExperienceLoaded()`。 |
| **注意事项** | `WaitForExperienceReady()` 会调用 `RegisterWithGameInstance(World)`，因此节点会存活到 `Step4_BroadcastReady()` 调用 `SetReadyToDestroy()`。已 Loaded 时仍延迟一帧，是为了避免调用方依赖即时完成时序。 |

---

## 按开发场景快速索引

| 我想做... | 用这个函数 | 在哪个类中 |
|-----------|-----------|-----------|
| 在项目启动时做一次全局初始化 | `UGameEngine::Init()` | `ULyraGameEngine` |
| 在游戏实例创建时初始化全局状态 | `UGameInstance::Init()` | `ULyraGameInstance` |
| 在游戏实例销毁时保存状态 | `UGameInstance::Shutdown()` | `ULyraGameInstance` |
| 在用户登录后加载设置 | `HandlerUserInitialized()` | `ULyraGameInstance` |
| 预加载游戏必需的资产 | `StartInitialLoading()` | `ULyraAssetManager` |
| 修改 PIE 启动行为 | `PreCreatePIEInstances()` | `ULyraEditorEngine` |
| 在地图启动时决定本局 Experience | `InitGame()` / `HandleMatchAssignmentIfNotExpectingOne()` | `ALyraGameMode` |
| 等待 Experience 加载完成后生成玩家 Pawn | `HandleStartingNewPlayer_Implementation()` / `OnExperienceLoaded()` | `ALyraGameMode` |
| 按 Experience 的 PawnData 决定 PawnClass | `GetDefaultPawnClassForController_Implementation()` | `ALyraGameMode` |
| 在蓝图中等待 Experience Ready | `WaitForExperienceReady()` / `Activate()` | `UAsyncAction_ExperienceReady` |
| 初始化比赛级 AbilitySystemComponent | `PostInitializeComponents()` | `ALyraGameState` |
| 初始化玩家级 AbilitySystemComponent 和 PawnData | `PostInitializeComponents()` / `OnExperienceLoaded()` / `SetPawnData()` | `ALyraPlayerState` |
| 同步服务器 FPS 或 Replay 录制者状态 | `Tick()` / `GetLifetimeReplicatedProps()` / `OnRep_RecorderPlayerState()` | `ALyraGameState` |
| 同步玩家 Team/Squad、StatTag 和观战视角 | `GetLifetimeReplicatedProps()` / `OnRep_MyTeamID()` / `OnRep_MySquadID()` | `ALyraPlayerState` |
| 从服务器广播客户端 gameplay message | `MulticastMessageToClients()` / `MulticastReliableMessageToClients()` | `ALyraGameState` |
| 向单个玩家客户端发送 gameplay message | `ClientBroadcastMessage()` | `ALyraPlayerState` |
| 在 Actor 生成后初始化游戏逻辑 | `BeginPlay()` | `ALyraHUD` / 任何 Actor |
| 在 Actor 销毁时清理 | `EndPlay()` | `ALyraHUD` / 任何 Actor |
| 在组件开始时初始化组件状态 | `BeginPlay()` / `PreInitializeComponents()` | Component 子类 |
| 缓存出生点并绑定关卡/Actor 生成事件 | `InitializeComponent()` | `ULyraPlayerSpawningManagerComponent` |
| 在 Experience 卸载时清理 | `EndPlay()` | `ULyraExperienceManagerComponent` |
| 同步 CommonUI 根布局和 HUD 显示开关 | `Initialize()` / `Tick()` / `Deinitialize()` | `ULyraUIManagerSubsystem` |
| 在编辑器中修改属性后联动 | `PostEditChangeProperty()` | `ULyraDeveloperSettings` |
| 在配置重载后重新应用 | `PostReloadConfig()` | `ULyraDeveloperSettings` |
| 验证数据资产配置 | `IsDataValid()` | `ULyraExperienceDefinition` |
| 注册编辑器委托（PIE 等） | `StartupModule()` | `FLyraEditorModule` |
| 控制加载画面显示 | `ShouldShowLoadingScreen()` | `ULyraExperienceManagerComponent` |
| 检查关卡配置错误 | `CheckForErrors()` | `ALyraWorldSettings` |
| 禁用引擎默认自动登录 | `ProcessAutoLogin()` | `ALyraGameSession` |

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameEngine、ULyraGameInstance、ULyraAssetManager 详解
- [04-Game-Framework.md](04-Game-Framework.md) — ALyraGameSession、ALyraWorldSettings 详解
- [07-Experience-Framework.md](07-Experience-Framework.md) — ULyraExperienceManagerComponent 状态机详解
- [08-UI-Framework.md](08-UI-Framework.md) — ALyraHUD、ULyraGameViewportClient 详解
- [11-Development-Tools.md](11-Development-Tools.md) — ULyraDeveloperSettings 属性详解
- [12-Editor-Module.md](12-Editor-Module.md) — ULyraEditorEngine、FLyraEditorModule 详解
