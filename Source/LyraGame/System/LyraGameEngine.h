#pragma once

#include "CoreMinimal.h"
#include "Engine/GameEngine.h"
#include "LyraGameEngine.generated.h"

class IEngineLoop;

UCLASS()
class LYRAGAME_API ULyraGameEngine : public UGameEngine
{
	GENERATED_BODY()
	
public:
	ULyraGameEngine(const FObjectInitializer& ObjectInitializer=FObjectInitializer::Get());
	
	virtual void Init(IEngineLoop* InEngineLoop) override;
};
