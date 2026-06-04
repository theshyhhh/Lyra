#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdEngine.h"
#include "LyraEditorEngine.generated.h"

UCLASS()
class ULyraEditorEngine : public UUnrealEdEngine
{
	GENERATED_BODY()

public:
	ULyraEditorEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Init(IEngineLoop* InEngineLoop) override;

	virtual void Start() override;

	virtual void Tick(float DeltaSeconds, bool bIdleMode) override;

	/// <summary>
	/// PIE（Play In Editor）启动前的扩展钩子。
	/// 可在此处：修改 PIE 网络模式、注入启动前的初始化逻辑、阻止特定条件下的 PIE 启动、或通知其他系统 PIE 即将开始。
	/// </summary>
	virtual FGameInstancePIEResult PreCreatePIEInstances(const bool bAnyBlueprintErrors, const bool bStartInSpectatorMode,
	                                                     const float PIEStartTime, const bool bSupportsOnlinePIE,
	                                                     int32& InNumOnlinePIEInstances) override;

private:
	/**
	 * 在引擎执行第一帧时强制让 UE 编辑器的 Content Browser 显示插件目录。
	 */
	void FirstTickSetup();

	bool bFirstTickSetup = false;
};
