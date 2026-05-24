#include "LyraAssetManager.h"

#include "LyraLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraAssetManager)

const FName FLyraBundles::Equipped("Equipped");

#define STARTUP_JOB_WEIGHTED(JobFunc,JobWeight) StartupJobs.Add(FLyraAssetManagerStartupJob(#JobFunc,[this](const FLyraAssetManagerStartupJob&\
StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){JobFunc;}, JobWeight))

#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)

ULyraAssetManager::ULyraAssetManager()
{
	DefaultPawnData = nullptr;
}

ULyraAssetManager& ULyraAssetManager::Get()
{
	check(GEngine)
	if (ULyraAssetManager* LyraAssetManager = Cast<ULyraAssetManager>(GEngine->AssetManager))
	{
		return *LyraAssetManager;
	}
	//如果失败则说明没有正确配置，报Fatal错误
	UE_LOG(LogLyra, Fatal, TEXT(" DefaultEngine.ini 中的 AssetManagerClassName 无效。它必须设置为 LyraAssetManager！"));
	//其实该语句不可达
	return *NewObject<ULyraAssetManager>();
}

const ULyraGameData& ULyraAssetManager::GetGameData()
{
}

const ULyraPawnData* ULyraAssetManager::GetDefaultPawnData() const
{
}

void ULyraAssetManager::StartInitialLoading()
{
	/**
	 * 启动性能统计宏。
	 * 它会记录这个作用域的耗时，用于 UE 启动分析。这里的字符串是统计项名称，方便在启动日志或 profiling 工具里看到 ULyraAssetManager::StartInitialLoading 花了多久
	 */
	SCOPED_BOOT_TIMING("ULyraAssetManager::StartInitialLoading");

	Super::StartInitialLoading();
	STARTUP_JOB(InitializeGameplayCueManager());
	{
		STARTUP_JOB_WEIGHTED(GetGameData(), 25.f);
	}
	DoAllStartupJobs();
}

void ULyraAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);
}

UPrimaryDataAsset* ULyraAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass,
                                                          const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath,
                                                          FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime)

	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(
			0, FText::Format(NSLOCTEXT("LyraEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));

		const bool bShowCancelButton = false;
		const bool bAllowInPie = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPie);
#endif
		UE_LOG(LogLyra, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);
		// 在编辑器中，这里可能会被递归调用，因为它会按需从 PostLoad 中调用；
		// 因此在这种情况下，对 Primary Asset 强制进行同步加载，其余资源则异步加载。
		if (GIsEditor)
		{
			//同步加载主资产
			Asset = DataClassPath.LoadSynchronous();
			//请求 AssetManager 加载该 PrimaryAssetType 下的 Primary Assets。
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				/**
				 * 阻塞直到请求的资源加载完成。该函数会将请求的资源推到优先级列表的顶部，
				 * 但不会刷新所有异步加载，因此通常会比调用 LoadObject 更快完成。
				 * @param Timeout                最大等待时间；如果该值为 0，则会永久等待。
				 * @param StartStalledHandles    如果为 true，则会强制所有正在等待外部资源的 Handle 立即尝试加载。
				 */
				Handle->WaitUntilComplete(0, false);
				//获取已加载的资产
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}
	if (Asset)
	{
		//如果资产加载成功，则添加至GameDataMap
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		//如果失败则程序崩溃
		UE_LOG(LogLyra, Fatal,
		       TEXT(
			       "Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."
		       ),
		       *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}
	return Asset;
}

void ULyraAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("ULyraAssetManager::DoAllStartupJobs");
	const double AllStartupJobsStartTime = FPlatformTime::Seconds();
	/**
	 * 检查当前可执行程序是否是作为 Dedicated Server 进程启动的，并且不应加载仅客户端使用的数据。
	 * Editor 构建可以通过 -server 启动参数将其设置为 true，但在单进程 PlayInEditor 模式下它会是 false。
	 * 该函数不应被用于 Gameplay 或网络用途；应改为检查 NM_DedicatedServer。
	 */
	if (IsRunningDedicatedServer())
	{
		for (const FLyraAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			//服务器下无需定期提供加载进度来更新加载界面
			// ReSharper disable once CppExpressionWithoutSideEffects
			StartupJob.DoJob();
		}
	}
	else
	{
		if (StartupJobs.Num() > 0)
		{
			//总进度
			float TotalJobValue = 0.f;
			for (const FLyraAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				TotalJobValue += StartupJob.JobWeight;
			}
			//当前进度
			float AccumulatedJobValue = 0.f;
			for (FLyraAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				//当前工作所占进度权重
				const float JobValue = StartupJob.JobWeight;
				//如果是使用的FStreamableHandle相关的加载，任务执行时，每隔一段时间汇报进度
				StartupJob.SubstepProgressDelegate.BindLambda([This=this,TotalJobValue,AccumulatedJobValue,JobValue](float NewProgress)
				{
					//当前工作已完成的进度
					const float SubstepAdjustment = FMath::Clamp(NewProgress, 0.f, 1.f) * JobValue;

					//所有工作的当前已完成进度
					const float OverallPercentWithSubstep = (SubstepAdjustment + AccumulatedJobValue) / TotalJobValue;

					//更新加载进度
					This->UpdateInitialGameContentLoadPercent(OverallPercentWithSubstep);
				});

				// ReSharper disable once CppExpressionWithoutSideEffects
				//执行任务：会阻塞，直至加载完毕
				StartupJob.DoJob();
				//任务执行完成后，解绑回调
				StartupJob.SubstepProgressDelegate.Unbind();
				AccumulatedJobValue += JobValue;
				UpdateInitialGameContentLoadPercent(AccumulatedJobValue);
			}
		}
		else
		{
			UpdateInitialGameContentLoadPercent(1.f);
		}
	}
	StartupJobs.Empty();
	//打印加载消耗的时间
	UE_LOG(LogLyra, Display, TEXT("All startup jobs took %.2f seconds to complete"), FPlatformTime::Seconds() - AllStartupJobsStartTime);
}

void ULyraAssetManager::InitializeGameplayCueManager()
{
	SCOPED_BOOT_TIMING("ULyraAssetManager::InitializeGameplayCueManager")
	//TODO:加载必须的GameplayCue
}

void ULyraAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
}
