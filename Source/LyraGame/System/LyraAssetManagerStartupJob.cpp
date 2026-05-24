#include "LyraAssetManagerStartupJob.h"

#include "LyraLogChannels.h"

TSharedPtr<FStreamableHandle> FLyraAssetManagerStartupJob::DoJob() const
{
	const double JobStartTime = FPlatformTime::Seconds();
	TSharedPtr<FStreamableHandle> Handle;
	UE_LOG(LogLyra, Display, TEXT("Startup job \"%s\" starting"), *JobName);
	JobFunc(*this, Handle);
	/**
	 * 当前项目下，ULyraAssetManager没用到FStreamableHandle相关的加载，所以暂时没有
	 */
	if (Handle.IsValid())
	{
		//绑定当前任务进度的委托
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate::CreateRaw(this, &FLyraAssetManagerStartupJob::UpdateSubstepProgressFromStreamable));
		Handle->WaitUntilComplete(0.f, false);
		//清空回调
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate());
	}
	UE_LOG(LogLyra, Display, TEXT("Startup job \"%s\" took %.2f seconds to complete"), *JobName, FPlatformTime::Seconds() - JobStartTime);
	return Handle;
}
