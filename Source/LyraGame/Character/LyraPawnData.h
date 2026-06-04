#pragma once

#include "Engine/DataAsset.h"
#include "LyraPawnData.generated.h"

#define UE_API LYRAGAME_API

/**
 * LyraPawnData 是 Lyra 角色初始化的核心配置资产，负责把“这个玩法里的玩家 Pawn 是什么样的”数据化。
 */
UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "Lyra Pawn Data", ShortTooltip = "用于定义Pawn的数据资产"))
class ULyraPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_API ULyraPawnData(const FObjectInitializer& ObjectInitializer);

	// 初始化该类pawn的默认类 (应该通常ALyraPawn或ALyraCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lyra|Pawn")
	TSubclassOf<APawn> PawnClass;
};
#undef UE_API
