# 08 - UI 框架

> HUD、视口客户端和用户设置。Lyra 的 UI 层通过 CommonUI 插件基类集成。

---

## 框架概述

UI 框架涵盖游戏内 UI 的显示层（HUD）、渲染视口配置（GameViewportClient）和本地用户设置持久化（SettingsLocal）。

**设计意图:**
- 视口和设置使用 CommonUI 插件基类以获得增强功能
- HUD 保持简单，实际 UI 小部件通过 CommonUI 的 ActivatableWidget 系统和 UIExtension 插件管理
- 设置系统通过 GameSettings 插件框架扩展

---

## 类列表

| 类 | 父类 | 生命周期 | 职责 |
|-----|------|---------|------|
| `ALyraHUD` | `AHUD` | [Runtime] | 基础 HUD（调试 Actor 列表） |
| `ULyraGameViewportClient` | `UCommonGameViewportClient` | [Runtime] | 自定义视口初始化 |
| `ULyraSettingsLocal` | `UGameUserSettings` | [Runtime] | 每用户游戏设置持久化 |

---

## 逐类详解

### ALyraHUD [Runtime]

**继承链:** `AActor → AHUD → ALyraHUD`

**UCLASS:** `UCLASS(Config = Game)`

**职责:** 基础 HUD 类。实际 UI 渲染由 CommonUI `UActivatableWidget` 系统处理。此类的主要职责是将其自身注册到 `UGameFrameworkComponentManager`，使其成为 ModularGameplay 框架的一部分——GameFeature Action 可以在 HUD 上动态添加组件（如 UI 扩展）。

**构造函数:** `PrimaryActorTick.bStartWithTickEnabled = false;` — 默认禁用 Tick，HUD 不需要每帧 Tick。

**重写的生命周期函数:**

##### `PreInitializeComponents()`
> ⏱️ **引擎调用时机:** Actor 生成流程中，在 `BeginPlay()` 之前，`PostInitializeComponents()` 之后。组件已创建但尚未 BeginPlay。
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 3](17-Engine-Lifecycle-Reference.md#3-aactor-生命周期)

**当前行为:** 调用 `UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this)` — 将 HUD 注册为 GameFramework 组件接收者。这使得 GameFeature Action 可以像对其他 Modular Actor 一样，在 HUD 上动态添加/移除组件。这是 Lyra 中很重要的架构设计：**HUD 本身也是一个 GameFeature 扩展点**。

##### `BeginPlay()`
> ⏱️ **引擎调用时机:** Actor 初始化流程最后一步。所有组件的 BeginPlay 都完成后才调用。
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 3](17-Engine-Lifecycle-Reference.md#3-aactor-生命周期)

**当前行为:** 在调用 `Super::BeginPlay()` **之前**，先发送 `UGameFrameworkComponentManager::NAME_GameActorReady` 扩展事件。此事件向 GameFrameworkComponentManager 发出信号："这个 Actor 的所有基本组件已经就绪"，触发等待此信号的组件初始化。

##### `EndPlay(const EEndPlayReason::Type EndPlayReason)`
> ⏱️ **引擎调用时机:** Actor 被销毁或关卡卸载时，在组件销毁之前。
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 3](17-Engine-Lifecycle-Reference.md#3-aactor-生命周期)

**当前行为:** 调用 `UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this)` 从 ComponentManager 注销，然后调用 `Super::EndPlay()`。

**其他方法:**
- `GetDebugActorList()` — 遍历全局所有 `UAbilitySystemComponent` 实例，将它们的 Avatar Actor / Owner Actor 添加到调试列表中。这使得在 HUD 调试信息中可以查看到所有活跃的 GAS 组件。

---

### ULyraGameViewportClient [Runtime]

**继承链:** `UObject → UGameViewportClient → UCommonGameViewportClient → ULyraGameViewportClient`

**UCLASS:** `UCLASS(BlueprintType)`

**职责:** 自定义视口客户端。

**重写的生命周期函数:**

##### `Init(FWorldContext&, UGameInstance*, bool)`
> ⏱️ **引擎调用时机:** 视口客户端被创建时，由 `UGameInstance` 在 World 创建之前调用。
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 13](17-Engine-Lifecycle-Reference.md#13-ugameviewportclient)

**当前行为:**
1. 调用 `Super::Init()` 完成标准初始化
2. 通过 `ICommonUIModule::GetSettings().GetPlatformTraits()` 检查当前平台是否拥有 `Platform.Trait.Input.HardwareCursor` 标签
3. 根据平台特征决定光标模式：桌面端有硬件光标 → 不显示软件光标；主机/移动端无硬件光标 → 显示软件光标部件

**新定义的 GameplayTag:**
```cpp
namespace GameViewportTags {
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_Input_HardwareCursor,
                                   "Platform.Trait.Input.HardwareCursor");
}
```

**注册:** `DefaultEngine.ini` 中 `GameViewportClientClassName=/Script/LyraGame.LyraGameViewportClient`

---

### ULyraSettingsLocal [Runtime]

**继承链:** `UObject → UGameUserSettings → ULyraSettingsLocal`

**UCLASS:** `UCLASS()`

**职责:** 每用户游戏设置持久化。扩展 `UGameUserSettings`（分辨率/图形/音频持久化系统）。

**关键方法:**
- `Get()` — 静态访问器，通过 `CastChecked<ULyraSettingsLocal>(GEngine->GetGameUserSettings())` 返回单例

**Editor CVars（定义于 .cpp 中，由 ULyraPlatformEmulationSettings 绑定）:**
- `Lyra.Settings.ApplyFrameRateSettingsInPIE` — 是否在 PIE 中应用帧率限制
- `Lyra.Settings.ApplyFrontEndPerformanceOptionsInPIE` — 是否在 PIE 中应用前端性能设置
- `Lyra.Settings.ApplyDeviceProfilesInPIE` — 是否在 PIE 中应用设备配置

**状态:**
- `OnExperienceLoaded()` 调用在 `ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()` 中被注释掉：Experience 加载时应用可扩展性/质量设置的计划尚未激活

**注册:** `DefaultEngine.ini` 中 `GameUserSettingsClassName=/Script/LyraGame.LyraSettingsLocal`

---

## 框架内部关系

```
ULyraGameViewportClient::Init()
  └── 视口初始化（项目级自定义）

ALyraPlayerController
  └── MyHUD → ALyraHUD

ULyraGameInstance::HandlerUserInitialized()
  └── [注释掉] ULyraSettingsLocal::LoadSharedSettingsFromDisk()

ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()
  └── [注释掉] ULyraSettingsLocal::OnExperienceLoaded()
```

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 在用户初始化时触发设置加载
- [05-Player-Framework.md](05-Player-Framework.md) — PlayerController 持有 HUD 引用
- [07-Experience-Framework.md](07-Experience-Framework.md) — Experience 加载完毕后计划应用设置
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — 设置加载在 Experience 加载完毕时被注释掉
