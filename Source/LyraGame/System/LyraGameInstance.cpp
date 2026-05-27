#include "LyraGameInstance.h"

#if UE_WITH_DTLS
#include "DTLSCertStore.h"
#include "DTLSHandlerComponent.h"
#include "Misc/FileHelper.h"
#endif // UE_WITH_DTLS

#include "CommonSessionSubsystem.h"
#include "LyraGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameInstance)

namespace Lyra
{
	static bool bTestEncryption = false;
	static FAutoConsoleVariableRef CVarLyraTestEncryption(
		TEXT("Lyra.TestEncryption"),
		bTestEncryption,
		TEXT("如果为 true，客户端会在请求加入服务器时发送一个加密 token，并尝试使用调试密钥对连接进行加密。这并不安全，仅用于演示目的。"),
		ECVF_Default);

#if UE_WITH_DTLS
	static bool bUseDTLSEncryption = false;
	static FAutoConsoleVariableRef CVarLyraUseDTLSEncryption(
		TEXT("Lyra.UseDTLSEncryption"),
		bUseDTLSEncryption,
		TEXT("如果使用 Lyra.TestEncryption 和 DTLS Packet Handler，则设置为 true。"),
		ECVF_Default);

	/* 用于在同一台设备上运行多个游戏实例时进行测试（桌面构建）。 */
	static bool bTestDTLSFingerprint = false;
	static FAutoConsoleVariableRef CVarLyraTestDTLSFingerprint(
		TEXT("Lyra.TestDTLSFingerprint"),
		bTestDTLSFingerprint,
		TEXT("如果为 true，并且正在使用 DTLS 加密，则会为每个连接生成唯一证书，并将指纹写入文件，以模拟通过在线服务传递该指纹。"),
		ECVF_Default);

#if !UE_BUILD_SHIPPING
	static FAutoConsoleCommandWithWorldAndArgs CmdGenerateDTLSCertificate(
		TEXT("GenerateDTLSCertificate"),
		TEXT("生成一个用于测试的 DTLS 自签名证书，并导出为 PEM。"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* InWorld)
		{
			if (InArgs.Num() == 1)
			{
				const FString& CertName = InArgs[0];

				FTimespan CertExpire = FTimespan::FromDays(365);
				TSharedPtr<FDTLSCertificate> Cert = FDTLSCertStore::Get().CreateCert(CertExpire, CertName);
				if (Cert.IsValid())
				{
					const FString CertPath = FPaths::ProjectContentDir() / TEXT("DTLS") / FPaths::MakeValidFileName(
						FString::Printf(TEXT("%s.pem"), *CertName));

					if (!Cert->ExportCertificate(CertPath))
					{
						UE_LOG(LogTemp, Error, TEXT("GenerateDTLSCertificate: Failed to export certificate."));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("GenerateDTLSCertificate: Failed to generate certificate."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("GenerateDTLSCertificate: Invalid argument(s)."));
			}
		}));
#endif // UE_BUILD_SHIPPING
#endif // UE_WITH_DTLS
};


ULyraGameInstance::ULyraGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

bool ULyraGameInstance::CanJoinRequestedSession() const
{
	// 临时的第一版实现：始终返回 true
	// 之后会扩展为检查玩家的状态
	if (!Super::CanJoinRequestedSession())
	{
		return false;
	}
	return true;
}

void ULyraGameInstance::HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege,
                                               ECommonUserOnlineContext OnlineContext)
{
	Super::HandlerUserInitialized(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);

	//TODO:等待实现自定义LocalPlayer

	// 如果加载成功，告诉LocalPlayer加载设置
	// if (bSuccess && ensure(UserInfo))
	// {
	// 	ULyraLocalPlayer* LocalPlayer = Cast<ULyraLocalPlayer>(GetLocalPlayerByIndex(UserInfo->LocalPlayerIndex));
	//
	// 	// There will not be a local player attached to the dedicated server user
	// 	if (LocalPlayer)
	// 	{
	// 		LocalPlayer->LoadSharedSettingsFromDisk();
	// 	}
	// }
}

void ULyraGameInstance::ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate)
{
	// 这是一个简单实现，用于演示如何使用硬编码密钥对游戏流量进行加密。
	// 对于完整实现，你通常会希望从安全来源获取加密密钥，
	// 例如通过 HTTPS 从 Web 服务获取。这个过程可以在该函数中完成，甚至可以异步完成——
	// 只需要在密钥确定后，调用传入的响应委托即可。EncryptionToken 的内容由用户决定，
	// 但它通常会包含用于生成唯一加密密钥的信息，例如用户 ID 和/或会话 ID。

	FEncryptionKeyResponse Response(EEncryptionResponse::Failure, TEXT("Unknown encryption failure"));

	if (EncryptionToken.IsEmpty())
	{
		Response.Response = EEncryptionResponse::InvalidToken;
		Response.ErrorMsg = TEXT("Encryption token is empty.");
	}
	else
	{
#if UE_WITH_DTLS
		if (Lyra::bUseDTLSEncryption)
		{
			TSharedPtr<FDTLSCertificate> Cert;

			if (Lyra::bTestDTLSFingerprint)
			{
				// 为该标识符生成服务器证书，并发布指纹
				FTimespan CertExpire = FTimespan::FromHours(4);
				Cert = FDTLSCertStore::Get().CreateCert(CertExpire, EncryptionToken);
			}
			else
			{
				// 出于测试目的从磁盘加载证书（绝不要在生产环境中这样做）
				const FString CertPath = FPaths::ProjectContentDir() / TEXT("DTLS") / TEXT("LyraTest.pem");

				Cert = FDTLSCertStore::Get().GetCert(EncryptionToken);

				if (!Cert.IsValid())
				{
					Cert = FDTLSCertStore::Get().ImportCert(CertPath, EncryptionToken);
				}
			}

			if (Cert.IsValid())
			{
				if (Lyra::bTestDTLSFingerprint)
				{
					// 指纹应发布到一个安全的 Web 服务中，以便发现/查询
					// 这里写入磁盘是为了本地测试
					TArrayView<const uint8> Fingerprint = Cert->GetFingerprint();

					FString DebugFile = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("DTLS")) / FPaths::MakeValidFileName(EncryptionToken) + TEXT(
						"_server.txt");

					FString FingerprintStr = BytesToHex(Fingerprint.GetData(), Fingerprint.Num());
					FFileHelper::SaveStringToFile(FingerprintStr, *DebugFile);
				}

				// 服务器当前只需要该标识符
				Response.EncryptionData.Identifier = EncryptionToken;
				Response.EncryptionData.Key = DebugTestEncryptionKey;

				Response.Response = EEncryptionResponse::Success;
			}
			else
			{
				Response.Response = EEncryptionResponse::Failure;
				Response.ErrorMsg = TEXT("Unable to obtain certificate.");
			}
		}
		else
#endif // UE_WITH_DTLS
		{
			Response.Response = EEncryptionResponse::Success;
			Response.EncryptionData.Key = DebugTestEncryptionKey;
		}
	}

	Delegate.ExecuteIfBound(Response);
}

void ULyraGameInstance::ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate)
{
	// 这是一个简单实现，用于演示如何使用硬编码密钥对游戏流量进行加密。
	// 对于完整实现，你通常会希望从安全来源获取加密密钥，
	// 例如通过 HTTPS 从 Web 服务获取。这个过程可以在该函数中完成，甚至可以异步完成——
	// 只需要在密钥确定后，调用传入的响应委托即可。

	FEncryptionKeyResponse Response;

#if UE_WITH_DTLS
	if (Lyra::bUseDTLSEncryption)
	{
		Response.Response = EEncryptionResponse::Failure;

		APlayerController* const PlayerController = GetFirstLocalPlayerController();

		if (PlayerController && PlayerController->PlayerState && PlayerController->PlayerState->GetUniqueId().IsValid())
		{
			const FUniqueNetIdRepl& PlayerUniqueId = PlayerController->PlayerState->GetUniqueId();

			// 理想情况下，Encryption Token 应该直接传入，而不是必须尝试重新构建它。
			const FString EncryptionToken = PlayerUniqueId.ToString();

			Response.EncryptionData.Identifier = EncryptionToken;

			// 服务器的指纹应该从安全服务中获取
			if (Lyra::bTestDTLSFingerprint)
			{
				// 但出于测试目的……
				FString DebugFile = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("DTLS")) / FPaths::MakeValidFileName(EncryptionToken) + TEXT(
					"_server.txt");
				FString FingerprintStr;
				FFileHelper::LoadFileToString(FingerprintStr, *DebugFile);

				Response.EncryptionData.Fingerprint.AddUninitialized(FingerprintStr.Len() / 2);
				HexToBytes(FingerprintStr, Response.EncryptionData.Fingerprint.GetData());
			}
			else
			{
				// 出于测试目的，从磁盘中读取预期的指纹；实际应从安全服务中获取
				const FString CertPath = FPaths::ProjectContentDir() / TEXT("DTLS") / TEXT("LyraTest.pem");

				TSharedPtr<FDTLSCertificate> Cert = FDTLSCertStore::Get().GetCert(EncryptionToken);
				if (!Cert.IsValid())
				{
					Cert = FDTLSCertStore::Get().ImportCert(CertPath, EncryptionToken);
				}

				if (Cert.IsValid())
				{
					TArrayView<const uint8> Fingerprint = Cert->GetFingerprint();

					Response.EncryptionData.Fingerprint = Fingerprint;
				}
				else
				{
					Response.Response = EEncryptionResponse::Failure;
					Response.ErrorMsg = TEXT("Unable to obtain certificate.");
				}
			}

			Response.EncryptionData.Key = DebugTestEncryptionKey;

			Response.Response = EEncryptionResponse::Success;
		}
	}
	else
#endif // UE_WITH_DTLS
	{
		Response.Response = EEncryptionResponse::Success;
		Response.EncryptionData.Key = DebugTestEncryptionKey;
	}

	Delegate.ExecuteIfBound(Response);
}

void ULyraGameInstance::Init()
{
	Super::Init();
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);
	if (ensure(ComponentManager))
	{
		//这套状态给 Lyra 的 Pawn、HeroComponent、PawnExtensionComponent、ASC 初始化 协作 用。
		//没有它，各组件就没有统一的“谁先准备好、谁后准备好”的顺序标准。
		ComponentManager->RegisterInitState(LyraGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(LyraGameplayTags::InitState_DataAvailable, false, LyraGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(LyraGameplayTags::InitState_DataInitialized, false, LyraGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(LyraGameplayTags::InitState_GameplayReady, false, LyraGameplayTags::InitState_DataInitialized);
	}
	//测试密钥，无法在正式开发中使用
	DebugTestEncryptionKey.SetNum(32);

	for (int32 i = 0; i < DebugTestEncryptionKey.Num(); ++i)
	{
		DebugTestEncryptionKey[i] = uint8(i);
	}

	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->OnPreClientTravelEvent.AddUObject(this, &ULyraGameInstance::OnPreClientTravelToSession);
	}
}

void ULyraGameInstance::Shutdown()
{
	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->OnPreClientTravelEvent.RemoveAll(this);
	}

	Super::Shutdown();
}

void ULyraGameInstance::OnPreClientTravelToSession(FString& URL)
{
	// 如果需要，则添加调试用的加密 Token。
	if (Lyra::bTestEncryption)
	{
#if UE_WITH_DTLS
		if (Lyra::bUseDTLSEncryption)
		{
			APlayerController* const PlayerController = GetFirstLocalPlayerController();

			if (PlayerController && PlayerController->PlayerState && PlayerController->PlayerState->GetUniqueId().IsValid())
			{
				const FUniqueNetIdRepl& PlayerUniqueId = PlayerController->PlayerState->GetUniqueId();
				const FString EncryptionToken = PlayerUniqueId.ToString();

				URL += TEXT("?EncryptionToken=") + EncryptionToken;
			}
		}
		else
#endif // UE_WITH_DTLS
		{
			// 这只是一个用于测试/调试的值；无论 token 的值是什么，服务器都会使用相同的密钥。
			// 但如果需要，token 可以是用户 ID 和/或会话 ID，用于为每个用户和/或每个会话生成唯一密钥。
			URL += TEXT("?EncryptionToken=1");
		}
	}
}
