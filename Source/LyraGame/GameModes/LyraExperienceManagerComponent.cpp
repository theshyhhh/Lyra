#include "LyraExperienceManagerComponent.h"

#include "GameFeaturesSubsystemSettings.h"
#include "LyraExperienceActionSet.h"
#include "LyraExperienceDefinition.h"
#include "LyraLogChannels.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraExperienceManagerComponent)


ULyraExperienceManagerComponent::ULyraExperienceManagerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsReplicated(true);
}

void ULyraExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULyraExperienceManagerComponent, CurrentExperience);
}

void ULyraExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool ULyraExperienceManagerComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (LoadState != ELyraExperienceLoadState::Loaded)
	{
		OutReason = TEXT("Experience still loading");
		return true;
	}
	else
	{
		return false;
	}
}

void ULyraExperienceManagerComponent::SetCurrentExperience(FPrimaryAssetId ExperienceId)
{
	//TODO:实现LyraAssetManager后改为LyraAssetManager
	UAssetManager& AssetManager = UAssetManager::Get();
	//获取传入的Experience路径
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperienceId);
	//TryLoad() 加载出来的是类，不是数据资产实例
	//拿到Experience类
	TSubclassOf<ULyraExperienceDefinition> AssetClass = Cast<UClass>(AssetPath.TryLoad());
	check(AssetClass)
	//获取类默认对象
	const ULyraExperienceDefinition* Experience = GetDefault<ULyraExperienceDefinition>(AssetClass);
	check(Experience)
	//确保加载时当前Experience为空
	check(CurrentExperience==nullptr)
	CurrentExperience = Experience;
	StartExperienceLoad();
}

void ULyraExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_HighPriority(FOnLyraExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}

void ULyraExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FOnLyraExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}

void ULyraExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_LowPriority(FOnLyraExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_LowPriority.Add(MoveTemp(Delegate));
	}
}

bool ULyraExperienceManagerComponent::IsExperienceLoaded() const
{
	return (LoadState == ELyraExperienceLoadState::Loaded) && (CurrentExperience != nullptr);
}

void ULyraExperienceManagerComponent::OnRep_CurrentExperience()
{
	//网络复制时加载Experience
	StartExperienceLoad();
}

void ULyraExperienceManagerComponent::StartExperienceLoad()
{
	check(CurrentExperience!=nullptr)
	check(LoadState==ELyraExperienceLoadState::Unloaded)
	UE_LOG(LogLyraExperience, Log, TEXT("EXPERIENCE:Start Experience Load(Current Experience:%s,%s)"),
	       *CurrentExperience->GetPrimaryAssetId().ToString(),
	       *GetClientServerContextString(this));

	LoadState = ELyraExperienceLoadState::Loading;

	UAssetManager& AssetManager = UAssetManager::Get();

	//需要按Bundle（可以叫捆绑包，就是分类，即按客户端、服务端加载或者UI、武器加载等等）加载的资产
	TSet<FPrimaryAssetId> BundleAssetList;

	//直接加载的资产(暂未使用)
	TSet<FSoftObjectPath> RawAssetList;

	BundleAssetList.Add(CurrentExperience->GetPrimaryAssetId());

	for (const TObjectPtr<ULyraExperienceActionSet> ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
		}
	}

	//要加载哪些Bundle
	TArray<FName> BundlesToLoad;
	//TODO:后面添加具体的Bundle规则后，需要改为对应的Bundle规则
	BundlesToLoad.Add(TEXT("Equipped"));
	//TODO:将这些客户端/服务器相关的东西集中到 LyraAssetManager 中。

	const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
	const bool bLoadClient = GIsEditor || (OwnerNetMode != NM_DedicatedServer);
	const bool bLoadServer = GIsEditor || (OwnerNetMode != NM_Client);

	//按需加载客户端捆绑包或服务端捆绑包
	if (bLoadClient)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
	}
	if (bLoadServer)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
	}
	//FStreamableHandle 用来跟踪异步加载状态。
	TSharedPtr<FStreamableHandle> BundleLoadHandle = nullptr;
	if (BundleAssetList.Num() > 0)
	{
		//异步加载 Experience 相关的 Asset Bundle。
		BundleLoadHandle = AssetManager.ChangeBundleStateForPrimaryAssets(
			BundleAssetList.Array(), //要处理的 Primary Assets，也就是 Experience 和 ActionSets。
			BundlesToLoad, //要加载的 Bundle。
			{}, //没有要移除的 Bundle。
			false, //不要求移除所有其他 Bundle。
			FStreamableDelegate(), //这里先不传完成回调，后面统一绑定。
			FStreamableManager::AsyncLoadHighPriority //高优先级异步加载。
		);
	}
	TSharedPtr<FStreamableHandle> RawLoadHandle = nullptr;
	if (RawAssetList.Num() > 0)
	{
		//异步加载普通的软引用资源
		RawLoadHandle = AssetManager.LoadAssetList(
			RawAssetList.Array(),
			FStreamableDelegate(),
			FStreamableManager::AsyncLoadHighPriority,
			TEXT("StartExperienceLoad")
		);
	}

	//合并两个异步加载操作统一管理
	TSharedPtr<FStreamableHandle> Handle = nullptr;
	if (BundleLoadHandle.IsValid() && RawLoadHandle.IsValid())
	{
		//创建一个组合 Handle，该 Handle 会等待其他 Handle 完成后再完成。只要该 Handle 处于激活状态，子 Handle 就会被作为硬引用持有。
		Handle = AssetManager.GetStreamableManager().CreateCombinedHandle({BundleLoadHandle, RawLoadHandle});
	}
	else
	{
		Handle = BundleLoadHandle.IsValid() ? BundleLoadHandle : RawLoadHandle;
	}
	//创建一个流式委托，用于绑定到资产加载完成时
	FStreamableDelegate OnAssetLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnExperienceLoadComplete);
	
	//如果当前没有资源可以加载或者资源已经加载完成，则直接执行这个委托
	if (!Handle.IsValid()||Handle->HasLoadCompleted())
	{
		OnAssetLoadedDelegate.ExecuteIfBound();
	}
	else
	{
		//其余情况则绑定委托等待执行
		//展示两种绑定方式
		Handle->BindCompleteDelegate(OnAssetLoadedDelegate);
		
		Handle->BindCancelDelegate(FStreamableDelegate::CreateLambda([OnAssetLoadedDelegate]()
		{
			OnAssetLoadedDelegate.ExecuteIfBound();
		}));
	}
	// 如果有预加载资源，就开始加载，但不绑定完成回调，也不影响 Experience 是否进入下一阶段。
	TSet<FPrimaryAssetId> PreloadAssetList;
	//@TODO: 这里打算以后放一些“提前加载，但不阻塞 Experience 启动”的资源。
	if (PreloadAssetList.Num() > 0)
	{
		AssetManager.ChangeBundleStateForPrimaryAssets(PreloadAssetList.Array(), BundlesToLoad, {});
	}
}

void ULyraExperienceManagerComponent::OnExperienceLoadComplete()
{
}
