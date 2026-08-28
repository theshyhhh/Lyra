# 03 - System 框架

> 核心系统级类：游戏引擎、游戏实例、资产管理器、全局游戏数据和跨局功能子系统。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 GameInstance（游戏实例）、LocalPlayer（本地玩家）和 Settings
> （设置）改动。当前项目与 Lyra 参考项目均声明 UE 5.7，底层机制按 Unreal
> Engine 5.7.4 源码核对；尚未执行登录、设置、资产加载或 Replay（回放）
> 运行验证。

---

## 框架概述

**问题：** 引擎启动、GameInstance（游戏实例）、资产加载、全局数据和 Replay
能力需要跨 World（世界）长期存在，不能由某个关卡 Actor 临时承担。

**当前项目的解决方案：** 通过 `DefaultEngine.ini` 替换 GameEngine（游戏
引擎）和 AssetManager（资产管理器），由 GameInstance 注册 Modular
Gameplay（模块化游戏玩法）初始化状态，由 GameInstance Subsystem（游戏
实例子系统）暴露平台 Replay 能力，并由 GameInstance 把 CommonUser（通用
用户）的登录完成事件桥接到对应 LocalPlayer 的共享设置加载入口。

**设计意图:**

- 提供统一的资产加载管线（AssetManager 启动作业系统）
- 管理游戏生命周期（GameInstance、GameEngine）
- 承载全局游戏配置数据（GameData）

**当前完成度：** AssetManager 和 GameInstance 已有主要结构；共享设置登录
入口已经接通，但异步加载端点仍直接返回失败。GameplayCue Manager（游戏
玩法提示管理器）和完整 Replay Subsystem 仍缺失；GameEngine 仍是结构占位。

---

## 当前复刻状态

| 模块 | 当前状态 | 当前实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| GameEngine（游戏引擎） | **结构占位** | `Init()` 只调用 `Super` | Lyra 可承载项目级初始化 | 当前没有额外引擎行为 |
| GameInstance（游戏实例） | **部分复刻** | InitState、测试加密、旅行委托、类型化 Controller 访问和登录后设置入口已接入 | Lyra 的设置异步端点会真正加载、应用并回调 | 当前调用链在 `AsyncLoadOrCreateSettings()` 中断 |
| Replay Subsystem（回放子系统） | **部分复刻** | 只判断平台 Trait（特征标签）是否支持 Replay | Lyra 还负责录制、播放、查询、删除和跳转 | 客户端自动录制未复刻 |
| AssetManager（资产管理器） | **部分复刻** | 启动作业、GameData/PawnData 类型化加载存在 | Lyra 还完整初始化 GameplayCue 等系统 | GameplayCue 启动链为空 |
| GameData（游戏数据） | **部分复刻** | 三个全局 GameplayEffect（游戏玩法效果）软类引用存在 | Lyra 由伤害、治疗和标签系统消费 | 当前主要是配置容器 |

---

## 类列表

| 类 | 父类或接口 | 生命周期 | 网络位置 | 当前状态 | 职责 |
|---|---|---|---|---|---|
| `ULyraGameEngine` | `UGameEngine` | Runtime，进程级 | 每个进程本地，不复制 | **结构占位** | 游戏引擎初始化入口 |
| `ULyraGameInstance` | `UCommonGameInstance` | Runtime，跨 World | Server / Client 各自本地，不复制 | **部分复刻** | InitState、测试加密、Session（会话）旅行和用户设置桥接 |
| `ULyraReplaySubsystem` | `UGameInstanceSubsystem` | Runtime，随 GameInstance | 本地子系统，不复制 | **部分复刻** | 判断平台是否支持 Replay |
| `ULyraAssetManager` | `UAssetManager` | Runtime，进程级 | 各进程独立加载资产 | **部分复刻** | 启动作业、类型化加载和全局数据访问 |
| `ULyraGameData` | `UPrimaryDataAsset` | Runtime，资产对象 | 配置资产不直接复制 | **部分复刻** | 保存全局 Damage / Heal / Dynamic Tag GE 引用 |
| `FLyraAssetManagerStartupJob` | 非 UObject | Runtime，短期值对象 | 不复制 | **已复刻** | 包装启动任务与进度 |
| `FLyraBundles` | 非 UObject | Runtime，静态常量 | 不复制 | **已复刻** | 提供 `Equipped` Asset Bundle（资产包）名称 |

---

## 逐类详解

### ULyraGameEngine [Runtime]

**当前状态：** **结构占位**

**继承链:** `UObject → UEngine → UGameEngine → ULyraGameEngine`

**UCLASS:** `UCLASS()`

**职责:** 自定义游戏引擎类。当前实现为透传（仅调用 `Super::Init(IEngineLoop*)`），作为未来项目级引擎初始化逻辑的钩子点。

**重写的生命周期函数:**

##### `Init(IEngineLoop* InEngineLoop)`

> ⏱️ **引擎调用时机:** 引擎启动最早阶段。所有模块加载完毕、UObject 系统初始化之后，**但在任何 World 创建之前**。整个进程生命周期仅调用一次。
>
> **此时可用:** UObject 系统 ✓、模块 ✓、Slate ✗、World ✗
>
> **适合写的逻辑:** 项目级引擎初始化（注册全局委托、修改引擎静态配置）。⚠️ 不能访问 World 或 Actor。耗时操作会增加启动时间，优先考虑懒加载。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 1](17-Engine-Lifecycle-Reference.md#1-引擎与编辑器生命周期)

当前仅调用 `Super::Init(InEngineLoop)`，作为预留扩展点。

**注册:** `DefaultEngine.ini` 中 `GameEngine=/Script/LyraGame.LyraGameEngine`

---

### ULyraGameInstance [Runtime]

**当前状态：** **部分复刻**

**继承链:** `UObject → UGameInstance → UCommonGameInstance → ULyraGameInstance`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 游戏实例。这是生命周期最长的游戏端对象，负责：
1. 在 `Init()` 中注册 InitState Tag 依赖（关键！所有 Modular Actor 的初始化都依赖于此）
2. 处理用户登录初始化（CommonUser 框架集成）
3. 网络加密支持（用于测试，非生产安全）
4. 会话旅行预处理
5. 按 LocalPlayer 索引启动每用户 Shared Settings（共享设置）加载

**重写的生命周期函数:**

##### `Init()`

> ⏱️ **引擎调用时机:** GameInstance 被创建时，在第一个 World 创建**之前**、任何 Actor 的 BeginPlay 之前。由 `UGameEngine::Init()` 触发。
>
> **此时可用:** Subsystem ✓、World ✗、Actor ✗
>
> **适合写的逻辑:** 注册 Subsystem、初始化全局状态、绑定网络/旅行委托、初始化项目级设置。⚠️ 必须调用 `Super::Init()`！
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

当前行为：`Super::Init()` 先完成 CommonGame 的 CommonUser / CommonSession
绑定；随后当前类向 `UGameFrameworkComponentManager` 注册 InitState 状态
转移链，生成调试加密密钥，并绑定 Session 旅行前委托。

##### `Shutdown()`

> ⏱️ **引擎调用时机:** GameInstance 被销毁时。进程关闭或引擎正常退出时调用。此时 World 可能已部分销毁，但 UObject 系统仍可用。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

当前先从 `UCommonSessionSubsystem::OnPreClientTravelEvent` 移除本实例绑定，
再调用 `Super::Shutdown()`；清理与 `Init()` 中的注册成对。

##### `HandlerUserInitialized()`

> ⏱️ **引擎调用时机:** CommonUser 框架回调，用户登录成功后异步触发。不在固定引擎帧中。
>
> **适合写的逻辑:** 为用户加载本地设置、应用用户偏好（语言/控制方案）。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

当前为**部分复刻**：成功且 `UserInfo` 有效时，根据
`UserInfo->LocalPlayerIndex` 查找 `ULyraLocalPlayer`，存在时调用
`LoadSharedSettingsFromDisk()`。Dedicated Server（专用服务器）用户通常没有
LocalPlayer，因此安全跳过。调用已经真实发生，但下游
`ULyraSettingsShared::AsyncLoadOrCreateSettings()` 仍直接返回 `false`，会触发
`ensure(false)`，不会发起引擎异步请求，也不会进入完成回调。

##### `ReceivedNetworkEncryptionToken()` / `ReceivedNetworkEncryptionAck()`

> ⏱️ **引擎调用时机:** 客户端连接服务器时，网络加密握手阶段。由 EOS/Steam/Oodle 等加密层触发。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

DTLS 加密的测试支持（使用硬编码密钥，仅用于测试）。

**其他方法:**

- `CanJoinRequestedSession()`: 始终返回 true（占位）
- `GetPrimaryPlayerController()`: 调用
  `Super::GetPrimaryPlayerController(false)` 并转换为
  `ALyraPlayerController`；`false` 只表示不要求有效 Unique Net ID（唯一网络
  ID），父类仍要求有效 World、PlayerState 和 Primary Player（主玩家）
- `OnPreClientTravelToSession()`: 会话旅行前在 URL 中附加加密 Token

**CVars:**

- `Lyra.TestEncryption` — 是否启用测试加密
- `Lyra.UseDTLSEncryption` — 是否使用 DTLS

---

### ULyraReplaySubsystem [Runtime]

**当前状态：** **部分复刻**

**继承链:** `UObject → USubsystem → UGameInstanceSubsystem → ULyraReplaySubsystem`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 回放系统的运行时能力判断入口。当前只负责判断平台是否支持回放，不直接实现录制、播放或回放列表管理。

**关键方法:**

- `DoesPlatformSupportReplays()` — 查询 CommonUI 的平台 Trait 集合，判断是否包含 `Platform.Trait.ReplaySupport`
- `GetPlatformSupportTraitTag()` — 返回回放支持对应的 GameplayTag

**与 Session 的关系:**
`ULyraUserFacingExperienceDefinition::CreateHostingRequest()` 会在 `bRecordReplay == true` 且平台支持回放时，向 Session URL 参数中追加 `DemoRec`。这意味着 Playlist 可以声明“本局需要录制回放”，但最终是否生效仍受平台 Trait 控制。

**与当前 PlayerController（玩家控制器）的关系：**
提交 `19e6961` 加入 Client Replay（客户端回放）的资格检查、
`RecorderPlayerState` 标记和 Replay Controller 跟随修复逻辑；但
`ULyraReplaySubsystem` 仍没有 Lyra 原项目的 `RecordClientReplay()`，
Controller 中的调用也被注释。因此播放跟随为**部分复刻**，客户端录制为
**未复刻**。完整对照见
[18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md)。

---

### ULyraAssetManager [Runtime]

**当前状态：** **部分复刻**

**继承链:** `UObject → UAssetManager → ULyraAssetManager`

**UCLASS:** `UCLASS(MinimalAPI, Config=Game)`

**职责:** 资产管理器。Lyra 资产加载的核心枢纽。

**关键特性:**

**重写的生命周期函数:**

##### `StartInitialLoading()`

> ⏱️ **引擎调用时机:** 引擎启动过程中，所有模块加载之后、第一个 World 创建之前。由 `UEngine::Init()` → `UAssetManager::Initialize` 链触发。仅调用一次。
>
> **此时可用:** 文件系统 ✓、AssetRegistry ✓（可能仍在扫描）、World ✗
>
> **适合写的逻辑:** 预加载关键游戏数据（GameData、PawnData 等）、初始化 GameplayCue 管理器、启动内容加载进度跟踪。使用 `FLyraAssetManagerStartupJob` 包装以支持进度报告。⚠️ 同步加载会增加启动时间，必须调用 `Super::StartInitialLoading()`。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 7](17-Engine-Lifecycle-Reference.md#7-assetmanager-生命周期)

当前行为：运行启动作业列表（GameplayCue 初始化 [TODO 空实现] + 加载 `ULyraGameData`），记录启动耗时。

##### `PreBeginPIE(bool bStartSimulate)`

> ⏱️ **引擎调用时机:** PIE 启动前，World 创建之前。`bStartSimulate` 指示是否模拟模式。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 7](17-Engine-Lifecycle-Reference.md#7-assetmanager-生命周期)

[Editor-Only] PIE 预加载钩子。

**其他特性:**

1. **启动作业系统:** 见上述 `StartInitialLoading()`。

2. **类型化加载辅助函数:**
   - `GetAsset<T>(TSoftObjectPtr<T>, bool bKeepInMemory)` — 同步加载软对象路径
   - `GetSubclass<T>(TSoftClassPtr<T>, bool bKeepInMemory)` — 同步加载软类路径
   - `LoadGameDataOfClass<T>()` — 加载并缓存 UPrimaryDataAsset 子类，按类型索引到 `GameDataMap`

3. **全局数据访问器:**
   - `GetGameData()` — 返回 `ULyraGameData` 单例（同步加载保底）
   - `GetDefaultPawnData()` — 返回默认 `ULyraPawnData`（PlayerState 和 Experience 都未指定 PawnData 时的保底）

4. **调试工具:**
   - `DumpLoadedAssets()` — 控制台命令，列出所有已加载资产及其内存状态
   - `ShouldLogAssetLoads()` — 由 CVar `Lyra.DumpLoadedAssets` 控制

5. **编辑器钩子:**
   - `PreBeginPIE()` — PIE 启动前预加载内容 [Editor-Only]

**关键方法（静态）:**

- `Get()` — 返回 `ULyraAssetManager*` 单例（`static_cast<ULyraAssetManager*>(&UAssetManager::Get())`）

---

### FLyraAssetManagerStartupJob

**当前状态：** **已复刻**

**类型:** 非 UObject C++ 结构体

**职责:** 包装一个启动作业的 TFunction 和相关的进度控制。

**字段:**

- `JobFunc` (TFunction) — 作业函数，返回一个 `TSharedPtr<FStreamableHandle>`
- `JobName` (FString) — 作业名称
- `JobWeight` (float) — 进度权重
- `UpdateProgress` (TFunction) — 进度回调委托

**关键方法:**

- `DoJob()` — 执行 JobFunc
- `UpdateSubstepProgress()` — 上报子步骤进度
- `UpdateSubstepProgressFromStreamable()` — 从 StreamableHandle 采样进度（上限 60fps）

---

### FLyraBundles

**当前状态：** **已复刻**

**类型:** 非 UObject C++ 结构体

**成员:**

- `static const FName Equipped` — 命名的 Asset Bundle 常量，用于 Experience 资产加载时按 Bundle 名称加载资产。

---

### ULyraGameData [Runtime]

**当前状态：** **部分复刻**

**继承链:** `UObject → UDataAsset → UPrimaryDataAsset → ULyraGameData`

**UCLASS:** `UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "Lyra Game Data"))`

**职责:** 全局游戏数据资产。持有跨游戏使用的基础 GameplayEffect 软引用。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `DamageGameplayEffect_SetByCaller` | `TSoftClassPtr<UGameplayEffect>` | 伤害 GE（伤害数值由 SetByCaller 传递） |
| `HealGameplayEffect_SetByCaller` | `TSoftClassPtr<UGameplayEffect>` | 治疗 GE |
| `DynamicTagGameplayEffect` | `TSoftClassPtr<UGameplayEffect>` | 动态添加/移除 GameplayTag 的 GE |

**访问方式:** 通过 `ULyraAssetManager::GetGameData()` 获取。

---

## 核心数据流

```
ULyraGameEngine::Init()          ← 目前透传，计划扩展
        |
ULyraAssetManager::StartInitialLoading()
  ├── InitializeGameplayCueManager()   [TODO: 空实现]
  └── LoadGameDataOfClass<ULyraGameData>()
        └── ULyraGameData 被缓存到 GameDataMap

ULyraGameInstance::Init()
  ├── 注册 InitState 状态转移（Spawned → DataAvailable → DataInitialized → GameplayReady）
  ├── 生成调试加密密钥
  └── 绑定会话旅行委托

UCommonUserSubsystem 用户初始化成功
  └── ULyraGameInstance::HandlerUserInitialized()
        └── GetLocalPlayerByIndex()
              └── ULyraLocalPlayer::LoadSharedSettingsFromDisk()
                    └── ULyraSettingsShared::AsyncLoadOrCreateSettings()
                          └── [当前直接返回 false，未发起异步加载]

ULyraReplaySubsystem
  └── 读取 CommonUI PlatformTraits → 判断 Platform.Trait.ReplaySupport
```

`StartInitialLoading()` 在首个 World 创建前运行。GameData 加载当前是同步
`TryLoad` / 类型化加载路径，会阻塞 Game Thread；Experience 后续的 Bundle
加载则是异步路径。Server 与 Client 各自拥有 AssetManager / GameInstance
运行环境，不依靠网络复制完整配置资产。

失败边界：

- 无法取得必需 GameData 时，现有访问器主要使用 `check` / `ensure` 和日志，
  不具备完整的可恢复错误流程。
- GameplayCue Manager 初始化函数为空，不会产生 Cue 预加载结果。
- Replay 平台 Trait 缺失时只返回“不支持”，不会自动回退到另一种录制方案。
- 登录成功只保证进入项目包装函数；当前共享设置包装函数返回 `false`，因此
  `OnSharedSettingsLoaded()` 不会由这条链触发。

---

## 初始化与清理

| 对象 | 初始化入口 | 所有权 / 生命周期 | 当前清理 |
|---|---|---|---|
| `ULyraGameEngine` | 引擎创建后调用 `Init()` | 进程级引擎对象 | 由引擎退出流程管理 |
| `ULyraGameInstance` | `UGameEngine` 创建后调用 `Init()` | 跨 World，拥有 GameInstance Subsystem | `Shutdown()` 移除 Session 旅行委托后调用 `Super` |
| `ULyraReplaySubsystem` | GameInstance Subsystem 自动创建 | Outer 为 GameInstance | 随 GameInstance 反初始化 |
| `ULyraAssetManager` | 引擎按配置创建并调用 `StartInitialLoading()` | 进程级单例 | 已缓存资产按 AssetManager / 进程生命周期持有 |
| `ULyraGameData` | AssetManager 根据 Primary Asset 配置加载 | Outer 和 GC 由资产系统管理 | 当前没有运行时卸载策略 |

---

## 网络与权限

System 框架的大多数对象不直接复制：Server、Client 和 Editor 进程各自完成
本地初始化。真正跨网络同步的是 GameState / PlayerState 中的运行时状态，
不是 `ULyraGameData` 或完整 Experience 配置。

- Replay 平台 Trait 是本地能力判断；它不证明服务器或其他客户端会录制。
- Shared Settings 是本地 SaveGame（存档）对象，不通过网络复制；专用服务器
  没有 LocalPlayer，监听服务器的主机用户则会执行本地设置路径。
- GameData 是配置来源，GAS 的 Ability、Active GameplayEffect（激活中的
  游戏玩法效果）、Attribute 和 GameplayCue 结果由 ASC 的网络规则处理。
- 测试加密握手会影响连接流程，但当前使用硬编码测试密钥，不能视为生产安全。

---

## 资产、配置与模块依赖

| 来源 | 当前配置 | 作用 |
|---|---|---|
| `Config/DefaultEngine.ini` | 注册 `ULyraGameEngine`、`ULyraAssetManager` | 替换引擎默认类型 |
| `Config/DefaultEngine.ini` | `LocalPlayerClassName=/Script/LyraGame.LyraLocalPlayer` | 让登录回调能取得自定义 LocalPlayer |
| `Config/DefaultGame.ini` | 配置 GameData、DefaultPawnData 和 Primary Asset 扫描 | 决定 AssetManager 能否发现资产 |
| `ULyraGameData` | 三个 `TSoftClassPtr<UGameplayEffect>` | 软引用在使用前需要加载 |
| `FLyraBundles::Equipped` | Experience Bundle 名称 | Experience 加载时选择关联资产 |
| `CommonUI` | 提供 Platform Trait 集合 | Replay 支持判断依赖 `Platform.Trait.ReplaySupport` |
| `GameplayAbilities` | 提供 GameplayEffect 与 ASC | GameData 只保存配置，不执行效果 |

> 🧪 **待验证：**
> 当前文档未逐个打开 GameData、DefaultPawnData 和 Experience 资产确认其
> 最终引用；C++ 只能证明字段和加载入口存在。

---

## 与 Lyra 的差异

### 差异：Replay Subsystem 只有能力判断

**当前项目：** 只有平台支持 Trait 查询。

**Lyra：** 还提供客户端录制、回放列表、播放、删除、清理和 Seek（跳转）。

**影响：** PlayerController 即使满足录制条件，也没有最终录制执行者。

**状态：** **部分复刻**

### 差异：GameplayCue 启动任务为空

**当前项目：** `InitializeGameplayCueManager()` 作为启动 Job（任务）存在，
但函数体为空。

**Lyra：** 在启动阶段初始化项目 GameplayCue Manager 和相关资源。

**影响：** 当前不能从 AssetManager 启动链证明 Cue 已预加载或按项目策略管理。

**状态：** **结构占位**

### 差异：用户共享设置入口已接入，异步端点仍占位

**当前项目：** CommonUser 用户初始化成功后已经调用 LocalPlayer；同步创建 /
加载 `SharedGameSettings` 的方法也存在，但登录路径使用的
`AsyncLoadOrCreateSettings()` 直接返回 `false`。

**Lyra：** 包装 UE 的
`AsyncLoadOrCreateSaveGameForLocalPlayer()`，完成时应用设置并把真实对象回传
LocalPlayer。

**影响：** 登录入口每次尝试都会触发 `ensure(false)`；磁盘对象不会异步加载，
LocalPlayer 缓存不会由回调替换，力反馈等每用户设置也没有实际字段可消费。

**状态：** **部分复刻**

---

## 已识别的 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 当前源码 TODO | 实现 `InitializeGameplayCueManager()` | 当前函数体为空 | GameplayCue Manager 类型与资源策略 |
| 中 | Lyra 对比 TODO | 扩展 Replay Subsystem 的录制和播放能力 | 当前只有 Trait 判断 | 本地设置、Replay UI / 清理策略 |
| 高 | 当前源码 TODO | 实现 Shared Settings 异步加载与成功/失败回调 | 登录入口已接通，但 `AsyncLoadOrCreateSettings()` 直接返回 `false` | `ULocalPlayerSaveGame` 异步 API 与设置应用策略 |
| 中 | 验证 TODO | 验证 GameData / DefaultPawnData 的资产扫描和加载 | 依赖 Config 与二进制资产 | Editor Asset Audit（资产审计）或运行日志 |
| 低 | 文档 TODO | 记录 GameData 中三个 GE 的实际资产与消费者 | 当前只确认字段 | 伤害、治疗、动态标签系统 |

---

## 快速回顾

- **一句话职责：** System 框架管理进程级启动、跨 World 状态和全局配置资产。
- **核心入口：** `UGameEngine::Init()`、`UGameInstance::Init()`、
  `UAssetManager::StartInitialLoading()`。
- **核心状态：** InitState 转移、GameData 缓存、Replay 平台 Trait，以及
  LocalPlayer 中的 Shared Settings 缓存。
- **网络位置：** Server / Client 各自初始化，配置资产不直接复制。
- **当前完成度：** **部分复刻**。
- **最重要的未完成项：** GameplayCue 初始化、共享设置异步加载 / 应用和完整
  Replay Subsystem。

## 复习要点

1. 为什么 GameInstance 和 AssetManager 适合承载跨 World 状态？
2. 哪些启动步骤发生在第一个 World 创建之前？
3. GameData 是静态配置、运行时实例还是网络状态？
4. Replay 平台 Trait 能证明什么，不能证明什么？
5. 同步加载与异步 Bundle 加载分别出现在哪条链？
6. 当前 System 框架与 Lyra 最大的三个差异是什么？
7. 用户登录回调为什么只负责选择 LocalPlayer，而不直接持有设置对象？

---

## 关联框架

- [04-Game-Framework.md](04-Game-Framework.md) — ALyraWorldSettings 通过 GetDefaultPawnData 加载 PawnData
- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 加载通过 AssetManager 进行 Bundle 加载
- [09-GameplayTags-System.md](09-GameplayTags-System.md) — `Platform.Trait.ReplaySupport` 的分散定义
- [02-Engine-Configuration.md](02-Engine-Configuration.md) — DefaultEngine.ini 中的类注册映射
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerState ASC 和 PlayerController Replay 入口消费 System 层能力
- [08-UI-Framework.md](08-UI-Framework.md) — LocalPlayer、本地/共享设置及登录后加载链的完整边界
- [18-Current-Source-Comparison-and-Controller-Callchain.md](18-Current-Source-Comparison-and-Controller-Callchain.md) — 区分 Replay 播放跟随与客户端录制缺口
