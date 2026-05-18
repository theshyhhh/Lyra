#include "LyraEditor.h"

#include "GameModes/LyraExperienceManager.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FLyraEditorModule"

DEFINE_LOG_CATEGORY(LogLyraEditor)

class FLyraEditorModule : public FDefaultGameModuleImpl
{
public:
	using ThisClass = FLyraEditorModule;

	virtual void StartupModule() override
	{
		//IsRunningGame():检查该可执行文件是否以游戏进程（而不是编辑器或专用服务器进程）的方式启动。
		//只在对应编辑器绑定对应委托
		if (!IsRunningGame())
		{
			FEditorDelegates::BeginPIE.AddRaw(this, &ThisClass::OnBeginPIE);
			FEditorDelegates::EndPIE.AddRaw(this, &ThisClass::OnEndPIE);
		}
	}

	virtual void ShutdownModule() override
	{
	}

	void OnBeginPIE(const bool bIsSimulating)
	{
		ULyraExperienceManager* LyraExperienceManager = GEngine->GetEngineSubsystem<ULyraExperienceManager>();
		check(LyraExperienceManager)
		LyraExperienceManager->OnPlayInEditorBegun();
	}

	void OnEndPIE(const bool bIsSimulating)
	{
	}
};

IMPLEMENT_MODULE(FLyraEditorModule, LyraEditor)
#undef LOCTEXT_NAMESPACE
