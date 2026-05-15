#include "LyraLogChannels.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogLyra);
DEFINE_LOG_CATEGORY(LogLyraExperience);
DEFINE_LOG_CATEGORY(LogLyraAbilitySystem);
DEFINE_LOG_CATEGORY(LogLyraTeams)

FString GetClientServerContextString(UObject* ContextObject)
{
	ENetRole Role=ROLE_None;
	if (AActor* Actor = Cast<AActor>(ContextObject))
	{
		Role=Actor->GetLocalRole();
	}
	else if (UActorComponent* ActorComp=Cast<UActorComponent>(ContextObject))
	{
		Role=ActorComp->GetOwnerRole();
	}
	if (Role!=ROLE_None)
	{
		return Role==ROLE_Authority ? TEXT("Server") : TEXT("Client");
	}
	else
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			/*
			在编辑器 PIE 中切换不同 Play World 时，保存一个人类可读的当前 PIE 上下文描述，比如：
			Not in a play world
			Standalone
			Listen Server
			Dedicated Server
			Client 1
			Client 2
			Editor
			*/
			extern ENGINE_API FString GPlayInEditorContextString;
			return GPlayInEditorContextString;
		}
#endif
	}
	return TEXT("[]");
};
