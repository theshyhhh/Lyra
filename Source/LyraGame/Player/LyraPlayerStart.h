#pragma once

#include "GameplayTagContainer.h"
#include "GameFramework/PlayerStart.h"
#include "LyraPlayerStart.generated.h"

#define UE_API LYRAGAME_API

//玩家出生点占用状态
enum class ELyraPlayerStartLocationOccupancy
{
	Empty, //空闲
	Partial, //部分占用
	Full //完全被占用
};

UCLASS(MinimalAPI, Config = Game)
class ALyraPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UE_API ALyraPlayerStart(const FObjectInitializer& Initializer = FObjectInitializer::Get());

	//获取当前出生点的被占用状态，即是否可以放下一个玩家Pawn
	UE_API ELyraPlayerStartLocationOccupancy GetLocationOccupancy(AController* const ControllerPawnToFit) const;

	/**
	 * @return 返回当前出生点是否已经被占用
	 */
	UE_API bool IsClaimed() const;

	/**
	 * 尝试占用当前出生点
	 * @param OccupyingController 要占用当前出生点的控制器
	 * @return 尝试占用是否成功
	 */
	UE_API bool TryClaim(AController* OccupyingController);

protected:
	/**
	 * 检查是否要解除占用状态
	 */
	UE_API void CheckUnclaimed();

	//占用当前出生点的控制器
	UPROPERTY(Transient)
	TObjectPtr<AController> ClaimingController = nullptr;

	//检查解除占用状态的时间间隔
	UPROPERTY(EditDefaultsOnly, Category="Player Start Claiming")
	float ExpirationCheckInterval = 1.f;

	//代表该出生点的Tag
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer StartPointTags;

	//检查是否要解除占用状态的Timer
	FTimerHandle ExpirationTimerHandle;
};

#undef UE_API
