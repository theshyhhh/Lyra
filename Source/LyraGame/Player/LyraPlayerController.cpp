#include "LyraPlayerController.h"

#include "LyraPlayerState.h"
#include "UI/LyraHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlayerController)

namespace Lyra::Input
{
	static int32 ShouldAlwaysPlayForceFeedBack = 0;
	static FAutoConsoleVariableRef CVarShouldAlwaysPlayForceFeedback(TEXT("LyraPC.ShouldAlwaysPlayForceFeedback"),
	                                                                 ShouldAlwaysPlayForceFeedBack,
	                                                                 TEXT("是否开启力反馈，即使上次输入设备不是手柄"));
}

ALyraPlayerController::ALyraPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

ALyraPlayerState* ALyraPlayerController::GetLyraPlayerState() const
{
	//允许为null，只在类型不正确时触发checked
	return CastChecked<ALyraPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

ALyraHUD* ALyraPlayerController::GetLyraHUD() const
{
	return CastChecked<ALyraHUD>(GetHUD(), ECastCheckedType::NullAllowed);
}
