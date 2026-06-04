#pragma once

#include "CommonPlayerController.h"
#include "LyraPlayerController.generated.h"

#define UE_API LYRAGAME_API

class ALyraPlayerState;
class ALyraHUD;

UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "该项目使用的PlayerController基类"))
class ALyraPlayerController : public ACommonPlayerController //, public ILyraCameraAssistInterface, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	UE_API ALyraPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//获取玩家状态
	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerController")
	UE_API ALyraPlayerState* GetLyraPlayerState() const;

	//获取玩家HUD
	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerController")
	UE_API ALyraHUD* GetLyraHUD() const;
};

UCLASS()
class ALyraReplayPlayerController : public ALyraPlayerController
{
	GENERATED_BODY()
};

#undef UE_API
