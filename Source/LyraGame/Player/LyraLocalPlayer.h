#pragma once

#include "CommonLocalPlayer.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "LyraLocalPlayer.generated.h"

#define UE_API LYRAGAME_API

struct FSwapAudioOutputResult;
class ULyraSettingsShared;
class ULyraSettingsLocal;

UCLASS(MinimalAPI)
class ULyraLocalPlayer : public UCommonLocalPlayer, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	UE_API ULyraLocalPlayer();

	//~UObject interface

	//C++ 构造函数和属性初始化完成之后调用，包括配置文件中的属性；调用时尚未发生序列化和其他设置。
	UE_API virtual void PostInitProperties() override;
	//~End of UObject interface

	//~UPlayer interface

	/**
	 * 更换当前本地玩家绑定的控制器
	 * @param PC 要分配给该 Player 的新 PlayerController。
	 */
	UE_API virtual void SwitchController(class APlayerController* PC) override;
	//~End of UPlayer interface

	//~ULocalPlayer interface

	//用来为该 ULocalPlayer 创建并关联 PlayerController，Lyra中手动调用OnPlayerControllerChanged，监听新的控制器的队伍变化委托
	UE_API virtual bool SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld) override;

	//在APlayerController::SetPlayer中玩家控制器关联LocalPlayer后调用，
	UE_API virtual void InitOnlineSession() override;
	//~End of ULocalPlayer interface

	//~ILyraTeamAgentInterface interface

	//什么也不做，我们只是观察与之关联的 PlayerController 所属的队伍。
	UE_API virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	//转发给拥有的玩家控制器获取队伍ID
	UE_API virtual FGenericTeamId GetGenericTeamId() const override;

	UE_API virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface


	/** 获取该玩家的本地设置。这些设置会在进程启动时从配置文件中读取，并且始终有效。*/
	UFUNCTION()
	UE_API ULyraSettingsLocal* GetLocalSettings() const;

	/** 创建一个或返回当前LocalPlayer持有的SharedSettings */
	UFUNCTION()
	UE_API ULyraSettingsShared* GetSharedSettings() const;

	/** 
	 * 启动一个异步请求来加载共享设置；加载完成或创建新的共享设置后，将调用 OnSharedSettingsLoaded。 
	 * 由ULyraGameInstance::HandlerUserInitialized调用，此时玩家已登录完成
	 * @param bForceLoad 是否强制加载
	 */
	UE_API void LoadSharedSettingsFromDisk(bool bForceLoad = false);

protected:
	//SharedSettings加载完成时调用的回调函数，用于更新当前LocalPlayer的SharedSettings和NetIdForSharedSettings
	UE_API void OnSharedSettingsLoaded(ULyraSettingsShared* LoadedOrCreatedSettings);
	
	//当玩家在设置中修改音频输出设备时触发该回调函数，它把新的设备 ID 提交给 UE 音频混合器（Audio Mixer），请求将游戏声音切换到该设备。
	UE_API void OnAudioOutputDeviceChanged(const FString& InAudioOutputDeviceId);

	//接收音频切换的结果，处理失败后的结果，当前待扩展
	UFUNCTION()
	UE_API void OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult);

	//当 ULyraLocalPlayer 关联的 PlayerController 发生创建或切换时，将队伍变化监听从旧 Controller 转移到新 Controller，并把队伍变化继续转发给监听 LocalPlayer 的系统。
	UE_API void OnPlayerControllerChanged(APlayerController* NewController);

	//绑定到控制器上队伍变化委托的回调函数，用于监听控制器队伍的变化，并广播LocalPlayer的队伍变化委托
	UFUNCTION()
	UE_API void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

private:
	//当前本地用户正在使用的那一份设置对象实例。Lyra 需要缓存、持续访问、保护其不被 GC，并在用户登录后将临时设置替换成真实存档设置。
	UPROPERTY(Transient)
	mutable TObjectPtr<ULyraSettingsShared> SharedSettings;

	//记录“当前这个 SharedSettings 是为哪个唯一网络账号（Unique Net ID）加载的”。
	FUniqueNetIdRepl NetIdForSharedSettings;

	//队伍变更委托
	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	//上一次绑定的玩家控制器
	UPROPERTY()
	TWeakObjectPtr<APlayerController> LastBoundPC;
};
#undef UE_API
