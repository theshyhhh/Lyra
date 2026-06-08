#include "LyraGameState.h"

#include "LyraExperienceManagerComponent.h"
#include "LyraLogChannels.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Messages/LyraVerbMessage.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameState)

extern ENGINE_API float GAverageFPS;

ALyraGameState::ALyraGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//开启Tick函数
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<ULyraAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	ExperienceManagerComponent = CreateDefaultSubobject<ULyraExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
	ServerFPS = 0.0f;
}

void ALyraGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ALyraGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ALyraGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ALyraGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() == ROLE_Authority)
	{
		ServerFPS = GAverageFPS;
	}
}

void ALyraGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
}

void ALyraGameState::RemovePlayerState(APlayerState* PlayerState)
{
	//@TODO：目前这个函数还没有被调用（只有“完整版/复杂版”的 AGameMode 会使用它，AGameModeBase 不会使用）
	// 至少需要给引擎代码补充注释，并且可能需要重新调整一些代码位置
	Super::RemovePlayerState(PlayerState);
}

void ALyraGameState::SeamlessTravelTransitionCheckpoint(bool bToTransitionMap)
{
	for (int i = PlayerArray.Num() - 1; i >= 0; i--)
	{
		APlayerState* PS = PlayerArray[i];
		if (PS && (PS->IsABot() || PS->IsInactive()))
		{
			RemovePlayerState(PS);
		}
	}
}

UAbilitySystemComponent* ALyraGameState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ALyraGameState::GetServerFPS() const
{
	return ServerFPS;
}

void ALyraGameState::SetRecorderPlayerState(APlayerState* NewPlayerState)
{
	if (RecorderPlayerState == nullptr)
	{
		//设置它并调用复制回调函数，以便它可以执行任何录制时的初始化设置。
		RecorderPlayerState = NewPlayerState;
		OnRep_RecorderPlayerState();
	}
	else
	{
		UE_LOG(LogLyra, Warning, TEXT("在 %s 上调用了 SetRecorderPlayerState，但它每局游戏应该只在主用户上调用一次。"), *GetName());
	}
}

APlayerState* ALyraGameState::GetRecorderPlayerState() const
{
	// TODO：如果为空，是否可以自动选中它？

	return RecorderPlayerState;
}

void ALyraGameState::MulticastMessageToClients_Implementation(const FLyraVerbMessage Message)
{
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

void ALyraGameState::MulticastReliableMessageToClients_Implementation(const FLyraVerbMessage Message)
{
	MulticastMessageToClients_Implementation(Message);
}

void ALyraGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ServerFPS)
	DOREPLIFETIME_CONDITION(ThisClass, RecorderPlayerState, COND_ReplayOnly);
}

void ALyraGameState::OnRep_RecorderPlayerState()
{
	OnRecorderPlayerStateChangedEvent.Broadcast(RecorderPlayerState);
}
