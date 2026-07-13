#include "LyraPlayerController.h"

#include "HttpServerModule.h"
#include "LyraCheatManager.h"
#include "LyraPlayerState.h"
#include "Camera/LyraPlayerCameraManager.h"
#include "Net/UnrealNetwork.h"
#include "UI/LyraHUD.h"
#if WITH_RPC_REGISTRY
#include "Tests/LyraGameplayRpcRegistrationComponent.h"
#include "HttpServerModule.h"
#endif


#include "AbilitySystemGlobals.h"
#include "CommonInputSubsystem.h"
#include "EngineUtils.h"
#include "GameMapsSettings.h"
#include "LyraGameplayTags.h"
#include "LyraLocalPlayer.h"
#include "LyraLogChannels.h"
#include "ReplaySubsystem.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Development/LyraDeveloperSettings.h"
#include "GameModes/LyraGameState.h"
#include "Replays/LyraReplaySubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlayerController)

namespace Lyra::Input
{
	static int32 ShouldAlwaysPlayForceFeedback = 0;
	static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(TEXT("LyraPC.ShouldAlwaysPlayForceFeedback"),
	                                                                 ShouldAlwaysPlayForceFeedback,
	                                                                 TEXT("是否开启力反馈，即使上次输入设备不是手柄"));
}

ALyraPlayerController::ALyraPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ALyraPlayerCameraManager::StaticClass();
#if USING_CHEAT_MANAGER
	CheatClass = ULyraCheatManager::StaticClass();
#endif
}

ALyraPlayerState* ALyraPlayerController::GetLyraPlayerState() const
{
	//允许为null，只在类型不正确时触发checked
	return CastChecked<ALyraPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

ULyraAbilitySystemComponent* ALyraPlayerController::GetLyraAbilitySystemComponent() const
{
	const ALyraPlayerState* LyraPS = GetLyraPlayerState();
	return (LyraPS ? LyraPS->GetLyraAbilitySystemComponent() : nullptr);
}

ALyraHUD* ALyraPlayerController::GetLyraHUD() const
{
	return CastChecked<ALyraHUD>(GetHUD(), ECastCheckedType::NullAllowed);
}

bool ALyraPlayerController::TryToRecordClientReplay()
{
	// 确认是否可以进行录制
	if (ShouldRecordClientReplay())
	{
		if (ULyraReplaySubsystem* ReplaySubsystem = GetGameInstance()->GetSubsystem<ULyraReplaySubsystem>())
		{
			//获取第一个本地的玩家控制器
			APlayerController* FirstLocalPlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			//如果获取的玩家控制器与本控制器相同
			if (FirstLocalPlayerController == this)
			{
				// If this is the first player, update the spectator player for local replays and then record
				if (ALyraGameState* GameState = Cast<ALyraGameState>(GetWorld()->GetGameState()))
				{
					GameState->SetRecorderPlayerState(PlayerState);
					
					//TODO:等待回放函数完成
					//ReplaySubsystem->RecordClientReplay(this);
					return true;
				}
			}
		}
	}
	return false;
}

bool ALyraPlayerController::ShouldRecordClientReplay()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	//仅在当前没在播放回放、没有在录制回放、非专用服务器、是本地玩家时可进行回放录制
	if (GameInstance != nullptr &&
		World != nullptr &&
		!World->IsPlayingReplay() &&
		!World->IsRecordingClientReplay() &&
		NM_DedicatedServer != GetNetMode() &&
		IsLocalPlayerController())
	{
		//获取默认地图
		FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
		//获取当前地图
		FString CurrentMap = World->URL.Map;

#if WITH_EDITOR
		//处理 PIE 地图前缀
		//在编辑器内运行 PIE（Play In Editor）时，UE 会复制地图并添加类似下面的前缀：UEDPIE_0_正确比较。
		//如果不移除前缀：/Game/.../UEDPIE_0_L_LyraFrontEnd就无法与配置中的：/Game/.../L_LyraFrontEnd
		CurrentMap = UWorld::StripPIEPrefixFromPackageName(CurrentMap, World->StreamingLevelsPrefix);
#endif
		if (CurrentMap == DefaultMap)
		{
			// 绝不在默认的前端地图中录制 Replay；这里可以改用更可靠的方式来检查当前是否处于主菜单。
			return false;
		}

		if (UReplaySubsystem* ReplaySubsystem = GameInstance->GetSubsystem<UReplaySubsystem>())
		{
			if (ReplaySubsystem->IsRecording() || ReplaySubsystem->IsPlaying())
			{
				// 同一时间只能有一个。

				return false;
			}
		}

		// 如果满足此条件，接下来检查相关设置。
		if (const ULyraLocalPlayer* LyraLocalPlayer = Cast<ULyraLocalPlayer>(GetLocalPlayer()))
		{
			//TODO:等待LocalSetting等相关类完成
			// if (LyraLocalPlayer->GetLocalSettings()->ShouldAutoRecordReplays())
			// {
			// 	return true;
			// }
		}
	}
	return false;
}

void ALyraPlayerController::ServerCheat_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogLyra, Warning, TEXT("ServerCheat: %s"), *Msg);
		ClientMessage(ConsoleCommand(Msg));
	}
#endif // #if USING_CHEAT_MANAGER
}

bool ALyraPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void ALyraPlayerController::ServerCheatAll_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogLyra, Warning, TEXT("ServerCheatAll: %s"), *Msg);
		for (TActorIterator<ALyraPlayerController> It(GetWorld()); It; ++It)
		{
			ALyraPlayerController* LyraPC = (*It);
			if (LyraPC)
			{
				LyraPC->ClientMessage(LyraPC->ConsoleCommand(Msg));
			}
		}
	}
#endif // #if USING_CHEAT_MANAGER
}

bool ALyraPlayerController::ServerCheatAll_Validate(const FString& Msg)
{
	return true;
}

void ALyraPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ALyraPlayerController::BeginPlay()
{
	Super::BeginPlay();
#if WITH_RPC_REGISTRY
	FHttpServerModule::Get().StartAllListeners();
	int32 RpcPort = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("rpcport="), RpcPort))
	{
		//todo:暂时不需要
		// ULyraGameplayRpcRegistrationComponent* ObjectInstance = ULyraGameplayRpcRegistrationComponent::GetInstance();
		// if (ObjectInstance && ObjectInstance->IsValidLowLevel())
		// {
		// 	ObjectInstance->RegisterAlwaysOnHttpCallbacks();
		// 	ObjectInstance->RegisterInMatchHttpCallbacks();
		// }
	}
#endif
	//确保不要将该Actor隐藏
	SetActorHiddenInGame(false);
}

void ALyraPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ALyraPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 禁用 PC 目标视角的复制，因为它不太适用于回放或客户端侧观战。
	// 引擎的 TargetViewRotation 只会在 APlayerController::TickActor 中设置，前提是服务器提前知道
	// 当前正在观战某个特定 Pawn，并且它只会以 COND_OwnerOnly 条件向下复制。
	// 在客户端保存的回放中，COND_OwnerOnly 永远不会为 true，并且在录制时目标 Pawn 并不总是已知的。
	// 为了支持客户端保存的回放，该复制逻辑被移动到了 ReplicatedViewRotation，并在 PlayerTick 中更新。
	DISABLE_REPLICATED_PROPERTY(APlayerController, TargetViewRotation);
}

void ALyraPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
#if WITH_SERVER_CODE&&WITH_EDITOR

	//确保当前请求被控制的pawn不为空，且请求被控制的pawn与当前PC持有的Pawn相同
	//这个判断的本质是：用最终 Controller 状态验证 possession 是否真的生效。
	if (GIsEditor && (InPawn != nullptr) && (GetPawn() == InPawn))
	{
		for (const FLyraCheatToRun& CheatRow : GetDefault<ULyraDeveloperSettings>()->CheatsToRun)
		{
			if (CheatRow.Phase == ECheatExecutionTime::OnPlayerPawnPossession)
			{
				ConsoleCommand(CheatRow.Cheat, /*bWriteToLog=*/ true);
			}
		}
	}
#endif
	SetIsAutoRunning(false);
}

void ALyraPlayerController::OnUnPossess()
{
	//在解除控制该Pawn之前，如果该控制器的ASC与当前要解除控制的Pawn相同，则将ASC的Avatar置为空
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		const APlayerState* ThePlayerState = PlayerState.Get();
		if (IsValid(ThePlayerState))
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ThePlayerState))
			{
				if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
				{
					ASC->SetAvatarActor(nullptr);
				}
			}
		}
	}
	Super::OnUnPossess();
}

void ALyraPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ALyraPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ALyraPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
	// 当我们作为客户端连接到远程服务器时，PlayerController 的复制可能会晚于 PlayerState 和 AbilitySystemComponent。
	// 然而，TryActivateAbilitiesOnSpawn 依赖 PlayerController 已经完成复制，
	// 以便检查“生成时激活”的 Ability 是否应该在本地执行。
	// 因此，一旦 PlayerController 存在并解析到了 PlayerState，就再次尝试激活“生成时激活”的 Ability。
	// 在其他网络模式下，PlayerController 不会延迟复制，所以 LyraASC 自身对 TryActivateAbilitiesOnSpawn 的调用会成功。
	// 这里的处理只针对这种情况：PlayerState 和 ASC 先于 PC 复制完成，
	// 当时 AbilityActorInfo 里的 PlayerController / 本地控制关系还不完整，于是 ActorInfo->IsLocallyControlled() 可能判断失败。
	// 导致它们误以为这些 Ability 不是给本地玩家用的。
	// 它只是客户端补刷新和补激活，真正激活 Ability 的地方不在此处
	if (GetWorld()->IsNetMode(NM_Client))
	{
		if (ALyraPlayerState* LyraPS = GetPlayerState<ALyraPlayerState>())
		{
			if (ULyraAbilitySystemComponent* LyraASC = LyraPS->GetLyraAbilitySystemComponent())
			{
				LyraASC->RefreshAbilityActorInfo();
				//todo:尝试激活生成时要执行的能力
				//LyraASC->TryActivateAbilitiesOnSpawn();
			}
		}
	}
}

void ALyraPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void ALyraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	//如果开启了自动奔跑，则每帧给一个向控制器的朝向自动移动
	if (GetIsAutoRunning())
	{
		if (APawn* CurrentPawn = GetPawn())
		{
			const FRotator MovementRotation(0.0f, GetControlRotation().Yaw, 0.0f);
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			CurrentPawn->AddMovementInput(MovementDirection, 1.0f);
		}
	}

	ALyraPlayerState* LyraPlayerState = GetLyraPlayerState();

	if (PlayerCameraManager && LyraPlayerState)
	{
		//获取摄像机当前正在观看的目标
		APawn* TargetPawn = PlayerCameraManager->GetViewTargetPawn();

		if (TargetPawn)
		{
			// Server 上写，是为了正常复制给其他客户端。
			// 本地控制 Pawn 上也写，是为了本地表现和 client-saved replay 能拿到视角状态。
			if (HasAuthority() || TargetPawn->IsLocallyControlled())
			{
				LyraPlayerState->SetReplicatedViewRotation(TargetPawn->GetViewRotation());
			}

			// 如果获取的Pawn不是本地控制的，则获取这个Pawn的观看旋转视角，设置给本地的PC使用
			if (!TargetPawn->IsLocallyControlled())
			{
				LyraPlayerState = TargetPawn->GetPlayerState<ALyraPlayerState>();
				if (LyraPlayerState)
				{
					TargetViewRotation = LyraPlayerState->GetReplicatedViewRotation();
				}
			}
		}
	}
}

void ALyraPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);
	if (const ULyraLocalPlayer* LyraLocalPlayer = Cast<ULyraLocalPlayer>(InPlayer))
	{
		// ULyraSettingsShared* UserSettings = LyraLocalPlayer->GetSharedSettings();
		// UserSettings->OnSettingChanged.AddUObject(this, &ThisClass::OnSettingsChanged);
		//
		// OnSettingsChanged(UserSettings);
	}
}

void ALyraPlayerController::AddCheats(bool bForce)
{
#if USING_CHEAT_MANAGER
	Super::AddCheats(true);
#else //#if USING_CHEAT_MANAGER
	Super::AddCheats(bForce);
#endif // #else //#if USING_CHEAT_MANAGER
}

void ALyraPlayerController::UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId)
{
	if (bForceFeedbackEnabled)
	{
		if (const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
		{
			//获取当前输入设备类型
			const ECommonInputType CurrentInputType = CommonInputSubsystem->GetCurrentInputType();
			if (Lyra::Input::ShouldAlwaysPlayForceFeedback || CurrentInputType == ECommonInputType::Gamepad || CurrentInputType ==
				ECommonInputType::Touch)
			{
				//如果当前开启了强制力反馈或者设备是手柄或者触摸屏，则将计算好的力反馈数值输出到设备中
				InputInterface->SetForceFeedbackChannelValues(ControllerId, ForceFeedbackValues);
				return;
			}
		}
	}
	//如果没有则输出一个空值，即没有力反馈
	InputInterface->SetForceFeedbackChannelValues(ControllerId, FForceFeedbackValues());
}

void ALyraPlayerController::UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents)
{
	Super::UpdateHiddenComponents(ViewLocation, OutHiddenComponents);

	if (bHideViewTargetPawnNextFrame)
	{
		//获取摄像机当前观察的目标Pawn
		const AActor* ViewTargetPawn = PlayerCameraManager ? Cast<AActor>(PlayerCameraManager->GetViewTarget()) : nullptr;
		if (ViewTargetPawn)
		{
			// TInlineComponentArray 本质上是一个 TArray，它会在栈上预留固定大小的空间，当预计结果中的元素数量少于指定数量时，尽量避免进行堆内存分配。 
			// 用于隐藏所有组件的内部辅助函数。
			auto AddToHiddenComponents = [&OutHiddenComponents](const TInlineComponentArray<UPrimitiveComponent*>& InComponents)
			{
				// 添加所有组件及其附加的全部子组件。
				for (UPrimitiveComponent* Comp : InComponents)
				{
					if (Comp->IsRegistered())
					{
						OutHiddenComponents.Add(Comp->GetPrimitiveSceneId());

						for (USceneComponent* AttachedChild : Comp->GetAttachChildren())
						{
							//不随父级自动隐藏标签
							static FName NAME_NoParentAutoHide(TEXT("NoParentAutoHide"));
							UPrimitiveComponent* AttachChildPC = Cast<UPrimitiveComponent>(AttachedChild);
							if (AttachChildPC && AttachChildPC->IsRegistered() && !AttachChildPC->ComponentTags.Contains(NAME_NoParentAutoHide))
							{
								/**
								 * FPrimitiveComponentId 是渲染系统使用的轻量级运行时标识符（Runtime Identifier）：
								 * 用一个 uint32 在游戏线程（Game Thread）与渲染线程（Render Thread）之间代表某个 UPrimitiveComponent，
								 * 避免把组件指针传到渲染线程后误访问游戏线程数据。
								 */
								OutHiddenComponents.Add(AttachChildPC->GetPrimitiveSceneId());
							}
						}
					}
				}
			};

			//TODO 通过接口来解决。收集需要隐藏的组件之类的。
			//TODO Hiding isn't awesome, sometimes you want the effect of a fade out over a proximity, needs to bubble up to designers.

			// hide pawn's components
			TInlineComponentArray<UPrimitiveComponent*> PawnComponents;
			//获取当前观察的目标Pawn的所有UPrimitiveComponent并添加到OutHiddenComponents数组中
			ViewTargetPawn->GetComponents(PawnComponents);
			AddToHiddenComponents(PawnComponents);

			//// hide weapon too
			//if (ViewTargetPawn->CurrentWeapon)
			//{
			//	TInlineComponentArray<UPrimitiveComponent*> WeaponComponents;
			//	ViewTargetPawn->CurrentWeapon->GetComponents(WeaponComponents);
			//	AddToHiddenComponents(WeaponComponents);
			//}
		}

		// 我们已经消费了它，将其重置以供下一帧使用。
		bHideViewTargetPawnNextFrame = false;
	}

}

void ALyraPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void ALyraPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
	{
		//TODO:转发给ASC处理
		//LyraASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void ALyraPlayerController::OnCameraPenetratingTarget()
{
	bHideViewTargetPawnNextFrame = true;
}

void ALyraPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogLyraTeams, Error, TEXT("不应该在 player controller (%s)上设置队伍ID; 它由关联的 PlayerState 驱动。"), *GetPathNameSafe(this));
}

FGenericTeamId ALyraPlayerController::GetGenericTeamId() const
{
	if (const ILyraTeamAgentInterface* PSWithTeamInterface = Cast<ILyraTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnLyraTeamIndexChangedDelegate* ALyraPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ALyraPlayerController::SetIsAutoRunning(const bool bEnabled)
{
	const bool bIsAutoRunning = GetIsAutoRunning();
	//只在当前状态与传入状态不一致时，才开启或关闭自动奔跑
	if (bIsAutoRunning != bEnabled)
	{
		if (!bEnabled)
		{
			OnEndAutoRun();
		}
		else
		{
			OnStartAutoRun();
		}
	}
}

bool ALyraPlayerController::GetIsAutoRunning() const
{
	bool bIsAutoRunning = false;
	if (const ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
	{
		//根据自动奔跑Tag的数量判断当前是否开启自动奔跑
		bIsAutoRunning = LyraASC->GetTagCount(LyraGameplayTags::Status_AutoRunning) > 0;
	}
	return bIsAutoRunning;
}

void ALyraPlayerController::OnPlayerStateChanged()
{
	//空实现，供派生类实现自己的逻辑，而不必绑定所有其他事件。
}
//TODO:等待完成ULyraSettingsShared类
// void ALyraPlayerController::OnSettingsChanged(ULyraSettingsShared* Settings)
// {
// }

void ALyraPlayerController::OnStartAutoRun()
{
	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
	{
		LyraASC->SetLooseGameplayTagCount(LyraGameplayTags::Status_AutoRunning, 1);
		K2_OnStartAutoRun();
	}
}

void ALyraPlayerController::OnEndAutoRun()
{
	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
	{
		LyraASC->SetLooseGameplayTagCount(LyraGameplayTags::Status_AutoRunning, 0);
		K2_OnEndAutoRun();
	}
}

void ALyraPlayerController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();
	// 如果有旧的PS，获取旧队伍ID，并将绑定到队伍更换委托上的所有函数解绑
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (ILyraTeamAgentInterface* PlayerStateTeamInterface = Cast<ILyraTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// 将OnPlayerStateChangedTeam绑定到新的PS的更换队伍委托上
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (ILyraTeamAgentInterface* PlayerStateTeamInterface = Cast<ILyraTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// 广播队伍的改变
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	// 更新最近一次持有的PS
	LastSeenPlayerState = PlayerState;
}

void ALyraPlayerController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void ALyraReplayPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 在 Replay 播放过程中拖动播放进度时，该状态可能随时失效。
	if (!IsValid(FollowedPlayerState))
	{
		UWorld* World = GetWorld();

		// 同时监听录制和播放过程中的变化。
		if (ALyraGameState* GameState = Cast<ALyraGameState>(World->GetGameState()))
		{
			if (!GameState->OnRecorderPlayerStateChangedEvent.IsBoundToObject(this))
			{
				GameState->OnRecorderPlayerStateChangedEvent.AddUObject(this, &ThisClass::RecorderPlayerStateUpdated);
			}
			//获取当前的RecorderPlayerState并通知该回放控制器更新
			if (APlayerState* RecorderState = GameState->GetRecorderPlayerState())
			{
				RecorderPlayerStateUpdated(RecorderState);
			}
		}
	}

}

void ALyraReplayPlayerController::SmoothTargetViewRotation(APawn* TargetPawn, float DeltaSeconds)
{
	Super::SmoothTargetViewRotation(TargetPawn, DeltaSeconds);
}

bool ALyraReplayPlayerController::ShouldRecordClientReplay()
{
	return false;
}

void ALyraReplayPlayerController::RecorderPlayerStateUpdated(APlayerState* NewRecorderPlayerState)
{
	if (NewRecorderPlayerState)
	{
		FollowedPlayerState = NewRecorderPlayerState;

		// 绑定 Pawn 变更事件，并立即调用一次。
		NewRecorderPlayerState->OnPawnSet.AddUniqueDynamic(this, &ALyraReplayPlayerController::OnPlayerStatePawnSet);
		OnPlayerStatePawnSet(NewRecorderPlayerState, NewRecorderPlayerState->GetPawn(), nullptr);
	}
}

void ALyraReplayPlayerController::OnPlayerStatePawnSet(APlayerState* ChangedPlayerState, APawn* NewPlayerPawn, APawn* OldPlayerPawn)
{
	//如果当前正在跟随的PS和传入的相同，则重新设置新的观察目标
	if (ChangedPlayerState == FollowedPlayerState)
	{
		SetViewTarget(NewPlayerPawn);
	}

}
