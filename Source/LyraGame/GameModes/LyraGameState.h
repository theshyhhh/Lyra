#pragma once

#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "LyraGameState.generated.h"

#define UE_API LYRAGAME_API

/**
 * AModularGameStateBase 负责把 GameStateBase 注册给 UGameFrameworkComponentManager，让 GameFeature 可以给 GameState 动态加组件、监听扩展事件。
 */
UCLASS(MinimalAPI, Config = Game)
class ALyraGameState : public AModularGameStateBase //, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UE_API ALyraGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
#undef UE_API
