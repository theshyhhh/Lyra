#pragma once

#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LyraReplaySubsystem.generated.h"

#define UE_API LYRAGAME_API
/**
 * 
 */
UCLASS(MinimalAPI)
class ULyraReplaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UE_API ULyraReplaySubsystem();

	/** 返回平台是否支持回放系统 */
	UFUNCTION(BlueprintCallable, Category = Replays, BlueprintPure = false)
	static UE_API bool DoesPlatformSupportReplays();

	/** 返回平台支持的特性标签 */
	static UE_API FGameplayTag GetPlatformSupportTraitTag();
};
#undef UE_API
