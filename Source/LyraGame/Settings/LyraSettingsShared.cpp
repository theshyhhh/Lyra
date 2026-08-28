#include "LyraSettingsShared.h"
#include "Player/LyraLocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSettingsShared)

static FString SHARED_SETTINGS_SLOT_NAME = TEXT("SharedGameSettings");

ULyraSettingsShared* ULyraSettingsShared::CreateTemporarySettings(const ULyraLocalPlayer* LocalPlayer)
{
	// 该设置并不是从磁盘加载的，但应该将其配置为能够进行保存。
	ULyraSettingsShared* SharedSettings = Cast<ULyraSettingsShared>(
		CreateNewSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	// SharedSettings->ApplySettings();

	return SharedSettings;
}

ULyraSettingsShared* ULyraSettingsShared::LoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer)
{
	// 这会在加载期间阻塞主线程
	ULyraSettingsShared* SharedSettings = Cast<ULyraSettingsShared>(
		LoadOrCreateSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	//SharedSettings->ApplySettings();

	return SharedSettings;
}

bool ULyraSettingsShared::AsyncLoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer, FOnSettingsLoadedEvent Delegate)
{
	return false;
}
