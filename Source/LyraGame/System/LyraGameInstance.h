#pragma once

#include "CoreMinimal.h"
#include "CommonGameInstance.h"
#include "LyraGameInstance.generated.h"
#define UE_API LYRAGAME_API

enum class ECommonUserOnlineContext : uint8;

UCLASS(MinimalAPI, Config = Game)
class ULyraGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:
	UE_API ULyraGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//TODO: 等待自定义玩家控制器
	//UE_API ALyraPlayerController* GetPrimaryPlayerController() const;

	UE_API virtual bool CanJoinRequestedSession() const override;

	/**
	 *  Lyra 在 CommonUser 用户初始化完成后 的项目级回调。
	 *  它只做一件关键事：登录成功后，让对应的 ULyraLocalPlayer 加载这个用户的共享本地设置。
	 * @param UserInfo 初始化完成的用户信息。这里最重要的是 UserInfo->LocalPlayerIndex，用它找到对应的 LocalPlayer。
	 * @param bSuccess 用户初始化是否成功。只有成功才加载设置。
	 * @param Error 失败时的错误文本。
	 * @param RequestedPrivilege 这次初始化请求的权限，比如能否在线游戏等。
	 * @param OnlineContext 在线上下文。
	 */
	UE_API virtual void HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege,
	                                           ECommonUserOnlineContext OnlineContext) override;

	UE_API virtual void ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) override;

	UE_API virtual void ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate) override;

protected:
	UE_API virtual void Init() override;
	UE_API virtual void Shutdown() override;

	UE_API void OnPreClientTravelToSession(FString& URL);

	// 用于试用加密代码的硬编码加密密钥。
	// 这并不安全，请不要在生产环境中使用这种技术！
	TArray<uint8> DebugTestEncryptionKey;
};

#undef UE_API
