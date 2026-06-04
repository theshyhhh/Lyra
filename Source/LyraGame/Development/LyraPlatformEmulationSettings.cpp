#include "LyraPlatformEmulationSettings.h"

#include "CommonUIVisibilitySubsystem.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Engine/PlatformSettingsManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPlatformEmulationSettings)

#define LOCTEXT_NAMESPACE "LyraCheats"

ULyraPlatformEmulationSettings::ULyraPlatformEmulationSettings()
{
}

FName ULyraPlatformEmulationSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

FName ULyraPlatformEmulationSettings::GetPretendBaseDeviceProfile() const
{
	return PretendBaseDeviceProfile;
}

FName ULyraPlatformEmulationSettings::GetPretendPlatformName() const
{
	return PretendPlatform;
}

#if WITH_EDITOR

void ULyraPlatformEmulationSettings::OnPlayInEditorStarted() const
{
	//PIE开启时通知哪些额外的平台特性被开启
	if (!AdditionalPlatformTraitsToEnable.IsEmpty())
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("PlatformTraitEnableActive", "Platform Trait Override\nEnabling {0}"),
			FText::AsCultureInvariant(AdditionalPlatformTraitsToEnable.ToStringSimple())
		));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	//PIE开启时通知哪些额外的平台特性被屏蔽
	if (!AdditionalPlatformTraitsToSuppress.IsEmpty())
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("PlatformTraitSuppressionActive", "Platform Trait Override\nSuppressing {0}"),
			FText::AsCultureInvariant(AdditionalPlatformTraitsToSuppress.ToStringSimple())
		));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	////PIE开启时通知当前模拟的平台
	if (PretendPlatform != NAME_None)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("PlatformOverrideActive", "Platform Override Active\nPretending to be {0}"),
			FText::FromName(PretendPlatform)
		));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void ULyraPlatformEmulationSettings::ApplySettings()
{
	UCommonUIVisibilitySubsystem::SetDebugVisibilityConditions(AdditionalPlatformTraitsToEnable, AdditionalPlatformTraitsToSuppress);

	//如果当前模拟平台还没有被应用，即PretendPlatform != LastAppliedPretendPlatform，就通知更该当前应用的平台
	if (GIsEditor && PretendPlatform != LastAppliedPretendPlatform)
	{
		ChangeActivePretendPlatform(PretendPlatform);
	}
	PickReasonableBaseDeviceProfile();
}

void ULyraPlatformEmulationSettings::ChangeActivePretendPlatform(FName NewPlatformName)
{
	LastAppliedPretendPlatform = NewPlatformName;
	//防御性赋值，PretendPlatform从Settings配置
	PretendPlatform = NewPlatformName;

	UPlatformSettingsManager::SetEditorSimulatedPlatform(PretendPlatform);
}

void ULyraPlatformEmulationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplySettings();
}

void ULyraPlatformEmulationSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	ApplySettings();
}

void ULyraPlatformEmulationSettings::PostInitProperties()
{
	Super::PostInitProperties();
	ApplySettings();
}

#endif

TArray<FName> ULyraPlatformEmulationSettings::GetKnownPlatformIds() const
{
	TArray<FName> Results;

#if WITH_EDITOR
	Results.Add(NAME_None);
	Results.Append(UPlatformSettingsManager::GetKnownAndEnablePlatformIniNames());
#endif

	return Results;
}

TArray<FName> ULyraPlatformEmulationSettings::GetKnownDeviceProfiles() const
{
	TArray<FName> Results;

#if WITH_EDITOR
	const UDeviceProfileManager& Manager = UDeviceProfileManager::Get();
	Results.Reserve(Manager.Profiles.Num() + 1);

	if (PretendPlatform == NAME_None)
	{
		Results.Add(NAME_None);
	}
	//如果当前模拟平台为空，则返回所有的平台配置文件，如果不为空，则仅返回当前平台配置文件
	for (const TObjectPtr<UDeviceProfile>& Profile : Manager.Profiles)
	{
		bool bIncludeEntry = true;
		if (PretendPlatform != NAME_None)
		{
			if (Profile->DeviceType != PretendPlatform.ToString())
			{
				bIncludeEntry = false;
			}
		}

		if (bIncludeEntry)
		{
			Results.Add(Profile->GetFName());
		}
	}
#endif
	return Results;
}

void ULyraPlatformEmulationSettings::PickReasonableBaseDeviceProfile()
{
	UDeviceProfileManager& Manager = UDeviceProfileManager::Get();
	//检查当前平台配置文件是否与当前模拟平台兼容，如果不兼容，则设为空
	if (UDeviceProfile* ProfilePtr = Manager.FindProfile(PretendBaseDeviceProfile.ToString(), /*bCreateOnFail=*/ false))
	{
		const bool bIsCompatible = (PretendPlatform == NAME_None) || (ProfilePtr->DeviceType == PretendPlatform.ToString());
		if (!bIsCompatible)
		{
			PretendBaseDeviceProfile = NAME_None;
		}
	}

	//如果当前模拟平台不为空，且没有配置文件，则为其设置合适的配置文件
	if ((PretendPlatform != NAME_None) && (PretendBaseDeviceProfile == NAME_None))
	{
		//选取名字最短的配置文件
		/**
		* 为什么选最短名字？因为 Lyra 用了一个简单启发式：
		* Android
		* Android_Low
		* Android_Medium
		* Android_High
		* 通常 base profile 名字更短，带质量后缀的变体名字更长。
		* 所以“最短名字”大概率是最基础的 profile。
		 */
		FName ShortestMatchingProfileName;
		const FString PretendPlatformStr = PretendPlatform.ToString();
		for (const TObjectPtr<UDeviceProfile>& Profile : Manager.Profiles)
		{
			if (Profile->DeviceType == PretendPlatformStr)
			{
				const FName TestName = Profile->GetFName();
				if ((ShortestMatchingProfileName == NAME_None) || (TestName.GetStringLength() < ShortestMatchingProfileName.GetStringLength()))
				{
					ShortestMatchingProfileName = TestName;
				}
			}
		}
		PretendBaseDeviceProfile = ShortestMatchingProfileName;
	}
}

#undef LOCTEXT_NAMESPACE
