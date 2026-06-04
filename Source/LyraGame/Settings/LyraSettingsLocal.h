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
};
