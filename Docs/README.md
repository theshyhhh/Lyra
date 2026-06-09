# Lyra 项目知识库

> UE5 Lyra 示例项目复刻的知识库。按框架子系统组织，帮助在开发时快速查阅类职责、继承关系和架构设计。

---

## 快速导航

### 架构概览

| 文档 | 内容 | 适合场景 |
|------|------|---------|
| [01-Architecture-Overview.md](01-Architecture-Overview.md) | 项目结构、四大架构原则、框架关系全景图 | 新人入门、快速建立心智模型 |
| [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md) | 引擎启动 → PIE → Experience 加载 → 网络复制的完整调用链 | 调试、理解执行顺序 |
| [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) | 所有 TODO、注释掉的接口、空实现 | 规划下一步开发方向 |
| [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md) | 所有引擎重写函数的调用时机、适合写的逻辑、注意事项 | 开发时选择正确生命周期函数 |

### 运行时框架

| 文档 | 包含的类 | 框架职责 |
|------|---------|---------|
| [03-System-Framework.md](03-System-Framework.md) | `ULyraGameEngine`, `ULyraGameInstance`, `ULyraReplaySubsystem`, `ULyraAssetManager`, `ULyraGameData` | 引擎入口、实例管理、资产加载、回放能力判断 |
| [04-Game-Framework.md](04-Game-Framework.md) | `ALyraGameMode`, `ALyraGameState`, `ALyraGameSession`, `ALyraWorldSettings`, `ULyraUserFacingExperienceDefinition`, `ULyraAbilitySystemComponent`, `FLyraVerbMessage`, `ULyraVerbMessageHelpers` | 比赛管理层、前端 Playlist 入口、GameState GAS 与客户端消息桥接 |
| [05-Player-Framework.md](05-Player-Framework.md) | `ULyraLocalPlayer`, `ALyraPlayerController`, `ALyraPlayerState`, `ILyraTeamAgentInterface`, `FGameplayTagStackContainer`, `ALyraPlayerStart`, `ULyraPlayerSpawningManagerComponent` | 玩家表示层、PlayerState ASC/Team/StatTag 与出生/重生管理 |
| [06-Character-Framework.md](06-Character-Framework.md) | `ALyraCharacter`, `ALyraCharacterWithAbilities`, `ULyraPawnExtensionComponent`, `ULyraPawnData` | 角色 Pawn 与 Pawn 生成配置 |
| [07-Experience-Framework.md](07-Experience-Framework.md) | `ULyraExperienceDefinition`, `ULyraUserFacingExperienceDefinition`, `ULyraExperienceManagerComponent`, `ULyraExperienceManager`, `UAsyncAction_ExperienceReady` | **核心架构 — Experience 加载状态机与 Ready 异步节点** |
| [08-UI-Framework.md](08-UI-Framework.md) | `ALyraHUD`, `ULyraGameViewportClient`, `ULyraUIManagerSubsystem`, `ULyraSettingsLocal` | 显示层、UI Policy 管理与设置 |

### 编辑器与开发工具

| 文档 | 包含的类 | 适用环境 |
|------|---------|---------|
| [11-Development-Tools.md](11-Development-Tools.md) | `ULyraDeveloperSettings`, `ULyraPlatformEmulationSettings` | [Editor-Dev] PIE 调试与平台模拟 |
| [12-Editor-Module.md](12-Editor-Module.md) | `ULyraEditorEngine`, `FLyraEditorModule` | [Editor-Only] 编辑器引擎与 PIE 钩子 |

### 速查参考

| 文档 | 内容 |
|------|------|
| [02-Engine-Configuration.md](02-Engine-Configuration.md) | DefaultEngine.ini / DefaultGame.ini 中的引擎类替换、AssetManager 扫描、GameData/ActionSet、UI Policy 与加载屏幕配置 |
| [09-GameplayTags-System.md](09-GameplayTags-System.md) | 全部 ~42 个原生 GameplayTag 声明、分类与 GameplayTag Stack 容器 |
| [10-Logging-System.md](10-Logging-System.md) | 9 个全局日志通道 + 1 个文件内静态日志分类速查 |
| [13-Plugins-Catalog.md](13-Plugins-Catalog.md) | 12 个插件的角色、依赖、模块类型 |
| [14-Inheritance-Chains.md](14-Inheritance-Chains.md) | 全部类的完整继承链速查 |

---

## 快速查阅指南

### 我想知道某个引擎生命周期函数何时调用、适合写什么

→ 查看 [17-Engine-Lifecycle-Reference.md](17-Engine-Lifecycle-Reference.md)（覆盖 30+ 个函数，按类分组，末尾有按开发场景的快速索引）

### 我想知道某个类属于哪个框架

→ 查看 [14-Inheritance-Chains.md](14-Inheritance-Chains.md) 按框架分类的继承链

### 我想知道 Experience 系统是怎么工作的

→ 查看 [07-Experience-Framework.md](07-Experience-Framework.md)（最核心的文档）

### 我想知道某个引擎默认类被替换成了什么

→ 查看 [02-Engine-Configuration.md](02-Engine-Configuration.md)

### 我想知道哪些功能还没做

→ 查看 [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md)

### 我想知道从引擎启动到游戏开始发生了什么

→ 查看 [15-Data-Flow-and-Lifecycle.md](15-Data-Flow-and-Lifecycle.md)

---

## 标记约定

| 标记 | 含义 |
|------|------|
| `[Runtime]` | 在所有构建目标中编译，Shipping 构建中可用 |
| `[Editor-Only]` | 仅存在于 LyraEditor 模块，运行时构建中不存在 |
| `[Editor-Dev]` | 存在于 LyraGame/Development/，在所有配置中编译但主要服务于编辑器/PIE |
| `[Stub/TODO]` | 类或函数结构存在但无实际实现，或有被注释掉的代码 |
| 🧩 | 继承 Modular 基类，支持 GameFeature 插件动态注入组件 |

---

## 知识库统计

| 指标 | 数量 |
|------|------|
| 框架文档 | 6 个（System / Game / Player / Character / Experience / UI） |
| 编辑器/工具文档 | 2 个（Development Tools / Editor Module） |
| 速查参考文档 | 4 个（Engine Config / GameplayTags / Logging / Plugins） |
| 综合文档 | 4 个（Architecture Overview / Data Flow / Stubs / Engine Lifecycle） |
| UCLASS / UINTERFACE 类 | 35 个 |
| 非 UObject 类型 | 16 个（枚举、结构体、命名空间、委托、C++ 接口体） |
| 插件 | 12 个（11 Runtime + 1 Editor） |
| GameplayTag | ~42 个 |
| 日志通道 | 9 个全局 + 1 个文件内静态 |
