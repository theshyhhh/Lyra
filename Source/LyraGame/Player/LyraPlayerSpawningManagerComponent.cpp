#include "LyraPlayerSpawningManagerComponent.h"

#include "EngineUtils.h"
#include "LyraPlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlayerSpawningManagerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogPlayerSpawning, Log, All);

ULyraPlayerSpawningManagerComponent::ULyraPlayerSpawningManagerComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
	SetIsReplicatedByDefault(false);
	bAutoRegister = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	//定义此 Tick 函数的最小 Tick Group。这些 Group 决定了在一帧更新期间对象执行 Tick 的相对顺序。如果存在前置依赖，该 Tick 可能会被延后执行。
	//任何需要在物理模拟开始前执行的项。
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


void ULyraPlayerSpawningManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//当通过 UWorld::AddToWorld 将 ULevel 添加到 World 时调用该委托。
	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ULyraPlayerSpawningManagerComponent::OnLevelAdded);

	UWorld* World = GetWorld();
	//当Actor动态生成时调用该委托
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ULyraPlayerSpawningManagerComponent::HandleOnActorSpawned));

	//遍历当前世界下的ALyraPlayerStart并添加到缓存它的数组中
	for (TActorIterator<ALyraPlayerStart> It(World); It; ++It)
	{
		if (ALyraPlayerStart* LyraPlayerStart = *It)
		{
			CachedPlayerStarts.Add(LyraPlayerStart);
		}
	}
}

void ULyraPlayerSpawningManagerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

APlayerStart* ULyraPlayerSpawningManagerComponent::GetFirstRandomUnoccupiedPlayerStart(AController* Controller,
                                                                                       const TArray<ALyraPlayerStart*>& FoundStartPoints) const
{
	if (Controller)
	{
		TArray<ALyraPlayerStart*> UnoccupiedStartPoints;
		TArray<ALyraPlayerStart*> OccupiedStartPoints;
		for (ALyraPlayerStart* StartPoint : FoundStartPoints)
		{
			ELyraPlayerStartLocationOccupancy State = StartPoint->GetLocationOccupancy(Controller);
			switch (State)
			{
			case ELyraPlayerStartLocationOccupancy::Empty:
				UnoccupiedStartPoints.Add(StartPoint);
				break;
			case ELyraPlayerStartLocationOccupancy::Partial:
				OccupiedStartPoints.Add(StartPoint);
				break;
			}
		}
		//首选完全空闲的
		if (UnoccupiedStartPoints.Num() > 0)
		{
			return UnoccupiedStartPoints[FMath::RandRange(0, UnoccupiedStartPoints.Num() - 1)];
		}
		//次选挤一挤能放下的
		else if (OccupiedStartPoints.Num() > 0)
		{
			return OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
		
	}
	return nullptr;
}

AActor* ULyraPlayerSpawningManagerComponent::ChoosePlayerStart(AController* Player)
{
	if (Player)
	{
#if WITH_EDITOR
		if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
		{
			return PlayerStart;
		}
#endif
		//检查一遍CachedPlayerStarts中是否都是有效的ALyraPlayerStart，因为出生点有可能被摧毁
		TArray<ALyraPlayerStart*> StartPoints;
		for (auto StartIt = CachedPlayerStarts.CreateIterator(); StartIt; ++StartIt)
		{
			if (ALyraPlayerStart* LyraPlayerStart = StartIt->Get())
			{
				StartPoints.Add(LyraPlayerStart);
			}
			else
			{
				StartIt.RemoveCurrent();
			}
		}
		//如果是观察者，则随便选择一个出生点，且不占用
		if (APlayerState* PlayerState = Player->GetPlayerState<APlayerState>())
		{
			if (PlayerState->IsOnlyASpectator())
			{
				if (!StartPoints.IsEmpty())
				{
					return StartPoints[FMath::RandRange(0, StartPoints.Num() - 1)];
				}
				return nullptr;
			}
		}
		AActor* PlayerStart = OnChoosePlayerStart(Player, StartPoints);

		if (!PlayerStart)
		{
			PlayerStart = GetFirstRandomUnoccupiedPlayerStart(Player, StartPoints);
		}

		if (ALyraPlayerStart* LyraStart = Cast<ALyraPlayerStart>(PlayerStart))
		{
			LyraStart->TryClaim(Player);
		}

		return PlayerStart;

	}
	return nullptr;
}

bool ULyraPlayerSpawningManagerComponent::ControllerCanRestart(AController* Player)
{
	bool bCanRestart = true;

	// TODO Can they restart?

	return bCanRestart;
}

void ULyraPlayerSpawningManagerComponent::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(NewPlayer, StartRotation);
	K2_OnFinishRestartPlayer(NewPlayer, StartRotation);
}

void ULyraPlayerSpawningManagerComponent::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld == GetWorld())
	{
		for (AActor* Actor : InLevel->Actors)
		{
			if (ALyraPlayerStart* LyraPlayerStart = Cast<ALyraPlayerStart>(Actor))
			{
				ensure(!CachedPlayerStarts.Contains(LyraPlayerStart));
				CachedPlayerStarts.Add(LyraPlayerStart);
			}
		}
	}
}

void ULyraPlayerSpawningManagerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	if (ALyraPlayerStart* LyraPlayerStart = Cast<ALyraPlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.Add(LyraPlayerStart);
	}
}
#if WITH_EDITOR
APlayerStart* ULyraPlayerSpawningManagerComponent::FindPlayFromHereStart(AController* Player)
{
	//“Play From Here” 只适用于 PlayerController，机器人等其他对象都应该从普通出生点生成。
	if (Player->IsA<APlayerController>())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				if (APlayerStart* PlayerStart = *It)
				{
					if (PlayerStart->IsA<APlayerStartPIE>())
					{
						return PlayerStart;
					}
				}
			}
		}
	}
	return nullptr;
}
#endif
