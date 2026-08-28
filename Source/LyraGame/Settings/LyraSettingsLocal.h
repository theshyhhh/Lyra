#pragma once

#include "GameFramework/GameUserSettings.h"
#include "LyraSettingsLocal.generated.h"

UCLASS()
class LYRAGAME_API ULyraSettingsLocal : public UGameUserSettings
{
	GENERATED_BODY()

public:
	ULyraSettingsLocal();

	static ULyraSettingsLocal* Get();

	//声明一个拥有 1 个参数的原生多播 Event 类型；底层属于 TMulticastDelegate 体系，但它在 API 设计上进一步表达了“外部负责监听、Owner 负责触发”的所有权语义。
	DECLARE_EVENT_OneParam(ULyraSettingsLocall, FAudioDeviceChanged, const FString& /*DeviceId*/)

	//音频设备发生改变时的广播的委托
	FAudioDeviceChanged OnAudioOutputDeviceChanged;

	void OnExperienceLoaded();

	UFUNCTION()
	bool ShouldAutoRecordReplays() const { return false; }
};
