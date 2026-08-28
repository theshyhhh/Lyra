# 08 - UI 与本地设置框架

> 本框架连接 HUD（Heads-Up Display，抬头显示）、CommonUI 根布局、视口、
> 机器级本地设置和每个本地用户的共享设置。当前 UI 主链已经接入；最新工作区
> 代码补上了 LocalPlayer（本地玩家）与设置系统的结构和入口，但共享设置的
> 异步加载、应用、保存及实际设置字段仍未完成。

> **核对基线：** 当前工作区以提交 `19e6961` 为基线，并包含 2026-08-28
> 尚未提交的 `LyraLocalPlayer`、`LyraSettingsLocal`、`LyraSettingsShared`、
> `LyraGameInstance` 和 `LyraExperienceManagerComponent` 修改。当前项目与
> Lyra 参考项目均声明 UE 5.7；底层机制按 Unreal Engine 5.7.4 源码核对。
> 尚未执行编译、PIE（Play In Editor，编辑器内运行）、多本地玩家、登录、
> 存档或音频设备切换验证。

---

## 框架概述

**问题：** UI 可见性、机器性能设置和玩家偏好属于不同生命周期。HUD 跟随
World 和 Controller（控制器）；图形设置跟随本机进程；按键、辅助功能等用户
偏好应按本地账号保存，不能让同一台机器上的所有玩家共用一份状态。

**当前项目的解决方案：** `ALyraHUD` 和 `ULyraUIManagerSubsystem` 负责
显示层；`ULyraSettingsLocal` 作为 `UGameUserSettings` 的项目类型保存机器级
设置；`ULyraSettingsShared` 继承 `ULocalPlayerSaveGame`，计划按 LocalPlayer
和平台用户保存可共享偏好；`ULyraLocalPlayer` 负责缓存设置对象，并把登录、
音频设备及 Controller 变化连接到对应系统。

**设计意图：**

- HUD 保持轻量，实际界面通过 CommonUI 和 UIExtension 组合。
- `ULyraSettingsLocal` 保存分辨率、质量和设备等机器相关设置。
- `ULyraSettingsShared` 保存每个本地用户的偏好，并允许以后接入云存档。
- LocalPlayer 在跨地图期间持续存在，适合作为“当前用户设置”的缓存和桥梁。
- Experience（体验）加载完成后再重应用受 DeviceProfile（设备配置文件）影响
  的本地设置。

**当前完成度：** UI 显示主链与 Lyra 参考实现基本一致；设置对象的类型、
注册、同步创建和调用入口已经存在，但异步登录加载必定返回失败，应用与保存
闭环尚未形成。

---

## 当前复刻状态

| 模块 | 当前状态 | 当前实现 | Lyra 对应内容 | 影响 |
|---|---|---|---|---|
| HUD 与 Modular Gameplay（模块化游戏玩法）接入 | **已复刻** | HUD 注册为组件接收者，发送 Ready 事件并在 EndPlay 注销 | Lyra 同名生命周期路径 | C++ 主链已对齐，UI 资产仍需验证 |
| 视口光标策略 | **已复刻** | 根据平台 Trait（特征标签）选择软/硬件光标 | Lyra 同名实现 | 行为依赖最终平台 Trait 配置 |
| UI 根布局可见性同步 | **已复刻** | 每帧同步 `PrimaryGameLayout` 与 `AHUD::bShowHUD` | Lyra 同名实现 | 默认 UI Policy 资产需运行验证 |
| Local Settings（本地设置）扩展 | **结构占位** | 有单例访问器、三个 Editor CVar、音频事件和空 `OnExperienceLoaded()` | Lyra 包含完整性能、音频、输入和 Replay 设置 | Experience 回调已进入，但不会重应用设置 |
| Shared Settings（共享设置）对象 | **部分复刻** | 可同步创建/加载 `SharedGameSettings`，LocalPlayer 以 `UPROPERTY` 缓存 | Lyra 还有数据版本、设置字段、Apply、Save 和 Enhanced Input 协作 | 当前对象主要是 SaveGame 外壳 |
| 登录后异步加载共享设置 | **结构占位** | GameInstance 已调用 LocalPlayer；最终函数直接返回 `false` | Lyra 调用 UE 异步 SaveGame API，完成后 Apply 并回调 | 登录链每次都会触发 `ensure(false)`，缓存不会被替换 |
| 音频输出设备切换 | **部分复刻** | LocalPlayer 监听事件并调用 Audio Mixer（音频混合器）切换设备 | Lyra 的设置 Setter 会产生事件 | 当前没有 C++ 广播者，失败分支为空 |
| Controller 的共享设置消费 | **未复刻** | `ALyraPlayerController::SetPlayer()` 中绑定和首次应用仍被注释 | Lyra 用共享设置驱动力反馈等状态 | 设置对象即使取得，也不会更新 Controller |

---

## 类列表

| 类 | 父类或接口 | 生命周期 | 网络位置 | 当前状态 | 职责 |
|---|---|---|---|---|---|
| `ALyraHUD` | `AHUD` | Runtime，随 Controller / World | Server 与 Owning Client 按引擎规则存在 | **已复刻** | Modular HUD 扩展点与 GAS 调试 Actor 列表 |
| `ULyraGameViewportClient` | `UCommonGameViewportClient` | Runtime，视口级 | 仅本地，不复制 | **已复刻** | 按平台 Trait 决定软件光标策略 |
| `ULyraUIManagerSubsystem` | `UGameUIManagerSubsystem` | Runtime，随 GameInstance | 仅本地，不复制 | **已复刻** | 管理 UI Policy 并同步根布局可见性 |
| `ULyraSettingsLocal` | `UGameUserSettings` | Runtime，进程 / 机器级 | 每个进程本地，不复制 | **结构占位** | 机器级设置入口与 Experience / 音频事件钩子 |
| `ULyraSettingsShared` | `ULocalPlayerSaveGame` | Runtime，按 LocalPlayer / 平台用户 | 本地 SaveGame，不复制 | **部分复刻** | 每用户共享设置对象、同步创建与加载外壳 |
| `ULyraLocalPlayer` | `UCommonLocalPlayer`、`ILyraTeamAgentInterface` | Runtime，跨 World 的本地用户对象 | 仅本机，不复制 | **部分复刻** | 缓存设置、响应登录、切换音频并转发队伍状态 |

---

## 核心数据流

### 登录成功后的共享设置加载

**阅读目标：** 弄清楚 CommonUser 登录成功后，当前代码如何尝试把账号对应的
共享设置加载到 `ULyraLocalPlayer`，以及流程实际停止在哪里。

**起点：** `UCommonUserSubsystem::HandleUserInitializeSucceeded()` 广播
`OnUserInitializeComplete`。

**终点：** 设计终点是 `ULyraLocalPlayer::SharedSettings` 被真实存档对象替换；
当前实际终点是异步请求未被调度。

```text
[CommonUser，当前已实现]
HandleUserInitializeSucceeded()
  `-- OnUserInitializeComplete.Broadcast(..., bSuccess=true)
        |
        v
[CommonGame，当前已绑定]
ULyraGameInstance::HandlerUserInitialized()
  |-- Super::HandlerUserInitialized()
  |-- bSuccess && UserInfo 有效
  `-- GetLocalPlayerByIndex(UserInfo->LocalPlayerIndex)
        |
        v
[当前项目已实现]
ULyraLocalPlayer::LoadSharedSettingsFromDisk()
  |-- 比较 CurrentNetId、SharedSettings、NetIdForSharedSettings
  `-- ULyraSettingsShared::AsyncLoadOrCreateSettings(...)
        |
        v
[当前结构占位]
直接 return false
  |-- ensure(false)
  |-- 不调用 UE 的 AsyncLoadOrCreateSaveGameForLocalPlayer()
  `-- 不触发 OnSharedSettingsLoaded()
```

每个节点的职责：

1. CommonUser 在下一帧完成用户状态切换并广播结果。
2. GameInstance 只在成功且 `UserInfo` 有效时查找对应 LocalPlayer；专用服务器
   用户没有 LocalPlayer，因此安全跳过。
3. LocalPlayer 用缓存的 Unique Net ID（唯一网络 ID）避免同一账号重复加载。
4. Shared Settings 包装函数本应把请求交给引擎，但当前函数体直接返回
   `false`，所以没有异步任务和完成回调。

> ⚠️ **注意：** 当前只能说“登录入口已经接入”，不能说“登录后设置已加载”。

### 首次按需获取共享设置

```text
调用 ULyraLocalPlayer::GetSharedSettings()
  |-- SharedSettings 已缓存 -> 直接返回
  `-- 尚未缓存
        |-- PLATFORM_DESKTOP == true
        |     `-- LoadOrCreateSettings()
        |           `-- UE 同步检查磁盘；失败或不存在时创建新对象
        `-- PLATFORM_DESKTOP == false
              `-- CreateTemporarySettings()
                    `-- 创建可关联 LocalPlayer 和 Slot 的临时对象

ULocalPlayerSaveGame::InitializeSaveGame()
  |-- OwningPlayer = 当前 LocalPlayer
  `-- SaveSlotName = "SharedGameSettings"

ULyraLocalPlayer::SharedSettings = 返回对象
  `-- UPROPERTY(Transient) 强引用防止 GC
```

该路径在桌面平台会同步阻塞 Game Thread（游戏线程）检查磁盘。当前 C++ 中没有
活动消费者调用 `GetSharedSettings()`；`ALyraPlayerController::SetPlayer()` 的
调用仍被注释，因此运行时是否有蓝图调用必须标记为待资产验证。

### Experience 完成后的本地设置通知

```text
ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()
  |-- 执行 Experience / ActionSet Actions
  |-- LoadState = Loaded
  |-- High -> Normal -> Low Priority 委托依次广播并清空
  `-- #if !UE_SERVER
        `-- ULyraSettingsLocal::Get()->OnExperienceLoaded()
              `-- 当前空实现
```

这条调用位于所有 Experience Loaded 委托之后。`#if !UE_SERVER` 是编译条件，
不是运行时 `NetMode` 判断；UE_SERVER 目标会移除该代码，Editor / Client /
Listen Server 构建仍包含它。当前函数为空，所以不会调用 Lyra 参考实现中的
`ReapplyThingsDueToPossibleDeviceProfileChange()`。

### 音频输出设备切换

```text
ULyraLocalPlayer::PostInitProperties()
  `-- 绑定 ULyraSettingsLocal::OnAudioOutputDeviceChanged

[当前缺少 C++ 生产者]
设置修改 -> OnAudioOutputDeviceChanged.Broadcast(DeviceId)
  |
  v
ULyraLocalPlayer::OnAudioOutputDeviceChanged(DeviceId)
  `-- UAudioMixerBlueprintLibrary::SwapAudioOutputDevice()
        `-- OnCompletedAudioDeviceSwap(Result)
              `-- Failure 分支当前为空
```

监听者和 Audio Mixer 消费端已经存在，但 `ULyraSettingsLocal` 没有
`SetAudioOutputDeviceId()` 或其他广播调用，因此当前静态 C++ 主链在生产端
中断。

---

## 初始化与运行流程

| 对象 | 创建 / 初始化入口 | 所有权与 GC | 清理或替换 |
|---|---|---|---|
| `ULyraSettingsLocal` | 引擎按 `GameUserSettingsClassName` 创建；`Get()` 从 `GEngine` 取得 | 由 Engine 持有，进程级对象 | 按 `UGameUserSettings` 生命周期管理 |
| `ULyraLocalPlayer` | GameInstance 按 `LocalPlayerClassName` 创建；`PostInitProperties()` 绑定音频事件 | GameInstance 管理；跨 World；本地对象 | LocalPlayer 移除后由引擎与 GC 管理；`AddUObject` 绑定不会强持有它 |
| `ULyraSettingsShared` | `UGameplayStatics::CreateSaveGameObject()` 创建，Outer 为 Transient Package（瞬态包） | `SharedSettings` 强引用；对象自身以 `OwningPlayer` 关联 LocalPlayer | 异步加载成功时应替换旧对象；旧对象无根引用后可被 GC |
| `ULyraUIManagerSubsystem` | GameInstance Subsystem 自动创建并 `Initialize()` | Outer 为 GameInstance | `Deinitialize()` 移除 Ticker 句柄 |
| `ALyraHUD` | PlayerController / GameMode HUD 类路径创建 | World Actor | `EndPlay()` 注销组件接收者 |

> 🧩 **引擎机制：** UE 5.7.4 的 `ULocalPlayerSaveGame` 在同步或异步加载失败
> 时会创建一个新对象，并通过 `InitializeSaveGame()` 设置 `OwningPlayer`、
> Slot 和数据版本；当前项目的异步包装没有调用这条引擎路径。

---

## 逐类详解

### ALyraHUD [Runtime]

**当前状态：** **已复刻**

**继承链：** `UObject → AActor → AHUD → ALyraHUD`

**职责：** 作为传统 HUD 与 Modular Gameplay 的连接点。实际界面由 CommonUI
布局和 GameFeature Action（游戏功能动作）注入，HUD 自身负责 Ready 事件、
注销及 GAS 调试对象收集。

**关键生命周期：**

- `PreInitializeComponents()`：调用 `Super` 后注册
  `AddGameFrameworkComponentReceiver(this)`。
- `BeginPlay()`：在 `Super` 前发送 `NAME_GameActorReady` 扩展事件。
- `EndPlay()`：注销接收者后调用 `Super`，与初始化路径对称。
- `GetDebugActorList()`：收集非 CDO / Archetype 的 ASC Owner 或 Avatar Actor。

### ULyraGameViewportClient [Runtime]

**当前状态：** **已复刻**

**继承链：**
`UObject → UGameViewportClient → UCommonGameViewportClient → ULyraGameViewportClient`

**职责：** `Init()` 读取 `Platform.Trait.Input.HardwareCursor`；有硬件光标时
禁用软件光标 Widget（部件），否则使用项目配置的软件光标。

**注册：**
`Config/DefaultEngine.ini` 的
`GameViewportClientClassName=/Script/LyraGame.LyraGameViewportClient`。

### ULyraUIManagerSubsystem [Runtime]

**当前状态：** **已复刻**

**继承链：**
`UObject → USubsystem → UGameInstanceSubsystem → UGameUIManagerSubsystem → ULyraUIManagerSubsystem`

**职责：** 读取默认 `UGameUIPolicy`，为每个 LocalPlayer 管理
`UPrimaryGameLayout`，并通过 Ticker 每帧同步根布局与 HUD 开关。

```text
Tick()
  `-- SyncRootLayoutVisibilityToShowHUD()
        |-- 遍历 GameInstance.LocalPlayers
        |-- LocalPlayer -> PlayerController -> HUD
        |-- HUD 存在且 bShowHUD == false -> Collapsed
        `-- 其他情况 -> SelfHitTestInvisible
```

### ULyraSettingsLocal [Runtime，进程 / 机器级]

**当前状态：** **结构占位**

**源码位置：**

- 当前项目：`Source/LyraGame/Settings/LyraSettingsLocal.h/.cpp`
- Lyra 参考：`Source/LyraGame/Settings/LyraSettingsLocal.h/.cpp`
- 引擎父类：`Engine/Source/Runtime/Engine/Classes/GameFramework/GameUserSettings.h`

**继承链：** `UObject → UGameUserSettings → ULyraSettingsLocal`

**职责：** 当前只提供项目类型、全局访问器和若干扩展钩子；分辨率、窗口模式
等可用能力仍主要来自 `UGameUserSettings` 父类，不能算作 Lyra 专属设置已复刻。

**关键成员与方法：**

| 成员 / 方法 | 当前行为 | 状态 |
|---|---|---|
| `Get()` | 从 `GEngine->GetGameUserSettings()` 取得并 `CastChecked` | **已复刻** |
| 三个 Editor CVar | 声明 PIE 帧率、前端性能和 DeviceProfile 开关 | **部分复刻** |
| `OnExperienceLoaded()` | 函数存在但为空 | **结构占位** |
| `ShouldAutoRecordReplays()` | 固定返回 `false`，没有 Setter 或 Config 字段 | **结构占位** |
| `OnAudioOutputDeviceChanged` | Event（事件）类型和实例存在，没有广播调用 | **结构占位** |

> ⚠️ **注意：** `DECLARE_EVENT_OneParam` 的 Owner 参数当前写成
> `ULyraSettingsLocall`（多一个 `l`），与真实类名和 Lyra 参考实现不同。
> UE 5.7.4 的 Event 宏只增加 `friend class OwningType`，且源码明确说明这种
> Owner 约束并未强制执行，所以不能据此断言当前一定编译失败；但该拼写破坏了
> 预期所有权语义，应在实现广播前修正。

### ULyraSettingsShared [Runtime，每本地用户]

**当前状态：** **部分复刻**

**源码位置：**

- 当前项目：`Source/LyraGame/Settings/LyraSettingsShared.h/.cpp`
- Lyra 参考：`Source/LyraGame/Settings/LyraSettingsShared.h/.cpp`
- 引擎父类：
  `Engine/Source/Runtime/Engine/Classes/GameFramework/SaveGame.h`
  与 `Private/GameFramework/SaveGame.cpp`

**继承链：**
`UObject → USaveGame → ULocalPlayerSaveGame → ULyraSettingsShared`

**一句话职责：** 它是计划保存“可随用户账号移动、且按本地玩家隔离”的偏好
数据的 SaveGame（存档）对象，不负责机器分辨率或图形质量。

**创建位置与所有权：**

- Slot 名固定为 `SharedGameSettings`。
- `CreateTemporarySettings()` 调用父类静态辅助函数创建并初始化新对象。
- `LoadOrCreateSettings()` 在 Game Thread 同步检查磁盘；不存在或加载失败时，
  引擎创建新对象。
- SaveGame Outer 为 Transient Package；父类 `OwningPlayer` 指向 LocalPlayer。
- LocalPlayer 的 `SharedSettings` 是 `UPROPERTY(Transient)` 强引用，负责缓存并
  防止对象提前被 GC。

**当前限制：**

- `AsyncLoadOrCreateSettings()` 直接返回 `false`。
- `ApplySettings()` 调用被注释，当前类也没有这个函数。
- 没有 Lyra 的设置字段、数据版本、Dirty（脏标记）、`SaveSettings()`、字幕、
  语言、辅助功能、输入灵敏度或 Enhanced Input 用户设置协作。
- `OnSettingChanged` 只有声明，没有当前广播者。

### ULyraLocalPlayer [Runtime，本地桥梁]

**当前状态：** **部分复刻**

**职责：** 连接当前本地用户、Controller、Team（队伍）、Local Settings、
Shared Settings 和 Audio Mixer。其 Team 细节见
[05-Player-Framework.md](05-Player-Framework.md#ulyralocalplayer-runtime)。

**设置相关入口：**

- `PostInitProperties()`：监听全局音频设备事件。
- `GetLocalSettings()`：返回进程级本地设置。
- `GetSharedSettings()`：按需同步加载或创建临时共享设置，并缓存对象。
- `LoadSharedSettingsFromDisk()`：登录后尝试异步加载；当前在被调函数处中断。
- `OnSharedSettingsLoaded()`：设计上替换缓存并记录账号 ID；当前不可达。

---

## 网络与权限

- `ULyraSettingsLocal`、`ULyraSettingsShared`、`ULyraLocalPlayer` 和 UI Subsystem
  都不参与 Actor Replication（Actor 复制）。每个进程和本地用户独立维护。
- Dedicated Server（专用服务器）通常没有 LocalPlayer；GameInstance 先检查
  Cast 结果，因此不会为专用服务器用户加载共享设置。
- Listen Server（监听服务器）的主机玩家拥有 LocalPlayer，设置路径会在主机
  进程本地执行。
- LocalPlayer 的 Team 变化可能源自复制到本机的 PlayerState，但 LocalPlayer
  只在本地继续广播，不把数据反向复制到服务器。
- SaveGame 的平台用户索引来自 LocalPlayer。多人分屏时每个 LocalPlayer 应
  使用自己的缓存和用户索引；当前尚未验证。

---

## 资产、配置与模块依赖

| 来源 | 当前配置 / 代码 | 作用与边界 |
|---|---|---|
| `Config/DefaultEngine.ini` | `LocalPlayerClassName=/Script/LyraGame.LyraLocalPlayer` | 让引擎创建当前项目的 LocalPlayer |
| `Config/DefaultEngine.ini` | `GameUserSettingsClassName=/Script/LyraGame.LyraSettingsLocal` | 让 `GEngine` 返回项目本地设置类型 |
| `Config/DefaultGame.ini` | `DefaultUIPolicyClass=/Game/UI/B_LyraUIPolicy...` | 决定 CommonUI 根布局策略；资产内容待验证 |
| C++ 常量 | `SharedGameSettings` | 当前共享设置 Slot 名，不由 Config 覆盖 |
| `LyraGame.Build.cs` | `CommonGame`、`CommonUser`、`GameSettings`、`AudioMixer`、`EnhancedInput` | 模块依赖存在；当前 Shared Settings 尚未消费 Enhanced Input |
| `ULyraSettingsShared` | `UCLASS()`，不是 Primary Asset | 通过 SaveGame 系统发现和序列化，不走 AssetManager 扫描 |

> 🧪 **待验证：** 二进制 UI Policy、设置界面和蓝图可能调用
> `GetSharedSettings()` 或广播设置事件；当前只确认文本 C++ 中没有活动调用者。

---

## 与 Lyra 的差异

### 差异：异步登录加载尚未进入引擎 SaveGame 流程

**当前项目：** GameInstance 与 LocalPlayer 的入口已存在，但
`ULyraSettingsShared::AsyncLoadOrCreateSettings()` 直接返回 `false`。

**Lyra：** 创建 `FOnLocalPlayerSaveGameLoadedNative` 回调，调用引擎
`AsyncLoadOrCreateSaveGameForLocalPlayer()`；完成后先 `ApplySettings()`，再把
对象传给 LocalPlayer。

**影响：** 登录成功不会加载或替换真实用户设置，`NetIdForSharedSettings`
也不会更新。

**状态：** **结构占位**

### 差异：Shared Settings 只有对象外壳

**当前项目：** 可以同步创建/加载 SaveGame 对象，但没有实际设置字段、应用、
脏标记或项目级保存编排。

**Lyra：** 保存力反馈、死区、字幕、背景音频、语言、鼠标/手柄灵敏度等字段，
并把部分输入设置委托给 Enhanced Input 用户设置。

**影响：** 当前同步读取成功也不会产生 Lyra 的用户偏好效果。

**状态：** **部分复刻**

### 差异：Experience 通知已接通但接收端为空

**当前项目：** Experience 在所有 Loaded 委托后调用
`ULyraSettingsLocal::OnExperienceLoaded()`，但函数为空。

**Lyra：** 重新应用可能受 DeviceProfile 改变影响的非分辨率设置。

**影响：** Experience 切换 DeviceProfile 后，当前设置不会从该入口重新应用。

**状态：** **结构占位**

### 差异：Controller 仍未消费共享设置

**当前项目：** `ALyraPlayerController::SetPlayer()` 中的委托绑定与首次
`OnSettingsChanged()` 调用仍被注释。

**Lyra：** Controller 监听共享设置变化并更新 `bForceFeedbackEnabled`。

**影响：** 当前力反馈过滤只使用 Controller 自身状态和 CVar，不响应用户
共享设置。

**状态：** **未复刻**

---

## 已识别的 TODO

| 优先级 | 类型 | 内容 | 依据 | 前置依赖 |
|---|---|---|---|---|
| 高 | 当前源码 TODO | 实现 `AsyncLoadOrCreateSettings()` 并保证成功/失败都进入明确回调 | 函数注释为临时占位且直接返回 false | `ULocalPlayerSaveGame` 异步 API |
| 高 | Lyra 对比 TODO | 为 Shared Settings 增加实际字段、`ApplySettings()`、版本和保存策略 | 当前只有加载外壳 | 先确定项目需要的用户偏好范围 |
| 中 | Lyra 对比 TODO | 恢复 Controller 的设置委托和首次应用 | `SetPlayer()` 注释代码 | 至少先实现力反馈字段和变更事件 |
| 中 | Lyra 对比 TODO | 实现 Local Settings 的 Experience 重应用 | 当前回调为空 | DeviceProfile / 性能设置实现 |
| 中 | Lyra 对比 TODO | 增加音频设备 Setter / 广播并处理切换失败 | 当前只有监听者，失败分支为空 | 设置 UI 与 Audio Mixer 测试设备 |
| 中 | Lyra 对比 TODO | 修正 `ULyraSettingsLocall` Event Owner 拼写 | 当前宏参数与真实类名不一致 | 无 |
| 中 | 验证 TODO | 验证登录失败、账号切换、重复登录和强制重载 | 缓存按 Net ID 去重 | CommonUser 测试账号 |
| 中 | 验证 TODO | 验证分屏玩家使用独立 Slot 用户索引和缓存 | 每个 LocalPlayer 各持有 SharedSettings | 两个本地用户 / 手柄 |
| 低 | 文档 TODO | 打开设置 UI 资产确认是否存在蓝图调用者 | C++ 搜索不足以确认二进制资产 | Unreal Editor 资产审计 |

---

## 快速回顾

- **一句话职责：** UI 层负责本地显示；设置层把机器设置和每用户设置分开。
- **核心入口：** `HandlerUserInitialized()`、`GetSharedSettings()`、
  `OnExperienceFullLoadCompleted()`、`PostInitProperties()`。
- **核心状态：** Engine 持有 `ULyraSettingsLocal`；LocalPlayer 强引用
  `ULyraSettingsShared`。
- **网络位置：** 全部为本地对象，不直接复制；Dedicated Server 通常无
  LocalPlayer。
- **当前完成度：** UI 主链**已复刻**；设置链整体**部分复刻**。
- **最重要的未完成项：** 异步加载 → Apply → 缓存替换 → 消费者更新闭环。

## 复习要点

1. 为什么 `ULyraSettingsLocal` 和 `ULyraSettingsShared` 不能合并为一个对象？
2. 登录成功后当前调用链在哪个函数停止？
3. `ULocalPlayerSaveGame` 如何关联 Slot、平台用户和 LocalPlayer？
4. `SharedSettings` 为什么必须由 `UPROPERTY` 强引用持有？
5. Experience 的设置回调发生在三组 Loaded 委托之前还是之后？
6. 音频设备链当前缺少生产者、消费者还是失败处理？
7. Dedicated Server 与 Listen Server 的 LocalPlayer 设置路径有何不同？
8. 为什么“同步加载对象成功”仍不等于“用户设置已应用”？

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md)
  — GameInstance 在 CommonUser 成功回调中发起共享设置加载。
- [05-Player-Framework.md](05-Player-Framework.md)
  — LocalPlayer 同时转发 Controller 队伍状态并缓存设置对象。
- [07-Experience-Framework.md](07-Experience-Framework.md)
  — Experience 完成后调用本地设置重应用入口。
- [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md)
  — 汇总登录、SaveGame、Team 和 Experience 的端到端调用链。
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md)
  — 按来源汇总异步加载、应用、音频和 Controller 消费缺口。
- [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md)
  — 查看 `PostInitProperties()`、`SwitchController()`、`SetPlayer()` 和
  `ULocalPlayerSaveGame` 的引擎调用时机。
