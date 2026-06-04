#include "LyraPlayerState.h"

#include "LyraPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlayerState)

ALyraPlayerState::ALyraPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

ALyraPlayerController* ALyraPlayerState::GetLyraPlayerController() const
{
	return Cast<ALyraPlayerController>(GetOwner());
}
