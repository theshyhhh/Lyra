#include "LyraLocalPlayer.h"

#include "AudioMixerBlueprintLibrary.h"
#include "Settings/LyraSettingsLocal.h"
#include "Settings/LyraSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLocalPlayer)

ULyraLocalPlayer::ULyraLocalPlayer()
{
}

void ULyraLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	if (ULyraSettingsLocal* LocalSettings = GetLocalSettings())
	{
		LocalSettings->OnAudioOutputDeviceChanged.AddUObject(this, &ULyraLocalPlayer::OnAudioOutputDeviceChanged);
	}
}

void ULyraLocalPlayer::SwitchController(class APlayerController* PC)
{
	Super::SwitchController(PC);
	OnPlayerControllerChanged(PlayerController);
}

bool ULyraLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	const bool bResult = Super::SpawnPlayActor(URL, OutError, InWorld);

	OnPlayerControllerChanged(PlayerController);

	return bResult;
}

void ULyraLocalPlayer::InitOnlineSession()
{
	OnPlayerControllerChanged(PlayerController);
	Super::InitOnlineSession();
}

void ULyraLocalPlayer::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
}

FGenericTeamId ULyraLocalPlayer::GetGenericTeamId() const
{
	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(PlayerController))
	{
		return ControllerAsTeamProvider->GetGenericTeamId();
	}
	else
	{
		return FGenericTeamId::NoTeam;
	}
}

FOnLyraTeamIndexChangedDelegate* ULyraLocalPlayer::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ULyraSettingsLocal* ULyraLocalPlayer::GetLocalSettings() const
{
	return ULyraSettingsLocal::Get();
}

ULyraSettingsShared* ULyraLocalPlayer::GetSharedSettings() const
{
	if (!SharedSettings)
	{
		// 在 PC 平台上使用同步加载是可以接受的，因为它只会检查磁盘。
		// 这里也可以改用 Platform Tag 来判断当前平台是否正确支持存档功能。
		//更精确的实现可以根据：
		//- 平台特性标签。
		//- 是否支持登录前本地存档。
		//- 是否必须等待平台用户登录。
		//- 是否使用云存档。
		// 来决定走哪个分支。
		// 当前 PLATFORM_DESKTOP判断是一种简化实现。
		bool bCanLoadBeforeLogin = PLATFORM_DESKTOP;

		if (bCanLoadBeforeLogin)
		{
			//尝试取得磁盘中真实对象
			SharedSettings = ULyraSettingsShared::LoadOrCreateSettings(this);
		}
		else
		{
			// 等待用户登录，所以此时需要一个临时的设置
			SharedSettings = ULyraSettingsShared::CreateTemporarySettings(this);
		}
	}
	return SharedSettings;
}

void ULyraLocalPlayer::LoadSharedSettingsFromDisk(bool bForceLoad)
{
	//获取该玩家之前缓存的唯一网络 ID（Unique Net ID）。
	FUniqueNetIdRepl CurrentNetId = GetCachedUniqueNetId();
	if (!bForceLoad && SharedSettings && CurrentNetId == NetIdForSharedSettings)
	{
		// 已经加载过一次，不再重复加载
		return;
	}

	//异步加载SharedSetting，将回调函数OnSharedSettingsLoaded绑定到委托
	ensure(
		ULyraSettingsShared::AsyncLoadOrCreateSettings(this,
			ULyraSettingsShared::FOnSettingsLoadedEvent::CreateUObject(this, &ULyraLocalPlayer::OnSharedSettingsLoaded)));
}

void ULyraLocalPlayer::OnSharedSettingsLoaded(ULyraSettingsShared* LoadedOrCreatedSettings)
{
	// 这些设置在执行到这里之前就已经应用了。
	if (ensure(LoadedOrCreatedSettings))
	{
		// 这会替换临时对象或之前已加载的对象，而被替换的对象之后会按正常流程被 GC（垃圾回收）掉。
		SharedSettings = LoadedOrCreatedSettings;
		//更新唯一网络ID
		NetIdForSharedSettings = GetCachedUniqueNetId();
	}
}

void ULyraLocalPlayer::OnAudioOutputDeviceChanged(const FString& InAudioOutputDeviceId)
{
	FOnCompletedDeviceSwap DevicesSwappedCallback;
	DevicesSwappedCallback.BindUFunction(this, FName("OnCompletedAudioDeviceSwap"));
	UAudioMixerBlueprintLibrary::SwapAudioOutputDevice(GetWorld(), InAudioOutputDeviceId, DevicesSwappedCallback);
}

void ULyraLocalPlayer::OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult)
{
	if (SwapResult.Result == ESwapAudioOutputDeviceResultState::Failure)
	{
		//音频设备切换失败路径，待扩展
	}
}

void ULyraLocalPlayer::OnPlayerControllerChanged(APlayerController* NewController)
{
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(LastBoundPC.Get()))
	{
		//获取上一个控制器的队伍ID
		OldTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		//解绑旧控制器的委托
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(NewController))
	{
		NewTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		//绑定新控制器的委托
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
		//更新上一个控制器
		LastBoundPC = NewController;
	}

	//广播队伍更换
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
}

void ULyraLocalPlayer::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}
