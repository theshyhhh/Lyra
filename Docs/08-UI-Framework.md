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

**职责:** 基础 HUD 类。实际 UI 渲染由 Lyra 中的 CommonUI `UActivatableWidget` 系统处理，因此此类相对轻量。

**重写的生命周期函数:**

##### `PreInitializeComponents()`
> ⏱️ **引擎调用时机:** Actor 生成流程中，在 `BeginPlay()` 之前，`PostInitializeComponents()` 之后。组件已创建但尚未 BeginPlay。
>
> **适合写的逻辑:** 在组件 BeginPlay 之前对组件进行最后配置、设置组件间依赖。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 3](17-Engine-Lifecycle-Reference.md#3-aactor-生命周期)

##### `BeginPlay()`
> ⏱️ **引擎调用时机:** Actor 初始化流程最后一步。所有组件的 BeginPlay 都完成后才调用。⚠️ 不保证 Actor 之间的初始化顺序。
>
> **适合写的逻辑:** 游戏逻辑初始化（最常见入口）、获取其他 Actor/组件引用、绑定委托。如需在所有 Actor 初始化完毕后执行逻辑，用定时器延迟一帧或依赖 Experience 加载完成委托。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 3](17-Engine-Lifecycle-Reference.md#3-aactor-生命周期)

##### `EndPlay(const EEndPlayReason::Type EndPlayReason)`
> ⏱️ **引擎调用时机:** Actor 被销毁或关卡卸载时，在组件销毁之前。`EndPlayReason` 区分销毁原因（`Destroyed`/`LevelTransition`/`EndPlayInEditor`/`RemovedFromWorld`/`Quit`）。
>
> **适合写的逻辑:** 清理资源、解绑委托、保存持久化状态。根据 `EndPlayReason` 做不同处理。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 3](17-Engine-Lifecycle-Reference.md#3-aactor-生命周期)

**其他方法:**
- `GetDebugActorList()` — 重写以添加 Lyra 特定的 Actor 到调试信息显示

---

### ULyraGameViewportClient [Runtime]

**继承链:** `UObject → UGameViewportClient → UCommonGameViewportClient → ULyraGameViewportClient`

**UCLASS:** `UCLASS(BlueprintType)`

**职责:** 自定义视口客户端。

**重写的生命周期函数:**

##### `Init(FWorldContext&, UGameInstance*, bool)`
> ⏱️ **引擎调用时机:** 视口客户端被创建时，由 `UGameInstance` 在 World 创建之前调用。每个 World 一个视口客户端。
>
> **适合写的逻辑:** 配置视口显示参数、注册自定义视口渲染回调、设置分辨率/窗口模式策略。⚠️ 必须调用 `Super::Init()`。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 13](17-Engine-Lifecycle-Reference.md#13-ugameviewportclient)

**注册:** `DefaultEngine.ini` 中 `GameViewportClientClassName=/Script/LyraGame.LyraGameViewportClient`

---

### ULyraSettingsLocal [Runtime]

**继承链:** `UObject → UGameUserSettings → ULyraSettingsLocal`

**UCLASS:** `UCLASS()`

**职责:** 每用户游戏设置持久化。扩展 `UGameUserSettings`（分辨率/图形/音频持久化系统）。

**关键方法:**
- `Get()` — 静态访问器，返回 `ULyraSettingsLocal*` 单例

**状态:**
- 当前较轻量 — 完整实现计划扩展
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
