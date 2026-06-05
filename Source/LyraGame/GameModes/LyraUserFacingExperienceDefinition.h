#pragma once

#include "Engine/DataAsset.h"
#include "LyraUserFacingExperienceDefinition.generated.h"

class UCommonSession_HostSessionRequest;
/**
 * Lyra 用来描述“玩家在前端看到的一局游戏/Playlist”的 UPrimaryDataAsset，
 * 它把 UI 展示信息、地图、真正要加载的 ULyraExperienceDefinition、会话创建参数打包成一个可被 AssetManager 扫描和前端/服务器使用的数据资产。
 */
UCLASS(BlueprintType)
class ULyraUserFacingExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//要加载的地图 Primary Asset Id，必须是 Map 类型。
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="Map"))
	FPrimaryAssetId MapID;

	//真正要进入的 LyraExperienceDefinition。
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="LyraExperienceDefinition"))
	FPrimaryAssetId ExperienceID;

	//额外 URL Options，比如可扩展 NumBots、自定义规则开关等。会拼到 travel URL。
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TMap<FString, FString> ExtraArgs;

	//前端UI的主标题
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileTitle;

	//次级标题
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileSubTitle;

	//详细的描述
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileDescription;

	//前端模式图标
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TObjectPtr<UTexture2D> TileIcon;

	//加载进入某个指定 Experience，或从该 Experience 返回时，要显示的加载界面 Widget。
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=LoadingScreen)
	TSoftClassPtr<UUserWidget> LoadingScreenWidget;

	//如果为 true，则这是一个默认 Experience，应该用于快速游玩，并在 UI 中优先显示。
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bIsDefaultExperience = false;

	/** 如果为 true，它将显示在前端界面的 Experience 列表中 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bShowInFrontEnd = true;

	/** 如果为 true，将会录制本局游戏的回放。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bRecordReplay = false;

	/**该会话中的最大玩家数 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	int32 MaxPlayerCount = 16;

public:
	/** 创建一个请求对象，用于根据这些设置真正启动一个 Session。 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta = (WorldContext = "WorldContextObject"))
	UCommonSession_HostSessionRequest* CreateHostingRequest(const UObject* WorldContextObject) const;
};
