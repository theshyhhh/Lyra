#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LyraGameData.generated.h"
#define UE_API LYRAGAME_API

class UGameplayEffect;

UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "Lyra Game Data", ShortTooltip = "包含全局数据资产的游戏资产."))
class ULyraGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_API ULyraGameData();

	static UE_API const ULyraGameData& Get();


	UPROPERTY(EditDefaultsOnly, Category="Default Gameplay Effects", meta=(DisplayName="Damage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	UPROPERTY(EditDefaultsOnly, Category="Default Gameplay Effects", meta=(DisplayName="Heal Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;

	// 用于添加和移除动态标签的 Gameplay Effect。
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;
};
#undef UE_API
