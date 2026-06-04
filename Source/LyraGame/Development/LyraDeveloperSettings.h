#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "LyraDeveloperSettings.generated.h"

/**
 * 作弊的执行时机
 */
UENUM()
enum class ECheatExecutionTime
{
	//当作弊管理器被创建时
	OnCheatManagerCreated,

	//当玩家Pawn被控制时
	OnPlayerPawnPossession
};


/**
 * FLyraCheatToRun 是一个“PIE 自动执行 Cheat 的配置项”：它描述一条 console command 应该在什么开发阶段自动运行。
 */
USTRUCT()
struct FLyraCheatToRun
{
	GENERATED_BODY()

	//执行时机
	UPROPERTY(EditAnywhere)
	ECheatExecutionTime Phase = ECheatExecutionTime::OnPlayerPawnPossession;

	//要执行的 console command 字符串
	UPROPERTY(EditAnywhere)
	FString Cheat;
};

/**
 * 该类让带有Config标签的变量可以出现在Project Settings / Editor Settings 中被设置
 * 还可以使用ConsoleVariable=“控制台变量名”和对应控制台变量绑定
 * 方便开发者进行编辑器开发下的一些调试
 */
UCLASS(Config=EditorPerProjectUserSettings, MinimalAPI)
class ULyraDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	ULyraDeveloperSettings();

	//~UDeveloperSettings interface
	//告诉 UE Settings 面板：这个 Developer Settings 应该显示在哪个顶层分类下。
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface

public:
	//用于 Play in Editor 的 Experience 覆盖项；如果未设置，则会使用当前打开地图的 World Settings 中的默认 Experience。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category=Lyra, meta=(AllowedTypes="LyraExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;

	//是否重写机器人数量 
	//InlineEditConditionToggle 用来把一个 bool 开关显示到另一个属性的同一行，作为这个属性的启用 / 禁用复选框，而不是让这个 bool 自己单独占一行。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category=LyraBots, meta=(InlineEditConditionToggle))
	bool bOverrideBotCount = false;

	//EditCondition 的作用是：根据某个条件决定属性是否可编辑。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category=LyraBots, meta=(EditCondition=bOverrideBotCount))
	int32 OverrideNumPlayerBotsToSpawn = 0;

	//是否允许机器人攻击
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=LyraBots)
	bool bAllowPlayerBotsToAttack = true;

	//在编辑器中游玩时，是否执行完整的游戏流程，还是跳过“等待玩家”等游戏阶段？
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=Lyra)
	bool bTestFullGameFlowInPIE = false;

	/**
	 * 是否即使上一次输入设备不是手柄，也仍然播放力反馈效果？
	 * Lyra 的默认行为是：只有最近一次输入设备是手柄时，才播放力反馈。
	 * ConsoleVariable = "LyraPC.ShouldAlwaysPlayForceFeedback"将该变量和对应的控制台变量绑定起来
	 */
	UPROPERTY(config, EditAnywhere, Category = Lyra, meta = (ConsoleVariable = "LyraPC.ShouldAlwaysPlayForceFeedback"))
	bool bShouldAlwaysPlayForceFeedback = false;

	// 在编辑器中，游戏逻辑是否应该加载装饰性背景，还是为了提高迭代速度而跳过它们？
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category=Lyra)
	bool bSkipLoadingCosmeticBackgroundsInPIE = false;

	// PIE下自动运行的作弊列表
	UPROPERTY(config, EditAnywhere, Category=Lyra)
	TArray<FLyraCheatToRun> CheatsToRun;

	// 通过 Gameplay Message Subsystem 广播的消息是否应该被记录到日志中？
	// 该变量绑定的CVar位于GameplayMessageRouter插件GameplayMessageSubsystem.cpp文件下
	// 因为绑定的CVar和该变量不在同一模块下，需确认模块已经加载
	UPROPERTY(config, EditAnywhere, Category=GameplayMessages, meta=(ConsoleVariable="GameplayMessageSubsystem.LogMessages"))
	bool LogGameplayMessages = false;

#if WITH_EDITORONLY_DATA
	/** 可通过编辑器工具栏访问的常用地图列表。 */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
#endif

#if WITH_EDITOR

public:
	// 由 Editor Engine 调用，用于在作弊功能处于激活状态时弹出提醒通知。
	LYRAGAME_API void OnPlayInEditorStarted() const;

private:
	//ApplySettings() 是 ULyraDeveloperSettings 预留的“设置被初始化、修改或重新加载后，把配置立即应用到 Editor/PIE 状态”的 hook。
	void ApplySettings();
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
};
