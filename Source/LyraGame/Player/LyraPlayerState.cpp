#include "LyraPlayerState.h"

#include "Character/LyraPawnData.h"
#include "LyraLogChannels.h"
#include "LyraPlayerController.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "GameModes/LyraGameMode.h"
#include "Messages/LyraVerbMessage.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlayerState)

const FName ALyraPlayerState::NAME_LyraAbilityReady("LyraAbilitiesReady");


ALyraPlayerState::ALyraPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  MyPlayerConnectionType(ELyraPlayerConnectionType::Player)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<ULyraAbilitySystemComponent>(this,TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	//TODO:等待实现ULyra***Set
	// HealthSet = CreateDefaultSubobject<ULyraHealthSet>(TEXT("HealthSet"));
	// CombatSet = CreateDefaultSubobject<ULyraCombatSet>(TEXT("CombatSet"));

	SetNetUpdateFrequency(100.f);

	MyTeamID = FGenericTeamId::NoTeam;
	MySquadID = INDEX_NONE;
}

ALyraPlayerController* ALyraPlayerState::GetLyraPlayerController() const
{
	return Cast<ALyraPlayerController>(GetOwner());
}

UAbilitySystemComponent* ALyraPlayerState::GetAbilitySystemComponent() const
{
	return GetLyraAbilitySystemComponent();
}

void ALyraPlayerState::SetPawnData(const ULyraPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogLyra, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."),
		       *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	//TODO:赋予技能相关
	// for (const ULyraAbilitySet* AbilitySet : PawnData->AbilitySets)
	// {
	// 	if (AbilitySet)
	// 	{
	// 		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
	// 	}
	// }

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_LyraAbilityReady);

	ForceNetUpdate();
}

void ALyraPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//启用 Push-based Replication。意思是属性变化时需要主动标记 dirty
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MySquadID, SharedParams);

	//跳过 Owner，也就是不复制给拥有这个 PlayerState 的客户端，只复制给其他客户端。
	SharedParams.Condition = ELifetimeCondition::COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, SharedParams);

	DOREPLIFETIME(ThisClass, StatTags);
}

void ALyraPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ALyraPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		AGameStateBase* GameState = GetWorld()->GetGameState();
		check(GameState);
		ULyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULyraExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(
			FOnLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void ALyraPlayerState::Reset()
{
	Super::Reset();
}

void ALyraPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	//todo:ULyraPawnExtensionComponent逻辑待完成
	// if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	// {
	// 	PawnExtComp->CheckDefaultInitialization();
	// }
}

void ALyraPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	//@TODO: Copy stats
}

void ALyraPlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
	case ELyraPlayerConnectionType::Player:
	case ELyraPlayerConnectionType::InactivePlayer:
		//@TODO：询问 Experience：对于正在断开连接的玩家，我们是应该立即销毁他们，还是让他们保留下来
		// （例如，对于长时间运行的服务器，如果大量玩家不断进出，这些对象可能会越积越多）
		bDestroyDeactivatedPlayerState = true;
		break;
	default:
		bDestroyDeactivatedPlayerState = true;
		break;
	}

	SetPlayerConnectionType(ELyraPlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}
}

void ALyraPlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == ELyraPlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(ELyraPlayerConnectionType::Player);
	}
}

void ALyraPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogLyraTeams, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId ALyraPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLyraTeamIndexChangedDelegate* ALyraPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ALyraPlayerState::SetPlayerConnectionType(ELyraPlayerConnectionType NewType)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyPlayerConnectionType, this);
	MyPlayerConnectionType = NewType;
}

void ALyraPlayerState::SetSquadID(int32 NewSquadID)
{
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MySquadID, this);

		MySquadID = NewSquadID;
	}
}

void ALyraPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void ALyraPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 ALyraPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool ALyraPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void ALyraPlayerState::ClientBroadcastMessage_Implementation(const FLyraVerbMessage Message)
{
	// This check is needed to prevent running the action when in standalone mode
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

FRotator ALyraPlayerState::GetReplicatedViewRotation() const
{
	// Could replace this with custom replication
	return ReplicatedViewRotation;
}

void ALyraPlayerState::SetReplicatedViewRotation(const FRotator& NewRotation)
{
	if (NewRotation != ReplicatedViewRotation)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this);
		ReplicatedViewRotation = NewRotation;
	}
}

void ALyraPlayerState::OnRep_PawnData()
{
}

void ALyraPlayerState::OnExperienceLoaded(const ULyraExperienceDefinition* CurrentExperience)
{
	if (ALyraGameMode* LyraGameMode = GetWorld()->GetAuthGameMode<ALyraGameMode>())
	{
		if (const ULyraPawnData* NewPawnData = LyraGameMode->GetPawnDataForController(GetOwningController()))
		{
			SetPawnData(NewPawnData);
		}
		else
		{
			UE_LOG(LogLyra, Error, TEXT("ALyraPlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"),
			       *GetNameSafe(this));
		}
	}
}

void ALyraPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALyraPlayerState::OnRep_MySquadID()
{
	//@TODO: Let the squad subsystem know (once that exists)
}
