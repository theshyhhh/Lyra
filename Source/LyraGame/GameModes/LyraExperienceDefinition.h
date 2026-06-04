#pragma once

#include "Engine/DataAsset.h"
#include "LyraExperienceDefinition.generated.h"

class ULyraExperienceActionSet;
class ULyraPawnData;
class UGameFeatureAction;

UCLASS(BlueprintType, Const)
class ULyraExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULyraExperienceDefinition();

	//~UObject interface
#if WITH_EDITOR

	/**
	 * 这个函数只在 #if WITH_EDITOR 下编译，所以它主要服务于编辑器、资产验证、Cook 前检查等流程。
	 * 在编辑器里检查这个ULyraExperienceDefinition中Actions是否配置有效，蓝图资产是否直接继承自该C++类
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;

#endif
	//~End of UObject interface

	//~UPrimaryDataAsset interface
#if WITH_EDITORONLY_DATA

	/**
	 * UpdateAssetBundleData 是给 Asset Manager 做资源登记的，确保 Actions 里间接引用的资源也能跟着 Experience 一起被正确识别、加载和打包。
	 */
	virtual void UpdateAssetBundleData() override;

#endif
	//~End of UPrimaryDataAsset interface

	//该玩法体验需要激活的GameFeature插件
	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	TArray<FString> GameFeaturesToEnable;

	/**
	 * 玩法功能的“安装与卸载动作”
	 * 它负责在 Game Feature 激活时把组件、技能、输入、UI、数据等接入当前游戏，在停用时把它们清理掉。
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category="Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	/** 生成玩家的默认类 */
	//@TODO: Make soft?
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TObjectPtr<const ULyraPawnData> DefaultPawnData;

	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TArray<TObjectPtr<ULyraExperienceActionSet>> ActionSets;
};
