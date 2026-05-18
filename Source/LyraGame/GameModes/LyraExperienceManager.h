#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "LyraExperienceManager.generated.h"

/**
 * GameFeature Plugin 的激活状态在编辑器进程里比较全局。
 * 如果你开多个 PIE 实例，多个 Experience 可能都依赖同一个 GameFeature Plugin。
 * 某个 PIE 实例结束时，如果直接把插件卸载掉，另一个还在运行的 PIE 实例就可能被影响。
 * 所以需要ULyraExperienceManager用于多个 PIE Session 之间仲裁 GameFeature Plugin 的激活/卸载。
 */
UCLASS(MinimalAPI)
class ULyraExperienceManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	//在FLyraEditorModule::OnBeginPIE中调用
	//PIE开始时调用，确保GameFeaturePluginRequestCountMap为空
	LYRAGAME_API void OnPlayInEditorBegun();

	/**
	 * 某个 Experience 准备激活某个 GameFeature Plugin 前，通知Manager，把这个 PluginURL 的请求计数加一。
	 */
	static void NotifyOfPluginActivation(const FString PluginURL);
	/**
	 * 某个 Experience 结束时，请求卸载某个 GameFeature Plugin。但它不会无条件允许卸载，而是先减少计数。
	 * 如果计数减到 0，说明没有其他 PIE Session 还需要这个插件，返回 true，允许真正卸载。
	 * 如果计数仍然大于 0，说明还有别的 PIE Session 正在用，返回 false，这次不卸载。
	 */
	static bool RequestToDeactivatePlugin(const FString PluginURL);

	// virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//
	// virtual void Deinitialize() override;
#else
	static void NotifyOfPluginActivation(const FString PluginURL)
	{
	}
	static bool RequestToDeactivatePlugin(const FString PluginURL) { return true; }
#endif

private:
	//对GameFeature插件的引用计数表
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
};
