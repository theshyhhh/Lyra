#include "LyraExperienceManagerComponent.h"

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeaturesSubsystemSettings.h"
#include "LyraExperienceActionSet.h"
#include "LyraExperienceDefinition.h"
#include "LyraExperienceManager.h"
#include "LyraLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Settings/LyraSettingsLocal.h"
#include "System/LyraAssetManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraExperienceManagerComponent)

//@TODO: 异步加载 Experience Definition 本身
//@TODO: 显式处理失败情况（进入“已完成但失败”的状态，而不是使用 check()）
//@TODO: 在合适的时机执行各个 Action 阶段，而不是一次性全部执行
//@TODO: 支持停用一个 Experience，并执行卸载 Action
//@TODO: 思考对于预加载资源来说，停用/清理意味着什么
//@TODO: 处理 Game Feature 的停用；目前我们会“泄漏”它们，使其一直保持启用状态
//（对于从一个 Experience 切换到另一个 Experience 的客户端，我们实际上希望对需求做差异比较，只卸载其中一部分，而不是全部卸载后又马上重新加载）
//@TODO: 同时处理内置插件和基于 URL 的插件（搜索冒号？）

namespace LyraConsoleVariables
{
	static float ExperienceLoadRandomDelayMin = 0.f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayMin(
		TEXT("lyra.chaos.ExperienceDelayLoad.MinSecs"),
		ExperienceLoadRandomDelayMin,
		TEXT("该值作为Experience加载完成时的固定延迟（总延迟=该延迟+随机延迟）"),
		ECVF_Default);

	static float ExperienceLoadRadomDelayRange = 0.f;
	static FAutoConsoleVariableRef CVarExperienceLoadRadomDelayRange(
		TEXT("lyra.chaos.ExperienceDelayLoad.RandomSecs"),
		ExperienceLoadRadomDelayRange,
		TEXT("该值作为Experience加载完成时的固定延迟（总延迟=该延迟+最小延迟）"),
		ECVF_Default);

	float GetExperienceLoadDelayDuration()
	{
		return FMath::Max(0.f, ExperienceLoadRandomDelayMin + FMath::FRand() * ExperienceLoadRadomDelayRange);
	}
}


ULyraExperienceManagerComponent::ULyraExperienceManagerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void ULyraExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULyraExperienceManagerComponent, CurrentExperience);
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
	ULyraAssetManager& AssetManager = ULyraAssetManager::Get();
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

const ULyraExperienceDefinition* ULyraExperienceManagerComponent::GetCurrentExperienceChecked() const
{
	check(LoadState == ELyraExperienceLoadState::Loaded);
	check(CurrentExperience != nullptr);
	return CurrentExperience;
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

	ULyraAssetManager& AssetManager = ULyraAssetManager::Get();

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
	BundlesToLoad.Add(FLyraBundles::Equipped);

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
	if (!Handle.IsValid() || Handle->HasLoadCompleted())
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
	check(LoadState==ELyraExperienceLoadState::Loading)
	check(CurrentExperience!=nullptr)
	UE_LOG(LogLyraExperience, Log, TEXT("EXPERIENCE: OnExperienceLoadComplete(CurrentExperience = %s, %s)"),
	       *CurrentExperience->GetPrimaryAssetId().ToString(),
	       *GetClientServerContextString(this));
	//清空之前保存的插件URL列表
	GameFeaturePluginURLs.Reset();
	//定义一个局部 lambda，用来复用“插件名转 URL”的逻辑。Context表示当前这些插件名来自哪个数据资产,主要用于报错时打印来源。
	auto CollectGameFeaturePluginURLs = [This=this](const UPrimaryDataAsset* Context, const TArray<FString>& FeaturePluginList)
	{
		for (const FString& PluginName : FeaturePluginList)
		{
			FString PluginURL;
			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, PluginURL))
			{
				This->GameFeaturePluginURLs.AddUnique(PluginURL);
			}
			else
			{
				ensureMsgf(
					false,
					TEXT("OnExperienceLoadComplete failed to find plugin URL from PluginName %s for experience %s - fix data, ignoring for this run"),
					*PluginName, *Context->GetPrimaryAssetId().ToString());
			}
		}
		// 		// Add in our extra plugin
		// 		if (!CurrentPlaylistData->GameFeaturePluginToActivateUntilDownloadedContentIsPresent.IsEmpty())
		// 		{
		// 			FString PluginURL;
		// 			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(CurrentPlaylistData->GameFeaturePluginToActivateUntilDownloadedContentIsPresent, PluginURL))
		// 			{
		// 				GameFeaturePluginURLs.AddUnique(PluginURL);
		// 			}
		// 		}
	};

	CollectGameFeaturePluginURLs(CurrentExperience, CurrentExperience->GameFeaturesToEnable);

	for (const TObjectPtr<ULyraExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
		}
	}
	NumGameFeaturePluginsLoading = GameFeaturePluginURLs.Num();

	if (NumGameFeaturePluginsLoading > 0)
	{
		LoadState = ELyraExperienceLoadState::LoadingGameFeatures;
		for (const FString& PluginURL : GameFeaturePluginURLs)
		{
			ULyraExperienceManager::NotifyOfPluginActivation(PluginURL);
			UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL,
			                                                               FGameFeaturePluginLoadComplete::CreateUObject(
				                                                               this, &ThisClass::OnGameFeaturePluginLoadComplete));
		}
	}
	else
	{
		OnExperienceFullLoadCompleted();
	}
}

void ULyraExperienceManagerComponent::OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result)
{
	NumGameFeaturePluginsLoading--;
	if (NumGameFeaturePluginsLoading == 0)
	{
		OnExperienceFullLoadCompleted();
	}
}

void ULyraExperienceManagerComponent::OnExperienceFullLoadCompleted()
{
	check(LoadState!=ELyraExperienceLoadState::Loaded)

	//进行一段随机的延迟测试来模拟延迟环境，测试初始化流程和加载屏是否健壮
	if (LoadState != ELyraExperienceLoadState::LoadingChaosTestingDelay)
	{
		const float DelaySecs = LyraConsoleVariables::GetExperienceLoadDelayDuration();

		if (DelaySecs > 0.f)
		{
			FTimerHandle DummyHandle;
			LoadState = ELyraExperienceLoadState::LoadingChaosTestingDelay;
			GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &ThisClass::OnExperienceFullLoadCompleted, DelaySecs, false);
			return;
		}
	}
	LoadState = ELyraExperienceLoadState::ExecutingActions;

	//创建 GameFeature Action 激活上下文。
	//后面每个 Action 执行时都会用到这个 Context。
	FGameFeatureActivatingContext Context;

	//从当前 World 找到对应的 FWorldContext。
	//Lyra 需要知道这些 Action 应该作用在哪个 World 上，尤其是 PIE 多窗口、多客户端、多服务端时非常重要。
	const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());

	if (ExistingWorldContext)
	{
		//这样某些 GameFeatureAction 在激活时可以限制自己只影响当前 World，而不是误作用到其他 PIE 世界。
		Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}
	auto ActivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
	{
		for (UGameFeatureAction* Action : ActionList)
		{
			//@TODO: 这些函数不接收 World 这一点，在客户端-服务器 PIE 中可能存在问题
			// 当前行为与 Gameplay Tags 这类系统类似：加载和注册会作用于整个进程，
			// 但真正将结果应用到 Actor 时，会被限制在某个特定的 World 中
			if (Action != nullptr)
			{
				//执行 Action 的 registering 阶段。
				//通常用于注册全局信息，比如 GameplayTag、组件请求、扩展处理等。
				Action->OnGameFeatureRegistering();
				//执行 Action 的 loading 阶段。
				//通常用于加载或准备该 Action 需要的数据。
				Action->OnGameFeatureLoading();
				//执行 Action 的 activating 阶段。
				//这是最关键的一步，很多实际效果在这里发生，比如：给 Actor 添加组件、添加输入映射、添加 UI、注册能力系统相关内容、启动某些玩法逻辑
				Action->OnGameFeatureActivating(Context);
			}
		}
	};

	ActivateListOfActions(CurrentExperience->Actions);
	for (const TObjectPtr<ULyraExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			ActivateListOfActions(ActionSet->Actions);
		}
	}
	LoadState = ELyraExperienceLoadState::Loaded;

	OnExperienceLoaded_HighPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_HighPriority.Clear();

	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();

	OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_LowPriority.Clear();

#if !UE_SERVER
	//应用必要的本地画质 / 性能 / 可扩展性设置。Dedicated Server 不需要这些，所以用 #if !UE_SERVER 排除。
	ULyraSettingsLocal::Get()->OnExperienceLoaded();
#endif
}

void ULyraExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// 停用该 Experience 加载的所有 Feature
	//@TODO: 这里也应该按 FILO（后进先出）的顺序处理
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		if (ULyraExperienceManager::RequestToDeactivatePlugin(PluginURL))
		{
			UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
		}
	}
	//@TODO: 也要确保能正确处理“部分加载”的状态
	if (LoadState == ELyraExperienceLoadState::Loaded)
	{
		LoadState = ELyraExperienceLoadState::Deactivating;
		//INDEX_NONE 的意义不是表示 0 个 pauser，而是表示 “现在还不能判断是否全部完成，因为 Action 还在注册 pauser 的过程中”
		NumExpectedPausers = INDEX_NONE;
		NumObservedPausers = 0;

		//构造反激活上下文，当某个Action反激活返回的Pauser完成异步工作后会调用该函数
		FGameFeatureDeactivatingContext Context(TEXT(""), [this](FStringView) { this->OnActionDeactivationCompleted(); });
		const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (ExistingWorldContext)
		{
			//告知 FGameFeatureDeactivatingContext：这组反激活操作只适用于当前 World
			Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
		}
		auto DeactivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
		{
			for (UGameFeatureAction* Action : ActionList)
			{
				if (Action)
				{
					//通知 Action："你对应的 GameFeature 正在被反激活"。
					//此时 Action
					//应执行运行时清理（如卸载已添加的 Component、移除已注册的 GameplayAbility、回滚 GameplayTag等）。
					//如果清理是异步的，Action 会在 Context 中注册 pauser。
					Action->OnGameFeatureDeactivating(Context);
					Action->OnGameFeatureUnregistering();
				}
			}
		};
		DeactivateListOfActions(CurrentExperience->Actions);
		for (const TObjectPtr<ULyraExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
		{
			if (ActionSet != nullptr)
			{
				DeactivateListOfActions(ActionSet->Actions);
			}
		}
		/**
		 * 此值在所有 Action 的 OnGameFeatureDeactivating 都调用完毕之后才能确定，因此必须在循环结束后获取。
		 * 这是异步流程的核心：如果没有任何 Action 注册 pauser（NumExpectedPausers == 0），说明反激活是同步完成的，可以直接调用 OnAllActionsDeactivated()。
		 */
		NumExpectedPausers = Context.GetNumPausers();

		/**
		* - Lyra 在编写时尚未完全支持异步反激活。
		* 见第 23 行的 TODO："Support deactivating an experience and do the unloadingactions"。
		* 这是当时的一个"半成品"状态——框架搭好了，但异步清理的实际测试和验证还没做完。打印 Error是留给开发者的诊断信息，防止静默失败。
		 */
		if (NumExpectedPausers > 0)
		{
			UE_LOG(LogLyraExperience, Error, TEXT("Actions that have asynchronous deactivation aren't fully supported yet in Lyra experiences"));
		}

		if (NumExpectedPausers == NumObservedPausers)
		{
			OnAllActionsDeactivated();
		}
	}
}


void ULyraExperienceManagerComponent::OnActionDeactivationCompleted()
{
	check(IsInGameThread());
	++NumObservedPausers;
	if (NumObservedPausers == NumExpectedPausers)
	{
		OnAllActionsDeactivated();
	}
}

void ULyraExperienceManagerComponent::OnAllActionsDeactivated()
{
	LoadState = ELyraExperienceLoadState::Unloaded;
	CurrentExperience = nullptr;
}
