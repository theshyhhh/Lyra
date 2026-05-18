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

	//在创建 PIE 实例之前调用
	//判断WorldSetting是否勾选了强制单机PIE，如果是则强制PIE网络模式更改为单机
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
