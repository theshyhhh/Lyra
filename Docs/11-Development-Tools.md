# 11 - 开发工具

> 编辑器开发设置类，位于 `Source/LyraGame/Development/`。它们在所有构建配置中编译，但主要服务于编辑器/PIE 工作流。标记为 `[Editor-Dev]`。

---

## 框架概述

Development 目录下的类为开发者提供编辑器和 PIE 环境中的调试、测试和平台模拟能力。它们通过 `UDeveloperSettingsBackedByCVars` 基类将编辑器设置面板中的配置与运行时 CVar 绑定，使得在编辑器中修改的设置也能通过控制台变量在命令行中访问。

**设计意图:**
- 加速 PIE 迭代（跳过非必要内容加载、自动执行作弊命令）
- 在 PC 上模拟主机/移动端平台行为
- 通过开发者设置面板集中管理调试选项

---

## 类列表

| 类/类型 | 父类 | 生命周期 | 职责 |
|---------|------|---------|------|
| `ULyraDeveloperSettings` | `UDeveloperSettingsBackedByCVars` | [Editor-Dev] | 开发者调试设置（经验覆盖、BOT、作弊、地图） |
| `ULyraPlatformEmulationSettings` | `UDeveloperSettingsBackedByCVars` | [Editor-Dev] | PIE 平台模拟（平台特征、设备配置） |
| `ECheatExecutionTime` | (UENUM) | [Editor-Dev] | 作弊执行时机枚举 |
| `FLyraCheatToRun` | (USTRUCT) | [Editor-Dev] | 作弊配置结构体 |

---

## 逐类详解

### ULyraDeveloperSettings [Editor-Dev]

**继承链:** `UObject → UDeveloperSettings → UDeveloperSettingsBackedByCVars → ULyraDeveloperSettings`

**UCLASS:** `UCLASS(Config=EditorPerProjectUserSettings, MinimalAPI)`

**配置:** `Config=EditorPerProjectUserSettings` 意味着每个开发者拥有自己的独立配置，不会被提交到版本控制。

**编辑器分类:** 出现在 Project Settings → Lyra 面板下。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `ExperienceOverride` | `FPrimaryAssetId` | PIE 时覆盖使用哪个 Experience（未设置则使用 WorldSettings 中的默认） |
| `bOverrideBotCount` | `bool` | 是否覆盖机器人数量的切换开关 |
| `OverrideNumPlayerBotsToSpawn` | `int32` | 覆盖的机器人数量（仅当 bOverrideBotCount 为 true 时生效） |
| `bAllowPlayerBotsToAttack` | `bool` | 是否允许机器人攻击 |
| `bTestFullGameFlowInPIE` | `bool` | 是否执行完整游戏流程（跳过等待玩家等阶段） |
| `bShouldAlwaysPlayForceFeedback` | `bool` | 是否始终播放力反馈（CVar: `LyraPC.ShouldAlwaysPlayForceFeedback`） |
| `bSkipLoadingCosmeticBackgroundsInPIE` | `bool` | 是否跳过装饰性背景加载以加速 PIE 迭代 |
| `CheatsToRun` | `TArray<FLyraCheatToRun>` | PIE 启动时自动执行的作弊命令列表 |
| `LogGameplayMessages` | `bool` | 是否记录 GameplayMessageSubsystem 的所有消息广播（CVar: `GameplayMessageSubsystem.LogMessages`） |
| `CommonEditorMaps` | `TArray<FSoftObjectPath>` | [Editor-Only] 编辑器工具栏可快速访问的常用地图列表 |

**编辑器方法 (WITH_EDITOR):**
- `OnPlayInEditorStarted()`: 由 `ULyraEditorEngine::PreCreatePIEInstances()` 调用。应用开发者设置并弹出提醒通知（如果作弊/调试功能处于激活状态）。
- `ApplySettings()`: 预留的设置应用钩子。
**重写的 UObject 生命周期:**

##### `PostInitProperties()`
> ⏱️ **引擎调用时机:** 对象构造完成、属性从 CDO 或 Config 文件加载后。在所有属性初始化完成之后。⚠️ 必须调用 `Super::PostInitProperties()`。
>
> **适合写的逻辑:** 基于已加载的属性做二次初始化、验证配置一致性。与构造函数不同——此时所有 `Config`/`EditDefaultsOnly` 属性已加载。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 6](17-Engine-Lifecycle-Reference.md#6-uobject-生命周期)

[Editor-Only] 调用 `ApplySettings()` 应用开发者设置 + 弹出作弊状态提醒。

##### `PostEditChangeProperty(FPropertyChangedEvent&)`
> ⏱️ **引擎调用时机:** 用户在编辑器 Details 面板修改属性后。`PropertyChangedEvent` 包含哪个属性被修改、修改类型。仅在编辑器触发，PIE 中修改不触发。
>
> **适合写的逻辑:** 属性联动（改 A 自动调 B）、实时验证用户输入、预览效果。通过 `PropertyChangedEvent.MemberProperty` 判断变更的属性。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 6](17-Engine-Lifecycle-Reference.md#6-uobject-生命周期)

[Editor-Only] 调用 `ApplySettings()` 应用变更。

##### `PostReloadConfig(FProperty*)`
> ⏱️ **引擎调用时机:** 配置文件被热重载后（如修改 .ini 后编辑器自动重载，或调用 `ReloadConfig()`）。多个属性变更时，每个都触发一次。
>
> **适合写的逻辑:** 在 Config 热重载后重新应用设置、响应外部配置变更。
>
> 📖 [详见 17-Engine-Lifecycle-Reference.md § 6](17-Engine-Lifecycle-Reference.md#6-uobject-生命周期)

[Editor-Only] 调用 `ApplySettings()`。

---

### ECheatExecutionTime [Editor-Dev]

**UENUM:** `UENUM()`

| 值 | 说明 |
|-----|------|
| `OnCheatManagerCreated` | 当作弊管理器被创建时执行 |
| `OnPlayerPawnPossession` | 当玩家 Pawn 被控制时执行 |

---

### FLyraCheatToRun [Editor-Dev]

**USTRUCT:** `USTRUCT()`

| 属性 | 类型 | 默认值 | 说明 |
|------|------|-------|------|
| `Phase` | `ECheatExecutionTime` | `OnPlayerPawnPossession` | 何时执行此作弊 |
| `Cheat` | `FString` | — | 要执行的控制台命令字符串 |

---

### ULyraPlatformEmulationSettings [Editor-Dev]

**继承链:** `UObject → UDeveloperSettings → UDeveloperSettingsBackedByCVars → ULyraPlatformEmulationSettings`

**UCLASS:** `UCLASS()`

**职责:** 在开发 PC 上模拟不同平台（Xbox、PS5、移动端）的行为，用于测试 UI 可见性、性能选项和帧率策略，无需目标硬件。

**属性:**

| 属性 | 类型 | 说明 |
|------|------|------|
| `AdditionalPlatformTraitsToEnable` | `FGameplayTagContainer` | 在 PIE 中额外启用的平台特征 |
| `AdditionalPlatformTraitsToSuppress` | `FGameplayTagContainer` | 在 PIE 中额外屏蔽的平台特征 |
| `PretendPlatform` | `FName` | 模拟的平台名称 |
| `PretendBaseDeviceProfile` | `FName` | 模拟的基础设备配置文件 |
| `bApplyFrameRateSettingsInPIE` | `bool` | 是否在 PIE 中应用帧率限制（CVar: `Lyra.Settings.ApplyFrameRateSettingsInPIE`） |
| `bApplyFrontEndPerformanceOptionsInPIE` | `bool` | 是否在 PIE 中应用前端性能选项（CVar: `Lyra.Settings.ApplyFrontEndPerformanceOptionsInPIE`） |
| `bApplyDeviceProfilesInPIE` | `bool` | 是否在 PIE 中应用模拟平台的设备配置（CVar: `Lyra.Settings.ApplyDeviceProfilesInPIE`） |

**注意事项（来自源文件注释）:**
- `bApplyFrameRateSettingsInPIE`: 帧率限制是引擎范围的，可能与其他 PIE 窗口冲突。建议同时禁用编辑器偏好中的 "Use Less CPU when in Background"。
- `bApplyFrontEndPerformanceOptionsInPIE` (默认 false): 前端性能选项是全局的。若一个 PIE 窗口在前端界面而另一个在游戏内界面，会互相覆盖。默认禁用以避免冲突。

**编辑器方法 (WITH_EDITOR):**
- `OnPlayInEditorStarted()`: 由 `ULyraEditorEngine::PreCreatePIEInstances()` 调用，弹出配置变更提醒。
- `ApplySettings()` / `ChangeActivePretendPlatform()`: 应用平台模拟设置。

---

## 触发流程

```
PIE 启动
  └── ULyraEditorEngine::PreCreatePIEInstances()
        ├── ULyraDeveloperSettings::OnPlayInEditorStarted()
        │     ├── ApplySettings()
        │     ├── 弹出作弊/调试功能激活提醒
        │     └── 检查 ExperienceOverride
        └── ULyraPlatformEmulationSettings::OnPlayInEditorStarted()
              └── ApplySettings() → ChangeActivePretendPlatform()
```

---

## 关联框架

- [12-Editor-Module.md](12-Editor-Module.md) — ULyraEditorEngine 在 PreCreatePIEInstances 中调用这些设置
- [07-Experience-Framework.md](07-Experience-Framework.md) — ExperienceOverride 影响加载哪个 Experience
