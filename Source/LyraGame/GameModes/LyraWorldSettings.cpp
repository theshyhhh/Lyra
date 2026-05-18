#include "LyraWorldSettings.h"

#include "EngineUtils.h"
#include "LyraLogChannels.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWorldSettings)

ALyraWorldSettings::ALyraWorldSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void ALyraWorldSettings::CheckForErrors()
{
	Super::CheckForErrors();
	FMessageLog MapCheck("MapCheck");
	for (TActorIterator<APlayerStart> PlayerStartIt(GetWorld()); PlayerStartIt; ++PlayerStartIt)
	{
		APlayerStart* PlayerStart = *PlayerStartIt;
		if (IsValid(PlayerStart) && PlayerStart->GetClass() == APlayerStart::StaticClass())
		{
			MapCheck.Warning()->AddToken(FUObjectToken::Create(PlayerStart))->AddToken(
				FTextToken::Create(FText::FromString("is a normal APlayerStart, replace with ALyraPlayerStart.")));
		}
	}
	//@TODO:确保这个软对象路径确实能够被转换成一个主资源 ID（例如，它不是指向某个未被扫描目录中的 Experience）
}
#endif


FPrimaryAssetId ALyraWorldSettings::GetDefaultGameplayExperience() const
{
	FPrimaryAssetId Result;
	if (!DefaultGameplayExperience.IsNull())
	{
		Result = UAssetManager::Get().GetPrimaryAssetIdForPath(DefaultGameplayExperience.ToSoftObjectPath());
		if (!Result.IsValid())
		{
			UE_LOG(LogLyraExperience, Error,
			       TEXT(
				       "%s.DefaultGameplayExperience is %s but that failed to resolve into an asset ID (you might need to add a path to the Asset Rules in your game feature plugin or project settings"
			       ),
			       *GetPathNameSafe(this), *DefaultGameplayExperience.ToString());
		}
	}
	return Result;
}
