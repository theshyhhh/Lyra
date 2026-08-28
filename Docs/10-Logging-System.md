# 10 - 日志系统

> 所有 Lyra 定义的日志通道速查表。

> **核对基线：** 当前项目最新提交 `19e6961`，文档本次复核日期为
> 2026-07-13。本文只确认 C++ 日志分类的声明、定义和当前调用位置，不根据
> 日志名称推断对应功能已经实现。

---

## 日志通道总表

| 日志分类 | 源文件 | 默认详细级别 | 编译时级别 | 用途 |
|---------|--------|-------------|-----------|------|
| `LogLyra` | `Source/LyraGame/LyraLogChannels.h` | Log | All | 通用 Lyra 日志 |
| `LogLyraExperience` | `Source/LyraGame/LyraLogChannels.h` | Log | All | Experience 加载/卸载相关 |
| `LogLyraAbilitySystem` | `Source/LyraGame/LyraLogChannels.h` | Log | All | GAS (Gameplay Ability System) 相关 |
| `LogLyraTeams` | `Source/LyraGame/LyraLogChannels.h` | Log | All | 团队系统相关 |
| `LogLyraGamePhase` | `Source/LyraGame/AbilitySystem/Phases/LyraGamePhaseLog.h` | Log | All | 游戏阶段相关 |
| `LogLyraCheat` | `Source/LyraGame/Player/LyraCheatManager.h` | Log | All | 作弊系统相关 |
| `LogLyraGameSettingRegistry` | `Source/LyraGame/Settings/LyraGameSettingRegistry.h` | Log | Log | 游戏设置注册表（编译时级别为 Log 而非 All） |
| `LogLyraRepGraph` | `Source/LyraGame/System/LyraReplicationGraph.h` | Display | All | 复制图谱相关（详细级别为 Display） |
| `LogLyraEditor` | `Source/LyraEditor/LyraEditor.h` | Log | All | 编辑器模块日志 |

---

## 当前实现边界

| 项目 | 当前状态 | 代码证据 | 含义 |
|---|---|---|---|
| 全局 Log Category（日志分类）声明 / 定义 | **已复刻** | `DECLARE_LOG_CATEGORY_EXTERN` 与对应 `DEFINE_LOG_CATEGORY` | 可供多个翻译单元记录日志 |
| `LogLyraCheat` | **已复刻** | `LyraCheatManager.h/.cpp` 中声明与定义 | 只证明日志通道存在 |
| `ULyraCheatManager` 功能 | **结构占位** | 类体只有 `GENERATED_BODY()` | 不能由 `LogLyraCheat` 推导 Cheat 功能完整 |
| `LogPlayerSpawning` | **已复刻** | `DEFINE_LOG_CATEGORY_STATIC` | 只在当前 `.cpp` 翻译单元可用 |

---

## 文件内静态日志分类

这些日志分类使用 `DEFINE_LOG_CATEGORY_STATIC` 定义在 `.cpp` 文件中，只能在当前翻译单元使用，不属于全局 extern 日志通道。

| 日志分类 | 源文件 | 默认详细级别 | 编译时级别 | 用途 |
|---------|--------|-------------|-----------|------|
| `LogPlayerSpawning` | `Source/LyraGame/Player/LyraPlayerSpawningManagerComponent.cpp` | Log | All | 出生点缓存、选择和重生管理组件内部诊断 |

---

## 辅助函数

### `GetClientServerContextString(UObject* ContextObject = nullptr)`

```cpp
LYRAGAME_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
```

定义于 `LyraLogChannels.h`。返回 `[Client]`、`[Server]` 或 `[Editor]` 上下文字符串，用于在日志输出中快速区分消息来源。

**使用示例:**
```cpp
UE_LOG(LogLyraExperience, Log, TEXT("%s Experience loading started"), *GetClientServerContextString(this));
// 输出: [Server] Experience loading started
```

在 Experience 加载代码中被大量使用，让网络环境下的日志阅读更直观。

---

## 注意事项

1. `LogLyraGameSettingRegistry` 的编译时级别为 `Log`（而非其他通道的 `All`），意味着在更严格的编译设置下可能被排除。
2. `LogLyraRepGraph` 的默认详细级别为 `Display`（而非 `Log`），在正常日志级别下即可显示。
3. `LyraCheatManager.h` 当前已经定义 `ULyraCheatManager`，不再属于“只有日志声明”的头文件；`LogLyraCheat` 由既有 `LyraCheatManager.cpp` 定义。
4. 以下 4 个头文件本身**仅包含日志声明**；这不代表对应系统一定不存在，
   仍需检查 `.cpp` 和其他头文件：
   - `LyraGameSettingRegistry.h`
   - `LyraReplicationGraph.h`
   - `LyraGamePhaseLog.h`
   - `LyraEditor.h`（`FLyraEditorModule` 实际定义在 `LyraEditor.cpp`）
5. `LogPlayerSpawning` 是文件内静态日志分类，不需要在头文件中声明，也不能被其他 `.cpp` 直接使用。

> ⚠️ **注意：**
> `DECLARE_LOG_CATEGORY_EXTERN` 只声明日志符号，`DEFINE_LOG_CATEGORY`
> 才提供定义；两者都不能证明同名业务系统已实现。判断功能状态仍需读取类体、
> 调用者和运行流程。

---

## 复习要点

1. 全局日志分类与 `DEFINE_LOG_CATEGORY_STATIC` 的可见范围有何不同？
2. 默认 Verbosity（详细级别）和编译时 Verbosity 分别控制什么？
3. 为什么存在 `LogLyraCheat` 不能证明 Cheat Manager 已完整复刻？
4. 网络日志为什么应配合 `GetClientServerContextString()` 使用？

## 关联框架

- [05-Player-Framework.md](05-Player-Framework.md) — `LogLyraTeams` 与 PlayerState / Controller 队伍变化协作
- [11-Development-Tools.md](11-Development-Tools.md) — `LogLyraCheat` 对应的 Cheat Manager 当前是结构占位
- [16-Stubs-and-Planned-Features.md](16-Stubs-and-Planned-Features.md) — 区分日志声明、结构占位与真实功能缺口
