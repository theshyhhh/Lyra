#pragma once

#include "CommonPlayerController.h"
#include "Camera/LyraCameraAssistInterface.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "LyraPlayerController.generated.h"

#define UE_API LYRAGAME_API

class ULyraAbilitySystemComponent;
class ALyraPlayerState;
class ALyraHUD;
class ULyraSettingsShared;

UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "该项目使用的PlayerController基类"))
class ALyraPlayerController : public ACommonPlayerController, public ILyraTeamAgentInterface, public ILyraCameraAssistInterface
{
	GENERATED_BODY()

public:
	UE_API ALyraPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//获取玩家状态
	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerController")
	UE_API ALyraPlayerState* GetLyraPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerController")
	UE_API ULyraAbilitySystemComponent* GetLyraAbilitySystemComponent() const;

	//获取玩家HUD
	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerController")
	UE_API ALyraHUD* GetLyraHUD() const;
	
	// 当 ShouldRecordClientReplay 返回 true 时，由 Game State 逻辑调用此函数，以开始自动录制客户端 Replay。
	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerController")
	UE_API bool TryToRecordClientReplay();

	// 调用此函数以确定是否应该录制 Replay，子类可以更改此行为。
	UE_API virtual bool ShouldRecordClientReplay();

	// 在服务器上运行作弊命令 WithValidation：指定一个RPC函数在执行前需要验证，只有验证通过才可以执行。
	UFUNCTION(Reliable, Server, WithValidation)
	UE_API void ServerCheat(const FString& Msg);

	// 在服务器上对所有玩家运行作弊命令
	UFUNCTION(Reliable, Server, WithValidation)
	UE_API void ServerCheatAll(const FString& Msg);

	//~AActor interface

	UE_API virtual void PreInitializeComponents() override;

	/**
	 * 1. 处理HTTPServer自动化测试相关
	 * 2. 确保该PC不被隐藏
	 */
	UE_API virtual void BeginPlay() override;

	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 禁用TargetViewRotation的复制，自己重新处理
	 */
	UE_API virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~AController interface

	/**
	 * 在控制Pawn时调用，Lyra中负责执行在该阶段执行的作弊命令、关闭自动奔跑
	 */
	UE_API virtual void OnPossess(APawn* InPawn) override;

	/**
	 * 解除控制时，更新当前ASC的Avatar
	 */
	UE_API virtual void OnUnPossess() override;

	//广播PlayerState改变
	UE_API virtual void InitPlayerState() override;

	//广播PlayerState改变
	UE_API virtual void CleanupPlayerState() override;

	// 1. 广播PlayerState改变
	// 2. 处理在远程客户端上，PC的复制可能慢于PS,ASC的复制，导致PS,ASC中ActorInfo->IsLocallyControlled()等判断可能失败的情况，给客户端补刷新和补激活
	UE_API virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface

	//SetPlayer时自动调用该函数
	UE_API virtual void ReceivedPlayer() override;

	/** 
	 * PC中：处理玩家输入（紧接在 PlayerInput 被 Tick 之后），并调用 UpdateRotation()。
	 * 只有当 PlayerController 拥有 PlayerInput 对象时，才会调用 PlayerTick。
	 * 因此，它只会在本地控制的 PlayerController 上被调用。
	 * LyraPC中：1.开启自动奔跑时，执行自动奔跑逻辑
	 * 2.负责每帧设置或者获取当前观察的目标的视角的旋转，目前只对本地保存的回放中本地控制的玩家有效
	 */
	UE_API virtual void PlayerTick(float DeltaTime) override;

	//TODO:等待完成ULyraSettingsShared类
	UE_API virtual void SetPlayer(UPlayer* InPlayer) override;

	//调用此函数来尝试为该玩家启用作弊功能，它会在初始化期间或通过 AllowCheats 命令触发。
	UE_API virtual void AddCheats(bool bForce) override;

	//根据当前设备类型，更新力反馈
	UE_API virtual void UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId) override;

	/**
	 * 
	 * 基于游戏逻辑构建需要隐藏的组件列表。
	 * @param ViewLocation 用于确定隐藏或取消隐藏组件的视点位置。
	 * @param OutHiddenComponents  要向其中添加或从中移除组件的列表。
	 */
	UE_API virtual void UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents) override;

	UE_API virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;

	//转发给ASC处理
	UE_API virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

	//~ILyraCameraAssistInterface interface
	//当相机穿透时调用，将 bHideViewTargetPawnNextFrame 置为true
	UE_API virtual void OnCameraPenetratingTarget() override;
	//~End of ILyraCameraAssistInterface interface

	//~ILyraTeamAgentInterface interface

	//设置队伍ID由关联的PlayerState处理，不由该PC处理
	UE_API virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	//从PS上获取队伍ID
	UE_API virtual FGenericTeamId GetGenericTeamId() const override;

	UE_API virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface

	/**
	 * 只在传入状态与正在执行的状态不一致时才执行
	 * @param bEnabled 是否开启自动奔跑
	 */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
	UE_API void SetIsAutoRunning(const bool bEnabled);

	/**
	 * @return 返回当前是否开启自动奔跑
	 */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
	UE_API bool GetIsAutoRunning() const;

protected:
	//在BroadcastOnPlayerStateChanged中调用，此处为空，供派生类拓展逻辑
	UE_API virtual void OnPlayerStateChanged();

	//TODO:等待完成ULyraSettingsShared类
	//UE_API void OnSettingsChanged(ULyraSettingsShared* Settings);

	//给ASC添加自动奔跑标签
	UE_API void OnStartAutoRun();

	//给ASC移除自动奔跑标签
	UE_API void OnEndAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnStartAutoRun"))
	UE_API void K2_OnStartAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnEndAutoRun"))
	UE_API void K2_OnEndAutoRun();

	//下一帧是否隐藏视图目标Pawn
	bool bHideViewTargetPawnNextFrame = false;

private:
	/**
	 * 广播PlayerState更改，解绑旧PS上的委托，更新并广播新队伍ID
	 */
	void BroadcastOnPlayerStateChanged();

	//PS更换队伍ID时的回调函数，用于广播OnTeamChangedDelegate委托
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	//最近一次持有的PS，主要用于PlayerState改变时，获取旧PlayerState的队伍ID，和解绑旧委托
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};

UCLASS()
class ALyraReplayPlayerController : public ALyraPlayerController
{
	GENERATED_BODY()
	/**
	 * Replay 观战控制器每帧执行的“跟随目标修复器”：
	 * 当 Replay 拖动或加载检查点导致原 PlayerState 被销毁后，它重新绑定当前 ALyraGameState，
	 * 找回录制者的 PlayerState，最终让相机重新跟随其 Pawn。
	 */
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void SmoothTargetViewRotation(APawn* TargetPawn, float DeltaSeconds) override;
	
	virtual bool ShouldRecordClientReplay() override;

	// 在 Replay 播放期间，当 Game State 的 RecorderPlayerState 完成复制时触发的回调。
	void RecorderPlayerStateUpdated(APlayerState* NewRecorderPlayerState);

	// 当被跟随的 PlayerState 更换 Pawn 时触发的回调。
	UFUNCTION()
	void OnPlayerStatePawnSet(APlayerState* ChangedPlayerState, APawn* NewPlayerPawn, APawn* OldPlayerPawn);

	// 我们当前正在跟随的 PlayerState。
	UPROPERTY(Transient)
	TObjectPtr<APlayerState> FollowedPlayerState;
};

#undef UE_API
