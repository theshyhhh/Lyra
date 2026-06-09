# 03 - System 框架

> 核心系统级类：游戏引擎、游戏实例、资产管理器、全局游戏数据和跨局功能子系统。

---

## 框架概述

System 框架包含 Lyra 对 UE 引擎底层系统的自定义实现。这些类在 `DefaultEngine.ini` 中被注册为引擎的默认实现，是 Lyra 架构的"骨架"层。

**设计意图:**
- 提供统一的资产加载管线（AssetManager 启动作业系统）
- 管理游戏生命周期（GameInstance、GameEngine）
- 承载全局游戏配置数据（GameData）

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ULyraGameEngine` | `UGameEngine` | [Runtime] | 游戏引擎初始化入口（目前透传） |
| `ULyraGameInstance` | `UCommonGameInstance` | [Runtime] | 游戏实例：InitState 注册、加密、会话旅行 |
| `ULyraReplaySubsystem` | `UGameInstanceSubsystem` | [Runtime] | 回放能力开关：根据平台 Trait 判断是否允许录制 |
| `ULyraAssetManager` | `UAssetManager` | [Runtime] | 资产管理器：启动作业、类型化加载、GameData 访问 |
| `ULyraGameData` | `UPrimaryDataAsset` | [Runtime] | 全局游戏数据资产：伤害/治疗/动态标签 GE 引用 |
| `FLyraAssetManagerStartupJob` | (非 UObject) | [Runtime] | 启动作业包装器：TFunction + 进度委托 |
| `FLyraBundles` | (非 UObject) | [Runtime] | 静态 Bundle 名称常量 |

---

## 逐类详解

### ULyraGameEngine [Runtime]

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

**继承链:** `UObject → UGameInstance → UCommonGameInstance → ULyraGameInstance`

**UCLASS:** `UCLASS(MinimalAPI, Config = Game)`

**职责:** 游戏实例。这是生命周期最长的游戏端对象，负责：
1. 在 `Init()` 中注册 InitState Tag 依赖（关键！所有 Modular Actor 的初始化都依赖于此）
2. 处理用户登录初始化（CommonUser 框架集成）
3. 网络加密支持（用于测试，非生产安全）
4. 会话旅行预处理

**重写的生命周期函数:**

##### `Init()`
> ⏱️ **引擎调用时机:** GameInstance 被创建时，在第一个 World 创建**之前**、任何 Actor 的 BeginPlay 之前。由 `UGameEngine::Init()` 触发。
>
> **此时可用:** Subsystem ✓、World ✗、Actor ✗
>
> **适合写的逻辑:** 注册 Subsystem、初始化全局状态、绑定网络/旅行委托、初始化项目级设置。⚠️ 必须调用 `Super::Init()`！
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

当前行为：向 `UGameFrameworkComponentManager` 注册 InitState 状态转移链（这是所有 Modular Actor 初始化的基础设施）并生成调试加密密钥。

##### `Shutdown()`
> ⏱️ **引擎调用时机:** GameInstance 被销毁时。进程关闭或引擎正常退出时调用。此时 World 可能已部分销毁，但 UObject 系统仍可用。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

当前仅调用 `Super::Shutdown()`，无额外清理。

##### `HandlerUserInitialized()`
> ⏱️ **引擎调用时机:** CommonUser 框架回调，用户登录成功后异步触发。不在固定引擎帧中。
>
> **适合写的逻辑:** 为用户加载本地设置、应用用户偏好（语言/控制方案）。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

当前 [Stub] — 调用 `LoadSharedSettingsFromDisk()` 的代码被注释掉。

##### `ReceivedNetworkEncryptionToken()` / `ReceivedNetworkEncryptionAck()`
> ⏱️ **引擎调用时机:** 客户端连接服务器时，网络加密握手阶段。由 EOS/Steam/Oodle 等加密层触发。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 2](17-Engine-Lifecycle-Reference.md#2-gameinstance-生命周期)

DTLS 加密的测试支持（使用硬编码密钥，仅用于测试）。

**其他方法:**
- `CanJoinRequestedSession()`: 始终返回 true（占位）
- `OnPreClientTravelToSession()`: 会话旅行前在 URL 中附加加密 Token

**CVars:**
- `Lyra.TestEncryption` — 是否启用测试加密
- `Lyra.UseDTLSEncryption` — 是否使用 DTLS

---

### ULyraReplaySubsystem [Runtime]

**继承链:** `UObject → USubsystem → UGameInstanceSubsystem → ULyraReplaySubsystem`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 回放系统的运行时能力判断入口。当前只负责判断平台是否支持回放，不直接实现录制、播放或回放列表管理。

**关键方法:**
- `DoesPlatformSupportReplays()` — 查询 CommonUI 的平台 Trait 集合，判断是否包含 `Platform.Trait.ReplaySupport`
- `GetPlatformSupportTraitTag()` — 返回回放支持对应的 GameplayTag

**与 Session 的关系:**
`ULyraUserFacingExperienceDefinition::CreateHostingRequest()` 会在 `bRecordReplay == true` 且平台支持回放时，向 Session URL 参数中追加 `DemoRec`。这意味着 Playlist 可以声明“本局需要录制回放”，但最终是否生效仍受平台 Trait 控制。

---

### ULyraAssetManager [Runtime]

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

**类型:** 非 UObject C++ 结构体

**成员:**
- `static const FName Equipped` — 命名的 Asset Bundle 常量，用于 Experience 资产加载时按 Bundle 名称加载资产。

---

### ULyraGameData [Runtime]

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

## 框架内部关系

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

ULyraReplaySubsystem
  └── 读取 CommonUI PlatformTraits → 判断 Platform.Trait.ReplaySupport
```

---

## 关联框架

- [04-Game-Framework.md](04-Game-Framework.md) — ALyraWorldSettings 通过 GetDefaultPawnData 加载 PawnData
- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 加载通过 AssetManager 进行 Bundle 加载
- [09-GameplayTags-System.md](09-GameplayTags-System.md) — `Platform.Trait.ReplaySupport` 的分散定义
- [02-Engine-Configuration.md](02-Engine-Configuration.md) — DefaultEngine.ini 中的类注册映射
