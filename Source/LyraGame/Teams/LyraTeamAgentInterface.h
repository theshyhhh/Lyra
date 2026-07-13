#pragma once

#include "GenericTeamAgentInterface.h"
#include "UObject/Object.h"

#include "UObject/WeakObjectPtr.h"
#include "LyraTeamAgentInterface.generated.h"
#define UE_API LYRAGAME_API

template <typename InterfaceType>
class TScriptInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLyraTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamID, int32, NewTeamID);


inline int32 GenericTeamIdToInteger(FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam) ? INDEX_NONE : (int32)ID;
}

inline FGenericTeamId IntegerToGenericTeamId(int32 ID)
{
	return (ID == INDEX_NONE) ? FGenericTeamId::NoTeam : FGenericTeamId((uint8)ID);
}


UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class ULyraTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ILyraTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	//如果新旧队伍ID不同，则会进行广播，相同则不会
	static UE_API void ConditionalBroadcastTeamChanged(TScriptInterface<ILyraTeamAgentInterface> This, FGenericTeamId OldTeamID,
	                                                   FGenericTeamId NewTeamID);

	FOnLyraTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnLyraTeamIndexChangedDelegate* Result = GetOnTeamIndexChangedDelegate();
		check(Result);
		return *Result;
	}
};
#undef UE_API
