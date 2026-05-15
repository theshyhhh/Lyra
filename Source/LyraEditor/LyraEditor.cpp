#include "LyraEditor.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FLyraEditorModule"

DEFINE_LOG_CATEGORY(LogLyraEditor)

class FLyraEditorModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
        
	}
	virtual void ShutdownModule() override
	{
        
	}
};

IMPLEMENT_MODULE(FLyraEditorModule, LyraEditor)
#undef LOCTEXT_NAMESPACE
    
