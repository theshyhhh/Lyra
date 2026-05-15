#include "LyraEditorEngine.h"

#include "LyraEditor.h"
#include "Settings/ContentBrowserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraEditorEngine)

ULyraEditorEngine::ULyraEditorEngine(const FObjectInitializer& ObjectInitializer)
{
}

void ULyraEditorEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
}

void ULyraEditorEngine::Start()
{
	Super::Start();
	UE_LOG(LogLyraEditor, Display, TEXT("自定义编辑器引擎类：%s启动"), *GetName());
}

void ULyraEditorEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	Super::Tick(DeltaSeconds, bIdleMode);
	FirstTickSetup();
}

FGameInstancePIEResult ULyraEditorEngine::PreCreatePIEInstances(const bool bAnyBlueprintErrors, const bool bStartInSpectatorMode,
                                                                const float PIEStartTime, const bool bSupportsOnlinePIE,
                                                                int32& InNumOnlinePIEInstances)
{
	return Super::PreCreatePIEInstances(bAnyBlueprintErrors, bStartInSpectatorMode, PIEStartTime, bSupportsOnlinePIE, InNumOnlinePIEInstances);
}

void ULyraEditorEngine::FirstTickSetup()
{
	if (bFirstTickSetup)
	{
		return;
	}
	bFirstTickSetup = true;
	GetMutableDefault<UContentBrowserSettings>()->SetDisplayPluginFolders(true);
}
