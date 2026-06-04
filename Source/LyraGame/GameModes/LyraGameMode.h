#pragma once

#include "ModularGameMode.h"
#include "LyraGameMode.generated.h"

#define UE_API LYRAGAME_API

/**
 * ALyraGameMode 继承 AModularGameModeBase，核心是为了走 AGameModeBase 的轻量服务器规则入口，
 * 同时接入 Lyra 的 ModularGameplay / GameFeature / Experience 架构，
 * 而不是使用 AGameMode 那套传统 MatchState 驱动流程。
 * Base 决定有没有 Match State；
 * Modular 决定能不能被 Game Feature 组件化扩展。
 */
UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "该项目使用的基础GameMode类"))
class ALyraGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	UE_API ALyraGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
#undef UE_API
