#pragma once

#include "LyraCharacter.h"
#include "LyraCharacterWithAbilities.generated.h"

#define UE_API LYRAGAME_API

UCLASS(MinimalAPI, Blueprintable)
class ALyraCharacterWithAbilities : public ALyraCharacter
{
	GENERATED_BODY()

public:
	UE_API ALyraCharacterWithAbilities(const FObjectInitializer& ObjectInitializer);
};
#undef UE_API
