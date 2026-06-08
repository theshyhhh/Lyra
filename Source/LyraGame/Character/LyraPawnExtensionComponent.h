#pragma once

#include "Components/PawnComponent.h"
#include "LyraPawnExtensionComponent.generated.h"

#define UE_API LYRAGAME_API

UCLASS(MinimalAPI)
class ULyraPawnExtensionComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UE_API ULyraPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);
};
#undef UE_API
