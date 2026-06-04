#pragma once

#include "CommonGameViewportClient.h"
#include "LyraGameViewportClient.generated.h"

UCLASS(BlueprintType)
class ULyraGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()

public:
	ULyraGameViewportClient();

	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
};
