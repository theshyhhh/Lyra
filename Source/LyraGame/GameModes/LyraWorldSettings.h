#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "LyraWorldSettings.generated.h"

#define UE_API LYRAGAME_API

class ULyraExperienceDefinition;

UCLASS(MinimalAPI)
class ALyraWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	UE_API ALyraWorldSettings(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	//编辑器地图检查函数，用于在 Map Check / 保存检查 / 编辑器校验时发现关卡配置问题。
	//这里是检查玩家出生点都是LyraPlayerStart
	UE_API virtual void CheckForErrors() override;
#endif

	/**
	 * @return 返回默认游戏体验资产(DefaultGameplayExperience)的PrimaryAssetId,如果ID无效，则日志输出报错
	 */
	UE_API FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	//如果服务器打开这个地图时没有被面向用户的 Experience 覆盖，就使用这个默认 Experience。
	UPROPERTY(EditDefaultsOnly, Category="GameMode")
	TSoftClassPtr<ULyraExperienceDefinition> DefaultGameplayExperience;

public:
#if WITH_EDITORONLY_DATA
	// 它主要用于 前端菜单、登录界面、主菜单、纯本地展示关卡 这类不应该以网络模式启动的地图。
	// 如果设置了这个选项，那么当你在编辑器中点击 Play 时，网络模式将被强制设为 Standalone。
	UPROPERTY(EditDefaultsOnly, Category="PIE")
	bool ForceStandaloneNetMode = false;

#endif
};


#undef UE_API
