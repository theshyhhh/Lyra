#pragma once

#include "Engine/DataAsset.h"
#include "LyraExperienceActionSet.generated.h"


class UGameFeatureAction;
/**
 * ULyraExperienceActionSet 的作用是：把一组可复用的 Experience 动作和 GameFeature 依赖打包成一个数据资产，供多个 Experience 组合使用。
 * 它的设计意图是组合优先。
 * 比如多个玩法 Experience 都需要同一套 HUD、输入映射、基础 Ability 或通用 GameFeature 依赖，
 * 就不用每个 Experience 重复配置一遍，而是做成一个 LyraExperienceActionSet，然后让多个 Experience 引用它。
 */
UCLASS(BlueprintType, NotBlueprintable)
class ULyraExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULyraExperienceActionSet();

	//~UObject interface
#if WITH_EDITOR
	/**
	 * 验证Actions的有效性
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UPrimaryDataAsset interface
#if WITH_EDITORONLY_DATA
	/**
	 * 给 Asset Manager 做资源登记的，确保 Actions 里间接引用的资源也能跟着 Experience 一起被正确识别、加载和打包。
	 */
	virtual void UpdateAssetBundleData() override;
#endif
	//~End of UPrimaryDataAsset interface

	/**
	 * 玩法功能的“安装与卸载动作”
	 * 它负责在 Game Feature 激活时把组件、技能、输入、UI、数据等接入当前游戏，在停用时把它们清理掉。
	 */
	UPROPERTY(EditAnywhere, Instanced, Category="Actions to Perform")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	//该玩法体验需要激活的GameFeature插件
	UPROPERTY(EditAnywhere, Category="Feature Dependencies")
	TArray<FString> GameFeaturesToEnable;
};
