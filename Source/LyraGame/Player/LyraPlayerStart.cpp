#include "LyraPlayerStart.h"

#include "GameFramework/GameModeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlayerStart)

ALyraPlayerStart::ALyraPlayerStart(const FObjectInitializer& Initializer) : Super(Initializer)
{
}

ELyraPlayerStartLocationOccupancy ALyraPlayerStart::GetLocationOccupancy(AController* const ControllerPawnToFit) const
{
	UWorld* const World = GetWorld();
	if (HasAuthority() && World)
	{
		if (AGameModeBase* AuthGameMode = World->GetAuthGameMode())
		{
			TSubclassOf<APawn> PawnClass = AuthGameMode->GetDefaultPawnClassForController(ControllerPawnToFit);
			const APawn* const PawnToFit = PawnClass ? GetDefault<APawn>(PawnClass) : nullptr;
			FVector ActorLocation = GetActorLocation();
			const FRotator ActorRotation = GetActorRotation();
			//如果 Actor 在 TestLocation 会侵占到某个会阻挡它的东西，则返回 true。
			if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
			{
				return ELyraPlayerStartLocationOccupancy::Empty;
			}
			/**
			 * 尝试在尽可能靠近 PlaceLocation 的位置，找到一个可接受的、无碰撞的位置来放置 TestActor。要求 PlaceLocation 是关卡内的有效位置。
		     * 如果找到了一个没有阻挡碰撞的位置，则返回 true，此时 PlaceLocation 会被覆盖为新的可用位置。
		     * 如果找不到合适的位置，则返回 false，此时 PlaceLocation 保持不变。
			 */
			else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				return ELyraPlayerStartLocationOccupancy::Partial;
			}
		}
	}
	return ELyraPlayerStartLocationOccupancy::Full;
}

bool ALyraPlayerStart::IsClaimed() const
{
	return ClaimingController != nullptr;
}

bool ALyraPlayerStart::TryClaim(AController* OccupyingController)
{
	if (OccupyingController != nullptr && !IsClaimed())
	{
		ClaimingController = OccupyingController;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ExpirationTimerHandle, FTimerDelegate::CreateUObject(this, &ALyraPlayerStart::CheckUnclaimed),
			                                  ExpirationCheckInterval, true);
		}
		return true;
	}
	return false;
}

void ALyraPlayerStart::CheckUnclaimed()
{
	if (ClaimingController != nullptr && ClaimingController->GetPawn() != nullptr && GetLocationOccupancy(ClaimingController) ==
		ELyraPlayerStartLocationOccupancy::Empty)
	{
		ClaimingController = nullptr;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpirationTimerHandle);
		}
	}
}
