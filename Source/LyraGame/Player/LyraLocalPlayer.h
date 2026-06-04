#pragma once

#include "CommonLocalPlayer.h"
#include "LyraLocalPlayer.generated.h"

#define UE_API LYRAGAME_API

UCLASS(MinimalAPI)
class ULyraLocalPlayer : public UCommonLocalPlayer //, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	UE_API ULyraLocalPlayer();
};
#undef UE_API
