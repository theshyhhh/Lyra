# 10 - 日志系统

> 所有 Lyra 定义的日志通道速查表。

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
4. 仍有 4 个日志通道所在的头文件**仅包含日志声明**，没有对应功能类定义：
   - `LyraGameSettingRegistry.h`
   - `LyraReplicationGraph.h`
   - `LyraGamePhaseLog.h`
   - `LyraEditor.h`
5. `LogPlayerSpawning` 是文件内静态日志分类，不需要在头文件中声明，也不能被其他 `.cpp` 直接使用。
