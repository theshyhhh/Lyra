#pragma once

#include "CoreMinimal.h"
#include "LoadingProcessInterface.h"
#include "Components/GameStateComponent.h"
#include "LyraExperienceManagerComponent.generated.h"

#define UE_API LYRAGAME_API

namespace UE::GameFeatures
{
	struct FResult;
}

class ULyraExperienceDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLyraExperienceLoaded, const ULyraExperienceDefinition* /*Experience*/);

//Experience的加载阶段
enum class ELyraExperienceLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	LoadingChaosTestingDelay,
	ExecutingActions,
	Loaded,
	Deactivating
};

UCLASS(MinimalAPI)
class ULyraExperienceManagerComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:
	UE_API ULyraExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	UE_API virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface

	/**
	 * 通过FPrimaryAssetId尝试获取到要设置的Experience的类默认对象
	 * 将CurrentExperience设为获取到的对象触发该属性的复制
	 * 调用StartExperienceLoad真正开始加载
	 * @param ExperienceId 要设置的Experience的PrimaryAssetId
	 */
	UE_API void SetCurrentExperience(FPrimaryAssetId ExperienceId);

	/**
	 * 如果Experience加载完成，则直接调用传入的委托
	 * 如果没有，则等待加载完成调用
	 */
	UE_API void CallOrRegister_OnExperienceLoaded_HighPriority(FOnLyraExperienceLoaded::FDelegate&& Delegate);
	UE_API void CallOrRegister_OnExperienceLoaded(FOnLyraExperienceLoaded::FDelegate&& Delegate);
	UE_API void CallOrRegister_OnExperienceLoaded_LowPriority(FOnLyraExperienceLoaded::FDelegate&& Delegate);

	// 如果Experience完全加载完成则返回ture
	UE_API bool IsExperienceLoaded() const;

private:
	UFUNCTION()
	void OnRep_CurrentExperience();
	/**
	 * StartExperienceLoad() 负责 Experience 相关资产 Bundle 的加载
	 * 该方法首先检查CurrentExperience是否已设置且LoadState为Unloaded状态，然后更新LoadState为Loading。
	 * 接下来，根据当前Experience和其ActionSets生成需要加载的Bundle资产列表。根据网络模式决定是否加载客户端或服务器相关的Bundles。
	 * 使用AssetManager异步加载这些Bundle以及直接引用的资源（如果有的话）。创建一个组合Handle来管理所有加载操作，并绑定到加载完成时的回调函数OnExperienceLoadComplete。
	 * 如果有预加载资源需求，则开始加载这些资源但不阻塞Experience的加载过程。
	 */
	void StartExperienceLoad();

	void OnExperienceLoadComplete();

	//当前正在使用的Experience
	UPROPERTY(ReplicatedUsing=OnRep_CurrentExperience)
	TObjectPtr<const ULyraExperienceDefinition> CurrentExperience;

	//当前Experience的加载阶段
	ELyraExperienceLoadState LoadState = ELyraExperienceLoadState::Unloaded;

	//正在加载的GameFeature插件数量
	int32 NumGameFeaturePluginsLoading = 0;

	//GameFeaturePlugin插件的URL
	TArray<FString> GameFeaturePluginURLs;

	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;

	/**
	 * Experience加载完成后的委托，分三种优先级
	 */
	FOnLyraExperienceLoaded OnExperienceLoaded_HighPriority;
	FOnLyraExperienceLoaded OnExperienceLoaded;
	FOnLyraExperienceLoaded OnExperienceLoaded_LowPriority;
};
#undef UE_API
