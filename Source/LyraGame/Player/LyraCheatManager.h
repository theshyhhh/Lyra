#pragma once
#include "GameFramework/CheatManager.h"
#include "Logging/LogMacros.h"
#include "LyraCheatManager.generated.h"

#ifndef USING_CHEAT_MANAGER
#define USING_CHEAT_MANAGER (1 && !UE_BUILD_SHIPPING)
#endif // #ifndef USING_CHEAT_MANAGER

DECLARE_LOG_CATEGORY_EXTERN(LogLyraCheat, Log, All);

UCLASS(config = Game, Within = PlayerController, MinimalAPI)
class ULyraCheatManager : public UCheatManager
{
	GENERATED_BODY()
};
