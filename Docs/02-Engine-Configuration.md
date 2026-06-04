# 02 - 引擎配置

> `Config/DefaultEngine.ini` 中 Lyra 对引擎默认类的替换映射，以及项目级配置决策。

---

## 引擎类替换 (`[/Script/Engine.Engine]`)

Lyra 通过 `DefaultEngine.ini` 替换了几乎所有核心引擎类，这是理解 Lyra 架构的起点：

| 引擎类 | Lyra 替换 | 类型 | 作用 |
|-------|----------|------|------|
| `GameEngine` | `/Script/LyraGame.LyraGameEngine` | [Runtime] | 自定义游戏引擎初始化入口 |
| `UnrealEdEngine` | `/Script/LyraEditor.LyraEditorEngine` | [Editor-Only] | 自定义编辑器引擎（PIE 前置钩子） |
| `EditorEngine` | `/Script/LyraEditor.LyraEditorEngine` | [Editor-Only] | 同上，编辑器引擎别名 |
| `WorldSettingsClassName` | `/Script/LyraGame.LyraWorldSettings` | [Runtime] | 自定义世界设置（每关卡配置 DefaultGameplayExperience） |
| `AssetManagerClassName` | `/Script/LyraGame.LyraAssetManager` | [Runtime] | 自定义资产管理器（启动作业、类型化加载） |
| `LocalPlayerClassName` | `/Script/LyraGame.LyraLocalPlayer` | [Runtime] | 自定义本地玩家（CommonUser 集成） |
| `GameViewportClientClassName` | `/Script/LyraGame.LyraGameViewportClient` | [Runtime] | 自定义视口客户端 |
| `GameUserSettingsClassName` | `/Script/LyraGame.LyraSettingsLocal` | [Runtime] | 自定义用户游戏设置 |

---

## 地图设置 (`[/Script/EngineSettings.GameMapsSettings]`)

| 设置 | 值 | 说明 |
|------|-----|------|
| `GameDefaultMap` | `/Game/L_Simple.L_Simple` | 默认游戏地图 |
| `EditorStartupMap` | `/Game/L_Simple.L_Simple` | 编辑器启动时加载的地图 |
| `ServerDefaultMap` | `/Game/L_Simple.L_Simple` | 专用服务器默认地图 |
| `GameInstanceClass` | `/Game/B_LyraGameInstance.B_LyraGameInstance_C` | **蓝图** GameInstance（基于 `ULyraGameInstance`） |
| `GlobalDefaultGameMode` | `/Game/B_LyraGameMode.B_LyraGameMode_C` | **蓝图** GameMode（基于 `ALyraGameMode`） |

> **注意:** GameInstance 和 GameMode 使用的是蓝图子类而非纯 C++ 类，这意味着可以在编辑器中配置额外的蓝图逻辑。

---

## 渲染设置 (`[/Script/Engine.RendererSettings]`)

| 设置 | 值 | 说明 |
|------|-----|------|
| `r.AllowStaticLighting` | False | 禁用静态光照（使用动态 GI） |
| `r.GenerateMeshDistanceFields` | True | 生成网格距离场 |
| `r.DynamicGlobalIlluminationMethod` | 1 | Lumen 动态全局光照 |
| `r.ReflectionMethod` | 1 | Lumen 反射 |
| `r.SkinCache.CompileShaders` | True | 皮肤缓存着色器 |
| `r.RayTracing` | True | 启用光线追踪 |
| `r.Substrate` | True | 启用 Substrate 材质系统 |
| `r.Shadow.Virtual.Enable` | 1 | 虚拟阴影贴图（Nanite） |

---

## 平台设置

- **Windows:** 目标 RHI 为 DX12，着色器格式为 SM6
- **Linux:** Vulkan SM6
- **Mac:** Metal SM6
- **硬件目标:** Desktop，默认图形性能 Maximum

---

## 碰撞系统 (`[/Script/Engine.CollisionProfile]`)

### 自定义追踪通道（5 个）

| 通道名 | 枚举值 | 用途 |
|--------|-------|------|
| `Lyra_TraceChannel_Interaction` | ECC_GameTraceChannel1 | 交互射线检测 |
| `Lyra_TraceChannel_Weapon` | ECC_GameTraceChannel2 | 武器射线检测 |
| `Lyra_TraceChannel_Weapon_Capsule` | ECC_GameTraceChannel3 | 武器胶囊体检测 |
| `Lyra_TraceChannel_Weapon_Multi` | ECC_GameTraceChannel4 | 武器多重检测 |
| `Lyra_TraceChannel_AimAssist` | ECC_GameTraceChannel5 | 瞄准辅助检测 |

### 自定义碰撞预设（5 个）

| 预设名 | 用途 |
|--------|------|
| `LyraPawnMesh` | 角色网格碰撞（仅与 Weapon_Multi 重叠） |
| `LyraPawnCapsule` | 角色胶囊碰撞（忽略 Camera，与 Weapon 系列交互） |
| `Interactable_OverlapDynamic` | 可交互物重叠检测（PhysicsBody 类型） |
| `Interactable_BlockDynamic` | 可交互物阻挡检测（WorldDynamic 类型） |
| `AimAssist_OverlapDynamic` | 瞄准辅助重叠检测（仅与 AimAssist 通道重叠） |

---

## 核心重定向 (`[CoreRedirects]`)

```ini
+ClassRedirects = (OldName="/Script/LyraGame.ALyraCharacterWithAbilities",
                   NewName="/Script/LyraGame.LyraCharacterWithAbilities")
```

类名从 `ALyraCharacterWithAbilities` 重命名为 `LyraCharacterWithAbilities`（移除了 `A` 前缀）。

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — 被替换的系统类详解
- [04-Game-Framework.md](04-Game-Framework.md) — GameMode、WorldSettings 详解
- [12-Editor-Module.md](12-Editor-Module.md) — ULyraEditorEngine 详解
