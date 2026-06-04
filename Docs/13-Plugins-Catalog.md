# 13 - 插件目录

> Lyra 项目中所有 12 个插件的角色、依赖关系和模块类型。

---

## 总览

| # | 插件名 | 类型 | 分类 | 默认启用 | 依赖 |
|---|--------|------|------|---------|------|
| 1 | **AsyncMixin** | Runtime | UI | ❌ | 无 |
| 2 | **CommonGame** | Runtime | Gameplay | ✅ | CommonUI, CommonUser, ModularGameplayActors, OnlineFramework |
| 3 | **CommonLoadingScreen** | Runtime | Gameplay | ✅ | 无 |
| 4 | **CommonUser** | Runtime | Gameplay | ✅ | OnlineSubsystem, OnlineSubsystemUtils, OnlineServices |
| 5 | **GameSettings** | Runtime | UI | ✅ | CommonUI |
| 6 | **GameSubtitles** | Runtime | UI | ❌ | 无 |
| 7 | **GameplayMessageRouter** | Runtime | Gameplay | ✅ | GameplayTagsEditor |
| 8 | **ModularGameplayActors** | Runtime | Gameplay | ✅ | ModularGameplay |
| 9 | **PocketWorlds** | Runtime | UI | ❌ | 无 |
| 10 | **UIExtension** | Runtime | UI | ❌ | CommonUI, CommonGame |
| 11 | **LyraExampleContent** | Runtime (纯内容) | 内容 | ✅ | 无（无代码模块） |
| 12 | **LyraExtTool** | Editor | 其他 | ✅ | 无 |

---

## Runtime 插件详解

### 1. AsyncMixin
- **角色:** 提供异步操作管理的 C++ 工具类（加载等）
- **依赖:** 无
- **默认启用:** 否（由依赖它的插件激活）
- **模块数:** 1（Default）

### 2. CommonGame
- **角色:** 为使用 Common 系列插件的项目提供通用游戏类
- **依赖:** CommonUI, CommonUser, ModularGameplayActors, OnlineFramework
- **核心类:** `UCommonGameInstance`, `ACommonPlayerController`, `UCommonLocalPlayer`, `UCommonGameViewportClient`
- **影响:** Lyra 的 GameInstance、PlayerController、LocalPlayer、GameViewportClient 全部继承自此插件的类

### 3. CommonLoadingScreen
- **角色:** 加载画面管理器
- **依赖:** 无
- **模块:**
  - `CommonLoadingScreen` (Default) — 运行时加载画面逻辑
  - `CommonStartupLoadingScreen` (PreLoadingScreen) — 启动时加载画面，在 PreLoadingScreen 阶段加载以确保尽早显示
- **影响:** Experience 加载过程中显示的加载画面由 `ILoadingProcessInterface` 与此插件联动

### 4. CommonUser
- **角色:** 在线/平台操作的抽象层，封装不同平台的用户账号管理
- **依赖:** OnlineSubsystem, OnlineSubsystemUtils, OnlineServices
- **影响:** `ULyraGameInstance::HandlerUserInitialized()` 调用此插件的用户初始化流程

### 5. GameSettings
- **角色:** 游戏设置的定义系统和 UI 暴露接口
- **依赖:** CommonUI
- **模块数:** 1（Default）
- **影响:** `ULyraGameSettingRegistry`（当前仅有日志声明）与此插件框架配合

### 6. GameSubtitles
- **角色:** 与媒体播放器关联的字幕支持
- **依赖:** 无
- **默认启用:** 否
- **模块数:** 1（Default）

### 7. GameplayMessageRouter
- **角色:** 解耦的游戏消息广播系统，允许不相关联的对象之间传递消息
- **依赖:** GameplayTagsEditor
- **模块:**
  - `GameplayMessageRuntime` (Default) — 运行时消息路由
  - `GameplayMessageNodes` (UncookedOnly) — 蓝图节点（仅编辑器可用）
- **标记:** Beta
- **CVar:** `GameplayMessageSubsystem.LogMessages` — 由 `ULyraDeveloperSettings::LogGameplayMessages` 控制

### 8. ModularGameplayActors
- **角色:** 为 ModularGameplay 插件提供基础 Actor 类
- **依赖:** ModularGameplay
- **核心类:**
  - `AModularGameModeBase` — 游戏模式基类
  - `AModularGameStateBase` — 游戏状态基类
  - `AModularPlayerState` — 玩家状态基类
  - `AModularCharacter` — 角色基类
  - `AModularAIController` — AI 控制器基类
- **影响:** Lyra 的 GameMode、GameState、PlayerState、Character 全部继承自此插件的 Modular 变体，这是 GameFeature 可注入组件的基础

### 9. PocketWorlds
- **角色:** "口袋世界"系统，用于 UI 预览等场景的隔离子世界
- **依赖:** 无
- **默认启用:** 否
- **包含内容:** 是

### 10. UIExtension
- **角色:** 模块化 UI 元素扩展子系统，允许 GameFeature 插件动态向 UI 插入元素
- **依赖:** CommonUI, CommonGame
- **默认启用:** 否
- **模块数:** 1（Default）

### 11. LyraExampleContent
- **角色:** Lyra 示例内容资产包
- **类型:** 纯内容（无 C++ 模块、无代码）
- **依赖:** 无
- **包含内容:** 是

---

## Editor 插件详解

### 12. LyraExtTool
- **角色:** 编辑器扩展工具
- **类型:** Editor-Only
- **依赖:** 无
- **模块:** `LyraExtTool` (Editor)
- **包含内容:** 是

---

## 依赖关系图

```
                   CommonUI
                      |
            ┌─────────┼─────────┐
            |         |         |
    CommonGame    GameSettings  UIExtension
            |
    ┌───────┼───────────┐
    |       |           |
CommonUser  Modular     OnlineFramework
            |
     ModularGameplayActors
            |
     ModularGameplay
```

LyraGame 模块同时依赖 CommonGame、ModularGameplayActors、GameFeatures、GameplayAbilities 等插件。

---

## 关联框架

- [03-System-Framework.md](03-System-Framework.md) — ULyraGameInstance 继承 UCommonGameInstance
- [05-Player-Framework.md](05-Player-Framework.md) — Lyra 玩家类继承自 Common 基类
- [07-Experience-Framework.md](07-Experience-Framework.md) — GameFeature 插件加载由 Experience 系统驱动
