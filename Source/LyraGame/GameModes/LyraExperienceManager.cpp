#include "LyraExperienceManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraExperienceManager)

#if WITH_EDITOR
void ULyraExperienceManager::OnPlayInEditorBegun()
{
	ensure(GameFeaturePluginRequestCountMap.IsEmpty());
	GameFeaturePluginRequestCountMap.Empty();
}

void ULyraExperienceManager::NotifyOfPluginActivation(const FString PluginURL)
{
	if (GIsEditor)
	{
		ULyraExperienceManager* LyraExperienceManager = GEngine->GetEngineSubsystem<ULyraExperienceManager>();
		check(LyraExperienceManager)
		int32& Count = LyraExperienceManager->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		Count++;
	}
}

bool ULyraExperienceManager::RequestToDeactivatePlugin(const FString PluginURL)
{
	if (GIsEditor)
	{
		ULyraExperienceManager* LyraExperienceManager = GEngine->GetEngineSubsystem<ULyraExperienceManager>();
		check(LyraExperienceManager)
		int32& Count = LyraExperienceManager->GameFeaturePluginRequestCountMap.FindChecked(PluginURL);
		Count--;
		if (Count == 0)
		{
			LyraExperienceManager->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}
		return false;
	}
	return true;
}

/**
 * 初始化反初始化测试
 */

// void ULyraExperienceManager::Initialize(FSubsystemCollectionBase& Collection)
// {
// 	Super::Initialize(Collection);
// 	UE_LOG(LogLyra, Log, TEXT("========== ULyraExperienceManager Initialize =========="));
//
// 	UE_LOG(LogLyra, Log, TEXT("ULyraExperienceManager Class: %s"), *GetClass()->GetName());
// 	
// 	UObject* OuterObject = GetOuter();
// 	UE_LOG(LogLyra, Log, TEXT("Outer: %s"), OuterObject ? *OuterObject->GetName() : TEXT("None"));
//
// 	UE_LOG(LogLyra, Log, TEXT("GEngine Is Valid: %s"), GEngine ? TEXT("true") : TEXT("false"));
//
// 	UE_LOG(LogLyra, Log, TEXT("Command Line: %s"), FCommandLine::Get());
//
// #if UE_BUILD_SHIPPING
// 	UE_LOG(LogLyra, Log, TEXT("Build Config: Shipping"));
// #elif UE_BUILD_DEVELOPMENT
// 	UE_LOG(LogLyra, Log, TEXT("Build Config: Development"));
// #elif UE_BUILD_DEBUG
// 	UE_LOG(LogLyra, Log, TEXT("Build Config: Debug"));
// #else
// 	UE_LOG(LogLyra, Log, TEXT("Build Config: Other"));
// #endif
//
// #if WITH_EDITOR
// 	UE_LOG(LogLyra, Log, TEXT("WITH_EDITOR: true"));
// #else
// 	UE_LOG(LogLyra, Log, TEXT("WITH_EDITOR: false"));
// #endif
//
// 	UE_LOG(LogLyra, Log, TEXT("================================================="));
// }
//
// void ULyraExperienceManager::Deinitialize()
// {
// 	UE_LOG(LogLyra, Log, TEXT("========== MyEngineSubsystem Deinitialize =========="));
//
// 	Super::Deinitialize();
// }
#endif
