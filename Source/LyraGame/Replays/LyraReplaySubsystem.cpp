#include "LyraReplaySubsystem.h"

#include "CommonUISettings.h"
#include "ICommonUIModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraReplaySubsystem)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_ReplaySupport, "Platform.Trait.ReplaySupport");

ULyraReplaySubsystem::ULyraReplaySubsystem()
{
}

bool ULyraReplaySubsystem::DoesPlatformSupportReplays()
{
	if (ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(GetPlatformSupportTraitTag()))
	{
		return true;
	}
	return false;
}

FGameplayTag ULyraReplaySubsystem::GetPlatformSupportTraitTag()
{
	return TAG_Platform_Trait_ReplaySupport.GetTag();
}
