#include "LyraGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameState)

ALyraGameState::ALyraGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//开启Tick函数
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}
