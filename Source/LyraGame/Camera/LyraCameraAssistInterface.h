#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LyraCameraAssistInterface.generated.h"

UINTERFACE(BlueprintType)
class ULyraCameraAssistInterface : public UInterface
{
	GENERATED_BODY()
};

class LYRAGAME_API ILyraCameraAssistInterface
{
	GENERATED_BODY()

public:
	/**
	 * 获取允许摄像机穿透的 Actor 列表。适用于第三人称摄像机，
	 * 当跟随摄像机需要忽略一组视角目标、Pawn、载具等对象时非常有用。
     */
	virtual void GetIgnoredActorsForCameraPenetration(TArray<const AActor*>& OutActorsAllowPenetration) const
	{
	}

	/**
	 * 用于防止摄像机穿透的目标 Actor。通常情况下，它几乎总是 View Target；
     * 如果未实现该函数，则默认仍会如此。不过，有时 View Target 并不是你需要保持在画面中的根 Actor。
     */
	virtual TOptional<AActor*> GetCameraPreventPenetrationTarget() const
	{
		return TOptional<AActor*>();
	}

	/**
	 * 当摄像机穿透目标时调用。如果你希望在目标 Actor 发生重叠时将其隐藏，此函数会很有用。
	 */
	virtual void OnCameraPenetratingTarget()
	{
	}
};
