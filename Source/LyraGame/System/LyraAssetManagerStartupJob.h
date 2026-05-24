#pragma once
#include "Engine/StreamableManager.h"


DECLARE_DELEGATE_OneParam(FLyraAssetManagerStartupJobSubstepProgress, float/*New Progress*/)

/**
 * 为什么要有这个结构体？
 * 因为启动阶段有多种加载任务，有些是纯同步逻辑，有些可能产生 FStreamableHandle，
 * 但 AssetManager 希望用同一套流程处理它们：排队、命名、计时、执行、等待异步加载完成、更新进度、最后清空任务列表。
 */
struct FLyraAssetManagerStartupJob
{
	FLyraAssetManagerStartupJobSubstepProgress SubstepProgressDelegate;

	//真正要执行的启动任务函数
	TFunction<void(const FLyraAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)> JobFunc;

	//工作名称
	FString JobName;

	//加载的进度权重
	float JobWeight;

	mutable double LastUpdate = 0;

	FLyraAssetManagerStartupJob(const FString& InJobName, const TFunction<void(const FLyraAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)>&
	                            InJobFunc, float InJobWeight) : JobFunc(InJobFunc), JobName(InJobName), JobWeight(InJobWeight)
	{
	}

	TSharedPtr<FStreamableHandle> DoJob() const;

	void UpdateSubstepProgress(float NewProgress) const
	{
		SubstepProgressDelegate.ExecuteIfBound(NewProgress);
	}

	void UpdateSubstepProgressFromStreamable(TSharedRef<FStreamableHandle> StreamableHandle) const
	{
		if (SubstepProgressDelegate.IsBound())
		{
			// StreamableHandle::GetProgress traverses() a large graph and is quite expensive
			double Now = FPlatformTime::Seconds();
			if (Now - LastUpdate > 1.0 / 60)
			{
				SubstepProgressDelegate.Execute(StreamableHandle->GetProgress());
				LastUpdate = Now;
			}
		}
	}
};
