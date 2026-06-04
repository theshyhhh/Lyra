#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "ModularCharacter.h"
#include "LyraCharacter.generated.h"

#define UE_API LYRAGAME_API

class ALyraPlayerState;
class ALyraPlayerController;

UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class ALyraCharacter : public AModularCharacter //, public IAbilitySystemInterface, public IGameplayCueInterface, public IGameplayTagAssetInterface
	//, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	UE_API ALyraCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra|Character")
	UE_API ALyraPlayerController* GetLyraPlayerController() const;

	UFUNCTION(BlueprintCallable, Category="Lyra|Character")
	UE_API ALyraPlayerState* GetLyraPlayerState() const;
};

#undef UE_API
