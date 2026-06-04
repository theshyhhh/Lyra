#pragma once


#include "GameFramework/GameSession.h"
#include "LyraGameSession.generated.h"

UCLASS(Config = Game)
class ALyraGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	ALyraGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** Override to disable the default behavior */
	virtual bool ProcessAutoLogin() override;

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
};
