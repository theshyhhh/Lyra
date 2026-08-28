# 07 - Experience 框架

> Experience 系统是 Lyra 的核心架构创新。它通过数据资产定义完整的"游戏体验"，再通过一个复制状态机驱动 GameFeature 插件加载和 GameFeatureAction 执行，从根本上改变 GameMode 管理玩法的方式。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 Experience → Local Settings（本地设置）回调改动。当前项目与
> Lyra 参考项目均声明 UE 5.7，底层机制按 Unreal Engine 5.7.4 源码核对；
> 尚未执行 PIE、多人、插件停用或设置重应用验证。

---

## 框架概述

**问题:** 传统的 GameMode 类承载所有玩法规则，每换一种玩法就要换一个 GameMode 类，且难以在运行时切换。

**当前项目的解决方案:** 将"玩法配置"从 GameMode 代码中抽离到数据资产（`ULyraExperienceDefinition`），通过 GameFeature 插件系统和可复用的 `UGameFeatureAction` 组合出任意玩法。核心状态机（`ULyraExperienceManagerComponent`）运行在 GameState 上并被复制，保证客户端与服务器各自进入对应加载流程。

**设计意图:**
- 一个地图可在运行时切换多种 Experience（例如从"大厅 Experience"切换到"对战 Experience"）
- Experience 通过组合 ActionSet 实现复用（DRY 原则）
- GameFeature 插件仅在需要时加载，减少内存占用
- 通过 PIE 引用计数防止一个 PIE 会话卸载另一个会话仍需要的插件
- 通过 UserFacingExperience 将前端 Playlist 和真正的 Experience Definition 解耦

---

## 当前复刻状态

| 模块 | 当前状态 | 当前实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| Experience 选择与加载 | **部分复刻** | 选择优先级、Bundle、插件激活、Action 和三档完成委托存在 | Lyra 有更完整失败、线上托管与卸载验证 | 基础主链可读，边缘分支仍需验证 |
| Experience 复制与客户端启动 | **部分复刻** | `CurrentExperience` 复制并由 `OnRep_CurrentExperience()` 启动客户端加载 | Lyra 同一模式 | 尚未执行真实多人到达顺序验证 |
| Action 反激活 | **部分复刻** | Deactivating Context（反激活上下文）和 pauser 计数存在 | Lyra 的完整 Action 生态会消费该机制 | 当前只具备框架，异步清理未验证 |
| 加载完成后重应用设置 | **结构占位** | 三档委托后主动调用 `ULyraSettingsLocal::OnExperienceLoaded()` | Lyra 会重应用设备配置相关的非分辨率设置 | 当前目标函数为空，调用无可见效果 |

---

## 类列表

| 类 | 父类/接口 | 生命周期 | 职责 |
|-----|----------|---------|------|
| `ULyraExperienceDefinition` | `UPrimaryDataAsset` | [Runtime + Editor-Only 验证] | 定义一个完整玩法的数据资产 |
| `ULyraExperienceActionSet` | `UPrimaryDataAsset` | [Runtime + Editor-Only 验证] | 可复用的 Action + 插件依赖打包 |
| `ULyraExperienceManagerComponent` | `UGameStateComponent`, `ILoadingProcessInterface` | [Runtime，复制] | Experience 加载/卸载状态机 |
| `ULyraExperienceManager` | `UEngineSubsystem` | [Runtime，编辑器核心逻辑] | PIE 多会话插件引用计数仲裁 |
| `ULyraUserFacingExperienceDefinition` | `UPrimaryDataAsset` | [Runtime] | 前端/Playlist 入口，创建 Host Session 请求并指定真实 Experience |
| `UAsyncAction_ExperienceReady` | `UBlueprintAsyncActionBase` | [Runtime，蓝图异步节点] | 蓝图等待 Experience 加载完成并广播 `OnReady` |

---

## 核心数据流

### 前端入口到真实 Experience

```
ULyraUserFacingExperienceDefinition (Playlist/前端卡片)
  ├── TileTitle / TileIcon / bShowInFrontEnd ──→ 前端展示
  ├── MapID ───────────────────────────────────→ HostSessionRequest.MapID
  ├── ExperienceID ────────────────────────────→ ExtraArgs["Experience"]
  ├── MaxPlayerCount / ExtraArgs ──────────────→ Session 创建参数
  └── bRecordReplay ───────────────────────────→ ExtraArgs["DemoRec"] (平台支持时)
        |
        v
CommonSession 创建/旅行到地图
        |
        v
ULyraExperienceManagerComponent 加载 ExperienceID 对应的 ULyraExperienceDefinition
```

`ULyraUserFacingExperienceDefinition` 面向玩家和前端 UI，`ULyraExperienceDefinition` 面向运行时玩法系统。前者决定“玩家选择哪一局、用哪张地图、Session 参数是什么”，后者决定“进入地图后加载哪些 GameFeature、ActionSet 和 PawnData”。

### 运行时 Experience 加载

```
ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne()
  ├── URL ?Experience=
  ├── PIE DeveloperSettings::ExperienceOverride
  ├── CommandLine -Experience=
  ├── ALyraWorldSettings::DefaultGameplayExperience
  ├── DedicatedServer Host
  └── Default LyraExperienceDefinition:B_LyraDefaultExperience
        |
        v
ULyraExperienceManagerComponent::SetCurrentExperience(FPrimaryAssetId)
        |
        v
ULyraExperienceDefinition (数据资产)
  ├── GameFeaturesToEnable[] ────────→ GameFeature 插件 URL
  ├── Actions[]: UGameFeatureAction* ──→ OnGameFeatureRegistering/Loading/Activating
  ├── DefaultPawnData: ULyraPawnData* ─→ 生成哪个 Pawn 类
  └── ActionSets[]: ULyraExperienceActionSet*
        ├── Actions[]: UGameFeatureAction*
        └── GameFeaturesToEnable[]
```

蓝图/Widget 等运行时对象如果只关心“Experience 已经可用”，可以通过 `UAsyncAction_ExperienceReady::WaitForExperienceReady()` 等待：

```
WaitForExperienceReady(WorldContextObject)
  ├── World 已有 GameState → 直接监听 ULyraExperienceManagerComponent
  ├── World 尚无 GameState → 先等待 World::GameStateSetEvent
  ├── Experience 已 Loaded → 下一帧广播 OnReady
  └── Experience 未 Loaded → 注册 OnExperienceLoaded 后广播 OnReady
```

---

## 逐类详解

### ULyraExperienceDefinition [Runtime]

**继承链:** `UObject → UDataAsset → UPrimaryDataAsset → ULyraExperienceDefinition`

**UCLASS:** `UCLASS(BlueprintType, Const)`

**职责:** 定义一个完整"游戏体验"的数据资产。可在编辑器中创建和配置。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `GameFeaturesToEnable` | `TArray<FString>` | 此 Experience 需要激活的 GameFeature 插件名称 |
| `Actions` | `TArray<TObjectPtr<UGameFeatureAction>>` (Instanced) | 负责安装/卸载玩法的 Action 实例 |
| `DefaultPawnData` | `TObjectPtr<const ULyraPawnData>` | 此 Experience 默认使用的 Pawn 类 |
| `ActionSets` | `TArray<TObjectPtr<ULyraExperienceActionSet>>` | 引用的可复用 ActionSet 资产 |

**编辑器特性 (Editor-Only):**
- `IsDataValid()`: 验证 Actions 配置有效性，确保蓝图资产直接继承自对应 C++ 类
- `UpdateAssetBundleData()`: 向 Asset Manager 注册间接引用的资源，确保 Cook/Chunk 打包时正确包含

**用法:** 关卡制作者在 `ALyraWorldSettings::DefaultGameplayExperience` 中选择一个 Experience 资产。运行时，`ULyraExperienceManagerComponent` 加载该资产并执行其 Actions。

实际进入加载前，`ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne()` 会按 URL、PIE 开发设置、命令行、WorldSettings、Dedicated Server 和默认兜底的顺序决定最终 `ExperienceId`。

---

### ULyraUserFacingExperienceDefinition [Runtime]

**继承链:** `UObject → UDataAsset → UPrimaryDataAsset → ULyraUserFacingExperienceDefinition`

**职责:** 前端/Playlist 数据资产。它持有 `MapID`、`ExperienceID`、展示文本、图标、加载界面、最大人数、额外 URL 参数和回放开关，并通过 `CreateHostingRequest()` 生成 `UCommonSession_HostSessionRequest`。

**关键点:**
- `ExperienceID` 不直接加载玩法，而是写入 `ExtraArgs["Experience"]`，供后续旅行/Experience 选择流程使用。
- `ModeNameForAdvertisement` 使用该 UserFacingExperience 自身的 PrimaryAssetName，适合作为在线会话广告中的模式名。
- `bRecordReplay` 需要同时满足平台 Trait `Platform.Trait.ReplaySupport` 才会追加 `DemoRec`。
- AssetManager 必须扫描 `LyraUserFacingExperienceDefinition` 类型，否则前端无法通过 Primary Asset 发现这些 Playlist 资产。

更完整的字段说明见 [04-Game-Framework.md](04-Game-Framework.md)。

---

### ULyraExperienceActionSet [Runtime]

**继承链:** `UObject → UDataAsset → UPrimaryDataAsset → ULyraExperienceActionSet`

**UCLASS:** `UCLASS(BlueprintType, NotBlueprintable)`

**职责:** 将一组 GameFeatureAction 和插件依赖打包为可复用的命名资产。多个 Experience 可以共享同一个 ActionSet。

**属性:**
- `Actions` — 与 ExperienceDefinition 相同类型的 Action 数组
- `GameFeaturesToEnable` — 此 ActionSet 需要的 GameFeature 插件名称

**设计模式:** 组合模式（Composite Pattern）。例如，"基础 HUD ActionSet"可以被多个 Experience 引用，而不需要在每个 Experience 中重复配置相同的 HUD Actions。

---

### ULyraExperienceManagerComponent [Runtime，复制]

**继承链:** `UActorComponent → UGameStateComponent → ULyraExperienceManagerComponent`
**实现接口:** `ILoadingProcessInterface`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** Experience 加载/卸载的运行时状态机。这是 Lyra 最复杂的类。

---

#### 状态机

```
SetCurrentExperience()
        |
        v
   Unloaded ──→ Loading ──→ LoadingGameFeatures ──╮
        ^                                          │
        |                          [可选延迟] LoadingChaosTestingDelay
        |                                          │
        |                                          v
   Deactivating ←── Loaded ←── ExecutingActions
        |               (所有委托广播)
        v
   Unloaded (CurrentExperience = nullptr)
```

---

#### 加载流程（详细信息）

**1. `SetCurrentExperience(FPrimaryAssetId)` — 入口**
   - 通过 AssetManager 同步加载 Experience 的类默认对象（CDO）
   - 断言 CurrentExperience 为空（不支持运行时切换，只能从 Unloaded 状态加载）
   - 调用 `StartExperienceLoad()`

**2. `StartExperienceLoad()` — 资产 Bundle 加载**
   - 收集需要加载 Bundle 的资产列表：Experience 自身 + 所有 ActionSet 的 PrimaryAssetId
   - 确定要加载的 Bundle：
     - `FLyraBundles::Equipped` — 始终加载
     - `LoadStateClient` — 非 DedicatedServer 时加载
     - `LoadStateServer` — 非客户端时加载
   - 通过 `UAssetManager::ChangeBundleStateForPrimaryAssets()` 高优先级异步加载
   - 也支持直接加载软引用资源列表（`RawAssetList`，当前未使用）
   - 合并所有异步加载操作为一个 CombinedHandle
   - 加载完成后（或已提前完成）调用 `OnExperienceLoadComplete()`
   - 额外加载预加载资产列表（不阻塞、不绑定回调）

**3. `OnExperienceLoadComplete()` — 插件识别与激活**
   - 从 Experience 和所有 ActionSet 收集 GameFeature 插件名称
   - 通过 `UGameFeaturesSubsystem::GetPluginURLByName()` 将名称转为 URL
   - 去重后保存到 `GameFeaturePluginURLs`
   - 若无插件需加载 → 直接进入 `OnExperienceFullLoadCompleted()`
   - 若有插件需加载 → 状态切换为 `LoadingGameFeatures`
   - 对每个插件 URL：
     - 调用 `ULyraExperienceManager::NotifyOfPluginActivation()`（PIE 引用计数 +1）
     - 调用 `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin()`
     - 每个插件完成时回调 `OnGameFeaturePluginLoadComplete()`

**4. `OnGameFeaturePluginLoadComplete()` — 计数等待**
   - `NumGameFeaturePluginsLoading` 递减
   - 所有插件加载完毕（计数为 0）→ `OnExperienceFullLoadCompleted()`

**5. `OnExperienceFullLoadCompleted()` — Action 执行与委托广播**
   - 【可选】混沌测试延迟（CVars: `lyra.chaos.ExperienceDelayLoad.MinSecs` + `.RandomSecs`）
   - 创建 `FGameFeatureActivatingContext`（限定作用于当前 World Context）
   - 依次执行 Experience 和所有 ActionSet 中每个非空 Action 的三个阶段；空 Action 会被跳过，避免配置数组中存在 nullptr 时直接崩溃：
     1. `OnGameFeatureRegistering()` — 注册全局信息（GameplayTag、组件请求等）
     2. `OnGameFeatureLoading()` — 加载/准备数据
     3. `OnGameFeatureActivating(Context)` — 实际生效（添加组件、输入映射、UI、技能等）
   - 状态设为 `Loaded`
   - 按三种优先级广播委托并清空：
     1. `OnExperienceLoaded_HighPriority` → Broadcast → Clear
     2. `OnExperienceLoaded` → Broadcast → Clear
     3. `OnExperienceLoaded_LowPriority` → Broadcast → Clear
   - 三档委托全部广播并清空后，在 `#if !UE_SERVER` 编译分支调用
     `ULyraSettingsLocal::Get()->OnExperienceLoaded()`。调用点已经接入，但当前
     目标函数为空，因此不会重应用画质、性能或设备配置。

> ⚠️ **注意：** `#if !UE_SERVER` 是 Compile Target（编译目标）条件，不是
> `NetMode`（网络模式）判断。专用服务器 Target 会排除该代码；Editor、Client
> 和 Listen Server（监听服务器）所在的非 `UE_SERVER` 构建会包含该调用。

> 🔍 **Lyra 对比：** Lyra 的 `OnExperienceLoaded()` 会进入
> `ReapplyThingsDueToPossibleDeviceProfileChange()`，最终重应用非分辨率设置；
> 当前项目只复刻了调用时机，没有复刻目标行为。

---

#### 卸载流程

##### `EndPlay(const EEndPlayReason::Type EndPlayReason)`

> ⏱️ **引擎调用时机:** Component（及所属 GameState）被销毁时。在 Owner Actor 的 EndPlay 之前调用。`EndPlayReason` 区分 `Destroyed`/`LevelTransition`/`EndPlayInEditor`/`RemovedFromWorld`/`Quit`。
>
> **适合写的逻辑:** 清理 Component 特有资源、取消异步操作、通知关联系统。⚠️ 此时 Owner Actor 仍存在，可以安全访问。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 5](17-Engine-Lifecycle-Reference.md#5-uactorcomponent-生命周期)

**`EndPlay()` 触发卸载:**
1. 遍历 `GameFeaturePluginURLs`，对每个插件：
   - 调用 `ULyraExperienceManager::RequestToDeactivatePlugin(PluginURL)`
   - 仅当引用计数归零时（返回 true）才真正调用 `DeactivateGameFeaturePlugin()`
2. 若 Experience 已完成加载（LoadState == Loaded）：
   - 创建 `FGameFeatureDeactivatingContext`（含异步完成回调 `OnActionDeactivationCompleted()`）
   - 反向执行所有 Action 的反激活：
     - `OnGameFeatureDeactivating(Context)` — 卸载组件、移除技能、回滚 Tag
     - `OnGameFeatureUnregistering()` — 注销全局注册
   - 在循环结束后读取 `Context.GetNumPausers()` 获取异步反激活的数量
   - 若无 pauser → `OnAllActionsDeactivated()` → 状态回到 `Unloaded`

> ⚠️ **异步反激活:** 框架已搭建（pauser 计数机制），但实际测试和验证尚未完成。异步反激活触发 Error 级别日志作为开发者诊断提示。

---

#### 关键属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `CurrentExperience` | `TObjectPtr<const ULyraExperienceDefinition>` | 当前加载的 Experience（复制） |
| `LoadState` | `ELyraExperienceLoadState` | 当前加载阶段（不复制） |
| `NumGameFeaturePluginsLoading` | `int32` | 正在加载的插件计数 |
| `GameFeaturePluginURLs` | `TArray<FString>` | 已激活的所有插件 URL |
| `NumObservedPausers` | `int32` | 已完成的异步反激活计数 |
| `NumExpectedPausers` | `int32` | 预期的异步反激活总数（初始值 INDEX_NONE） |

**关键查询方法:**
- `IsExperienceLoaded()` — 判断 `LoadState == Loaded` 且 `CurrentExperience != nullptr`。
- `GetCurrentExperienceChecked()` — 仅允许在 Loaded 状态调用，返回当前 Experience；`ALyraGameMode::GetPawnDataForController()` 用它读取 `DefaultPawnData`。

---

### ULyraExperienceManager [Runtime，编辑器核心逻辑]

**继承链:** `UObject → USubsystem → UEngineSubsystem → ULyraExperienceManager`

**UCLASS:** `UCLASS(MinimalAPI)`

**职责:** 在编辑器进程中跨 PIE 会话仲裁 GameFeature 插件激活/卸载。

**问题背景:** 编辑器中可以同时打开多个 PIE 窗口。若一个 PIE 会话关停时直接卸载插件，另一个仍在运行的 PIE 会话可能崩溃或异常。

**解决方案 — 引用计数:**
- `GameFeaturePluginRequestCountMap`: `TMap<FString, int32>` — 每个插件 URL 的引用计数
- `NotifyOfPluginActivation(PluginURL)`: 计数 +1
- `RequestToDeactivatePlugin(PluginURL)`: 计数 -1 → 仅当计数归零时返回 true（允许卸载）

**编辑器集成:**
- `OnPlayInEditorBegun()`: 由 `FLyraEditorModule::OnBeginPIE` 调用，确保 PIE 开始时计数表为空

**非编辑器构建:**
- `NotifyOfPluginActivation`: 空操作
- `RequestToDeactivatePlugin`: 始终返回 true（不阻止卸载）

---

### UAsyncAction_ExperienceReady [Runtime，蓝图异步节点]

**继承链:** `UObject → UBlueprintAsyncActionBase → UAsyncAction_ExperienceReady`

**UCLASS:** `UCLASS()`

**职责:** 给蓝图提供“等待当前 World 的 Experience 加载完成”的异步节点。适合 Widget、测试蓝图或 GameFeature Action 生成的蓝图对象，在不直接持有 ExperienceManagerComponent 的情况下等待玩法系统就绪。

**关键 API:**
- `WaitForExperienceReady(UObject* WorldContextObject)` — BlueprintCallable，`BlueprintInternalUseOnly=true`，会创建异步 Action 并 `RegisterWithGameInstance(World)`，保证节点在 `SetReadyToDestroy()` 前不会被 GC。
- `OnReady` — `FExperienceReadyAsyncDelegate`，Experience 已就绪后广播。

**执行流程:**
1. `Activate()` 从弱引用 `WorldPtr` 取 World。
2. 如果 `World->GetGameState()` 已存在，直接进入 `Step2_ListenToExperienceLoading()`。
3. 如果 GameState 尚未创建，先绑定 `World->GameStateSetEvent`，等 GameState 设置后再监听 Experience。
4. 若 `ULyraExperienceManagerComponent::IsExperienceLoaded()` 已经为 true，使用 `SetTimerForNextTick()` 下一帧广播 `OnReady`。这样可以避免调用方依赖“节点激活时立即完成”的偶发时序。
5. 若 Experience 尚未 Loaded，调用 `CallOrRegister_OnExperienceLoaded()` 注册回调。
6. `Step4_BroadcastReady()` 广播 `OnReady` 后调用 `SetReadyToDestroy()`。

**注意:** 源码 TODO 指出，后续应区分启动加载期和运行期：启动期保持下一帧扰动，运行期动态创建对象可以考虑立即回调；这类时序扰动也更适合下沉到 Experience 加载系统本身，而不是每个等待节点单独实现。

---

## 委托优先级系统

注册 Experience 加载完成的回调支持三种优先级：

| 优先级 | 典型使用者 | 使用场景 |
|--------|-----------|---------|
| **HighPriority** | 核心系统（UI、输入） | 需要在其他系统之前设置的 |
| **Normal** | 一般游戏系统 | 多数功能 |
| **LowPriority** | 依赖其他系统已完成初始化的 | 最后设置的功能 |

**使用方式:**
```cpp
ExperienceComponent->CallOrRegister_OnExperienceLoaded(
    FOnLyraExperienceLoaded::FDelegate::CreateUObject(this, &MyClass::OnExperienceLoaded)
);
```
若 Experience 已完成加载，委托会立即执行；否则注册等待加载完成。

---

## 加载屏幕集成

##### `ShouldShowLoadingScreen(FString& OutReason) const`

> ⏱️ **引擎调用时机:** CommonLoadingScreen 插件每帧查询，决定是否显示加载画面。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 5](17-Engine-Lifecycle-Reference.md#5-uactorcomponent-生命周期)

通过 `ILoadingProcessInterface::ShouldShowLoadingScreen()` 与 CommonLoadingScreen 插件联动：
- `LoadState != Loaded` → 返回 true → 显示 `DefaultGame.ini` 中配置的 `W_LoadingScreen_Host`
- `LoadState == Loaded` → 返回 false → 隐藏加载画面

当前 CommonLoadingScreen 默认配置：

```ini
[/Script/CommonLoadingScreen.CommonLoadingScreenSettings]
LoadingScreenWidget = /Game/UI/Foundation/LoadingScreen/W_LoadingScreen_Host.W_LoadingScreen_Host_C
ForceTickLoadingScreenEvenInEditor = False
```

---

## 已识别的 TODO（来自源文件注释）

1. 异步加载 Experience Definition 本身（当前为同步 TryLoad）
2. 显式失败处理（当前使用 check() 断言，会导致崩溃而非优雅处理）
3. 分阶段 Action 执行（当前所有 Action 在单次遍历中执行，无阶段区分）
4. 完整停用/卸载支持（异步反激活尚未完全测试）
5. 预加载资产清理策略
6. GameFeature 卸载（当前在不同 Experience 之间"泄漏"已加载的插件）
7. 切换 Experience 时的差异比较（仅卸载有变化的部分）
8. 内置插件 vs URL 式插件的处理区分

### 设置回调 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 中 | Lyra 对比 TODO | 实现 `ULyraSettingsLocal::OnExperienceLoaded()` 的设备配置重应用 | 调用已启用，函数体为空 | 明确当前项目所需的 Device Profile（设备配置）与画质设置 |
| 中 | 验证 TODO | 验证 Client、Listen Server 与 Dedicated Server Target 的执行差异 | 当前使用 `#if !UE_SERVER` | 三种构建 / 启动模式和日志 |

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraAssetManager 提供 Bundle 加载能力
- [04-Game-Framework.md](04-Game-Framework.md) — ALyraWorldSettings::DefaultGameplayExperience 指定使用哪个 Experience；ExperienceManagerComponent 运行在 ALyraGameState 上
- [06-Character-Framework.md](06-Character-Framework.md) — DefaultPawnData 决定生成哪个 Pawn
- [02-Engine-Configuration.md](02-Engine-Configuration.md) — AssetManager 扫描 `LyraExperienceDefinition` 和 `LyraUserFacingExperienceDefinition`
- [12-Editor-Module.md](12-Editor-Module.md) — FLyraEditorModule 调用 ULyraExperienceManager::OnPlayInEditorBegun
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — 状态机中的 8 个 TODO
- [08-UI-Framework.md](08-UI-Framework.md) — `OnExperienceLoaded()` 的设置目标、LocalPlayer 缓存与 Shared Settings 边界
