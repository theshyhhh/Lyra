#pragma once

#include "GameFramework/SaveGame.h"
#include "LyraSettingsShared.generated.h"

class ULyraLocalPlayer;

UCLASS()
class LYRAGAME_API ULyraSettingsShared : public ULocalPlayerSaveGame
{
	GENERATED_BODY()

public:
	//负责在设置发生变更时进行广播
	DECLARE_EVENT_OneParam(ULyraSettingsShared, FOnSettingChangedEvent, ULyraSettingsShared* Settings);

	FOnSettingChangedEvent OnSettingChanged;

	/** 创建一个临时的设置对象；之后它会被从用户 Save Game 中加载的设置对象所替换。 */
	static ULyraSettingsShared* CreateTemporarySettings(const ULyraLocalPlayer* LocalPlayer);

	/** 同步加载一个设置对象；在用户登录之前不能调用此函数。 */
	static ULyraSettingsShared* LoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer);

	DECLARE_DELEGATE_OneParam(FOnSettingsLoadedEvent, ULyraSettingsShared* Settings);

	/** todo:临时占位，异步加载设置待完成 */
	static bool AsyncLoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer, FOnSettingsLoadedEvent Delegate);
};
