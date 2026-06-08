//下一步重写GetDefaultPawnClassForController
#pragma once

#include "ModularGameMode.h"
#include "LyraGameMode.generated.h"

#define UE_API LYRAGAME_API

class ULyraPawnData;
class ULyraExperienceDefinition;
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLyraGameModePlayerInitialized, AGameModeBase* /*GameMode*/, AController* /*Controller*/);

/**
 * ALyraGameMode 继承 AModularGameModeBase，核心是为了走 AGameModeBase 的轻量服务器规则入口，
 * 同时接入 Lyra 的 ModularGameplay / GameFeature / Experience 架构，
 * 而不是使用 AGameMode 那套传统 MatchState 驱动流程。
 * Base 决定有没有 Match State；
 * Modular 决定能不能被 Game Feature 组件化扩展。
 */
UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "该项目使用的基础GameMode类"))
class ALyraGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	UE_API ALyraGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Lyra|Pawn")
	UE_API const ULyraPawnData* GetPawnDataForController(const AController* InController) const;

	//~AGameModeBase Interface

	UE_API virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	//GameState初始化函数，此时GameState已构造完成，在该函数中获取Experience管理组件，绑定Experience加载完成时的委托
	UE_API virtual void InitGameState() override;

	//该函数在InitNewPlayer中调用，此时按照Lyra的自定义逻辑我们还没有尝试创建Pawn、分配队伍等，更新玩家起点位置太早，所以重写该函数并什么都不做
	UE_API virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;

	/**
     * PostLogin中调用，在父类中有初始化HUD等作用
     * 新增了广播玩家已初始化完成的委托
     */
	UE_API virtual void GenericPlayerInitialization(AController* NewPlayer) override;

	/**
	 * 处理玩家启动函数，在PostLogin中调用，
	 * 为了确保在Experience加载完成后才进行RestartPlayer(创建Pawn等的函数)，重写该函数，添加一个Experience加载完成的判断条件
	 * 如果此时未加载Experience，RestartPlayer则在OnExperienceLoaded中执行
	 */
	UE_API virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UE_API virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	//是否在起始点生成，Lyra中默认为false
	UE_API virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	//选择出生点，转发给玩家生成管理组件LyraPlayerSpawningManagerComponent
	UE_API virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UE_API virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UE_API virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	UE_API virtual void FailedToRestartPlayer(AController* NewPlayer) override;

	UE_API virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	//~End of AGameModeBase Interface
	/**
	 * 判断控制器是否可以Restart
	 */
	UE_API virtual bool ControllerCanRestart(AController* Controller);

	//下一帧重启（重生）指定的玩家或 Bot。
	//如果 bForceReset 为 true，则会在当前帧重置 Controller，并放弃当前已 Possess 的 Pawn（如果有的话）
	UFUNCTION(BlueprintCallable)
	UE_API void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);

protected:
	//Experience加载完成时调用，遍历世界中的Controller，对还没有Pawn的PlayerController进行RestartPlayer
	UE_API void OnExperienceLoaded(const ULyraExperienceDefinition* CurrentExperience);

	//返回Experience是否已加载完成
	UE_API bool IsExperienceLoaded() const;

	//按优先级获取要使用的Experience,传给OnMatchAssignmentGiven
	UE_API void HandleMatchAssignmentIfNotExpectingOne();

	//如果ExperienceId有效，转发给ULyraExperienceManagerComponent加载对应Experience
	UE_API void OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource);

	UE_API bool TryDedicatedServerLogin();
	UE_API void HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode);

	UFUNCTION()
	UE_API void OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
	                                                ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);

public:
	// 委托通知玩家已初始化完成，AGameModeBase*, AController*为刚初始化完成的
	FOnLyraGameModePlayerInitialized OnGameModePlayerInitialized;
};
#undef UE_API
