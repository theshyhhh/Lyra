#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "LyraPlayerState.generated.h"

#define UE_API LYRAGAME_API

class ALyraPlayerController;

UCLASS(MinimalAPI, Config = Game)
class ALyraPlayerState : public AModularPlayerState //, public IAbilitySystemInterface //, public ILyraTeamAgentInterface

{
	GENERATED_BODY()

public:
	UE_API ALyraPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerState")
	UE_API ALyraPlayerController* GetLyraPlayerController() const;
};

#undef UE_API
