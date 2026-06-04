# 12 - Editor 模块

> `LyraEditor` 模块包含所有纯编辑器代码。标记为 `[Editor-Only]`。此模块仅在编辑器构建目标中编译，运行时构建中不存在。

---

## 框架概述

LyraEditor 模块负责：
1. 替换默认的 `UUnrealEdEngine`，在 PIE 生命周期中注入自定义逻辑
2. 在模块启动时绑定 PIE 开始/结束委托
3. 协调编辑器内的 GameFeature 插件引用计数管理

**设计意图:**
- 将编辑器特定行为与运行时代码解耦
- 通过 `ULyraEditorEngine` 在 PIE 实例创建前插入自定义前置处理
- 确保多 PIE 会话的 GameFeature 插件管理安全

---

## 模块结构

| 文件 | 内容 | 说明 |
|------|------|------|
| `LyraEditor.h` | `DECLARE_LOG_CATEGORY_EXTERN(LogLyraEditor)` | 编辑器日志通道 |
| `LyraEditor.cpp` | `FLyraEditorModule` | 模块启动/关闭 + PIE 委托绑定 |
| `LyraEditorEngine.h` | `ULyraEditorEngine` | 自定义编辑器引擎 |
| `LyraEditorEngine.cpp` | 实现 | Init/Start/Tick/PreCreatePIEInstances/FirstTickSetup |

**模块依赖 (LyraEditor.Build.cs):**
- 公共依赖: Core, CoreUObject, Engine, EditorFramework, UnrealEd, GameplayTagsEditor, GameplayTasksEditor, GameplayAbilities, GameplayAbilitiesEditor, StudioTelemetry, **LyraGame**
- 私有依赖: InputCore, Slate, SlateCore, ToolMenus, EditorStyle, DataValidation, MessageLog, Projects, DeveloperToolSettings, CollectionManager, SourceControl, Chaos

> ⚠️ **LyraEditor 依赖 LyraGame** — 编辑器模块可以引用所有 LyraGame 类。例如 `ULyraEditorEngine` 引用了 `ALyraWorldSettings`、`ULyraDeveloperSettings`、`ULyraPlatformEmulationSettings`。

---

## 逐类详解

### FLyraEditorModule [Editor-Only]

**类型:** `FDefaultGameModuleImpl`

**职责:** 编辑器模块。管理 PIE 委托和 Experience 管理器协调。

**重写的生命周期函数:**

##### `StartupModule()`
> ⏱️ **引擎调用时机:** 模块被引擎加载时。取决于模块的 `LoadingPhase`——`Default` 阶段在引擎核心初始化后。⚠️ 不要在 Default 阶段访问 Slate/UI 系统。
>
> **适合写的逻辑:** 注册委托（PIE 开始/结束）、注册编辑器扩展（工具栏按钮、菜单项）、初始化模块级单例。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 10](17-Engine-Lifecycle-Reference.md#10-模块加载生命周期)

当不作为游戏运行时（`!IsRunningGame()`），绑定 `BeginPIE` 和 `EndPIE` 委托到 `ULyraExperienceManager::OnPlayInEditorBegun()`，在 PIE 开始时重置 GameFeature 插件引用计数表。

##### `ShutdownModule()`
> ⏱️ **引擎调用时机:** 引擎关闭、模块卸载前。此时某些系统可能已部分销毁。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 10](17-Engine-Lifecycle-Reference.md#10-模块加载生命周期)

当前为空实现。

---

### ULyraEditorEngine [Editor-Only]

**继承链:** `UObject → UEngine → UEditorEngine → UUnrealEdEngine → ULyraEditorEngine`

**UCLASS:** `UCLASS()`

**注册:** `DefaultEngine.ini` 中：
```ini
UnrealEdEngine = /Script/LyraEditor.LyraEditorEngine
EditorEngine = /Script/LyraEditor.LyraEditorEngine
```

**职责:** 自定义编辑器引擎。在 PIE 生命周期中插入 Lyra 特定的前置处理逻辑。

**重写的生命周期函数:**

##### `Init(IEngineLoop*)`
> ⏱️ **引擎调用时机:** 引擎启动最早阶段。所有模块加载完成、UObject 系统初始化后，任何 World 创建之前。仅调用一次。
>
> **适合写的逻辑:** 项目级编辑器初始化。⚠️ 此时不能访问 World 或 Slate。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 1](17-Engine-Lifecycle-Reference.md#1-引擎与编辑器生命周期)

调用 Super，当前无额外逻辑。

##### `Start()`
> ⏱️ **引擎调用时机:** `Init()` 之后，引擎主循环开始前。此时 Slate/编辑器 UI 已就绪。
>
> **适合写的逻辑:** 编辑器 UI 注册（菜单扩展、工具栏按钮）、启动时 UI 初始化。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 1](17-Engine-Lifecycle-Reference.md#1-引擎与编辑器生命周期)

调用 Super 后输出日志。

##### `Tick(float DeltaSeconds, bool bIdleMode)`
> ⏱️ **引擎调用时机:** 编辑器主循环中每帧。`bIdleMode` 表示编辑器是否空闲（无焦点/无 PIE）。⚠️ 只能用于编辑器全局行为，不要放耗时操作。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 1](17-Engine-Lifecycle-Reference.md#1-引擎与编辑器生命周期)

每帧调用 `FirstTickSetup()`（通过布尔标志保证仅第一帧执行一次）：强制 Content Browser 显示插件文件夹。

##### `PreCreatePIEInstances(bool, bool, float, bool, int32&)`
> ⏱️ **引擎调用时机:** 用户点击 PIE 后，PIE 实例创建**之前**。此时还没有 PIE World。这是编辑器开发中最常用的钩子之一。
>
> **适合写的逻辑:** 修改 PIE 网络模式（强制 Standalone）、通知其他系统 PIE 即将开始、应用开发设置和平台模拟、通过返回值阻止 PIE。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 1](17-Engine-Lifecycle-Reference.md#1-引擎与编辑器生命周期)

执行三个步骤：
1. 检查 `ALyraWorldSettings::ForceStandaloneNetMode` → 强制 PIE 为 Standalone 模式
2. 调用 `ULyraDeveloperSettings::OnPlayInEditorStarted()`
3. 调用 `ULyraPlatformEmulationSettings::OnPlayInEditorStarted()`

**其他方法:**

| 方法 | 说明 |
|------|------|
| `FirstTickSetup()` | 强制 Content Browser 显示插件文件夹（引擎第一帧时执行，“doesn't really work due to engine iteration”） |

---

## 与 LyraGame 模块的关系

```
LyraEditor 模块 (编辑器环境)
        |
        | 依赖并引用
        v
LyraGame 模块 (运行时 + 编辑器)
  ├── ULyraDeveloperSettings
  ├── ULyraPlatformEmulationSettings
  ├── ALyraWorldSettings (ForceStandaloneNetMode)
  └── ULyraExperienceManager (OnPlayInEditorBegun)
```

**启动流程整合:**
```
编辑器启动
  └── ULyraEditorEngine::Init() / Start() / Tick()
        └── FirstTickSetup() (第一帧)

PIE 启动前 (PreCreatePIEInstances):
  ├── ForceStandaloneNetMode 检查
  ├── DeveloperSettings 应用
  └── PlatformEmulationSettings 应用

PIE 开始时 (FLyraEditorModule):
  └── ExperienceManager::OnPlayInEditorBegun()
```

---

## 关联框架

- [02-Engine-Configuration.md](02-Engine-Configuration.md) — DefaultEngine.ini 中注册 ULyraEditorEngine
- [07-Experience-Framework.md](07-Experience-Framework.md) — ULyraExperienceManager 由 FLyraEditorModule 驱动
- [11-Development-Tools.md](11-Development-Tools.md) — DeveloperSettings 和 PlatformEmulationSettings 由 ULyraEditorEngine 调用
