#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "LyraPlatformEmulationSettings.generated.h"

/**
 * ULyraPlatformEmulationSettings 是 Lyra 的 Editor/PIE 平台模拟配置类，
 * 从而测试不同平台下的 UI 可见性、性能选项和帧率策略。
 */
UCLASS()
class ULyraPlatformEmulationSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	ULyraPlatformEmulationSettings();
	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface

	//获取当前模拟的基础设备配置文件
	FName GetPretendBaseDeviceProfile() const;

	//获取模拟的平台名称
	FName GetPretendPlatformName() const;

private:
	//在 Editor/PIE 中额外“启用”的一些平台特性
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation, meta=(Categories="Input,Platform.Trait"))
	FGameplayTagContainer AdditionalPlatformTraitsToEnable;

	//在 Editor/PIE 中额外“屏蔽”一些平台特性
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation, meta=(Categories="Input,Platform.Trait"))
	FGameplayTagContainer AdditionalPlatformTraitsToSuppress;

	//模拟的平台名称，GetOptions可将编辑器中对变量的更改改为下拉选项，选项由对应函数提供
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation, meta=(GetOptions=GetKnownPlatformIds))
	FName PretendPlatform;

	// 模拟从 ULyraSettingsLocal 应用的设备特定设备配置文件时，要假装正在使用的基础设备配置文件。
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation, meta=(GetOptions=GetKnownDeviceProfiles, EditCondition=bApplyDeviceProfilesInPIE))
	FName PretendBaseDeviceProfile;

	/**
	 * 是否在 PIE 中启用 PC 风格的帧率限制逻辑？
     * 帧率限制是引擎范围的设置，所以在编辑器中启用它并不总是理想的。
     * 你可能还需要禁用编辑器偏好设置中的“Use Less CPU when in Background”。
	 */
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation, meta=(ConsoleVariable="Lyra.Settings.ApplyFrameRateSettingsInPIE"))
	bool bApplyFrameRateSettingsInPIE = false;

	/**
	 * 是否在 PIE 中应用前端专用的性能选项？
     * 它们所驱动的大多数引擎性能/可扩展性设置都是全局的，所以如果一个 PIE 窗口处于前端界面，而另一个处于游戏内界面，那么其中一个会覆盖另一个，另一个就会被迫停留在这些设置上。
     * 默认 false，避免多 PIE 窗口互相覆盖全局性能设置
	 */
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation, meta=(ConsoleVariable="Lyra.Settings.ApplyFrontEndPerformanceOptionsInPIE"))
	bool bApplyFrontEndPerformanceOptionsInPIE = false;

	//是否在 PIE 中应用模拟平台 / Experience 驱动的 DeviceProfile
	UPROPERTY(EditAnywhere, config, Category=PlatformEmulation,
		meta=(InlineEditConditionToggle, ConsoleVariable="Lyra.Settings.ApplyDeviceProfilesInPIE"))
	bool bApplyDeviceProfilesInPIE = false;

#if WITH_EDITOR

public:
	// 由EditorEngine调用，用于在PIE启动时，弹出哪些配置被更改的提醒
	LYRAGAME_API void OnPlayInEditorStarted() const;

private:
	// 最新应用的模拟平台
	FName LastAppliedPretendPlatform;

private:
	void ApplySettings();

	//更改当前模拟的平台
	void ChangeActivePretendPlatform(FName NewPlatformName);
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;

	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
private:
	UFUNCTION()
	TArray<FName> GetKnownPlatformIds() const;

	UFUNCTION()
	TArray<FName> GetKnownDeviceProfiles() const;

	void PickReasonableBaseDeviceProfile();
};
