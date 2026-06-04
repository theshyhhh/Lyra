#include "LyraDeveloperSettings.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraDeveloperSettings)
#define LOCTEXT_NAMESPACE "LyraCheats"

ULyraDeveloperSettings::ULyraDeveloperSettings()
{
}

FName ULyraDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR
void ULyraDeveloperSettings::OnPlayInEditorStarted() const
{
	if (ExperienceOverride.IsValid())
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("ExperienceOverrideActive", "Developer Settings Override\nExperience {0}"),
		                                     FText::FromName(ExperienceOverride.PrimaryAssetName)));
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void ULyraDeveloperSettings::ApplySettings()
{
}

void ULyraDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplySettings();
}

void ULyraDeveloperSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	ApplySettings();
}


void ULyraDeveloperSettings::PostInitProperties()
{
	//确保在变量绑定对应CVars时，该模块已经加载
	FModuleManager::Get().LoadModuleChecked("GameplayMessageRuntime");
	Super::PostInitProperties();
	ApplySettings();
}
#endif
#undef LOCTEXT_NAMESPACE
