#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "System/GameplayTagStack.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "LyraPlayerState.generated.h"

#define UE_API LYRAGAME_API
class ULyraExperienceDefinition;
class ULyraPawnData;
class ULyraAbilitySystemComponent;
struct FLyraVerbMessage;
struct FGameplayTag;
class ALyraPlayerController;

//描述“这个 PlayerState 当前代表哪种连接身份”的运行时状态 enum。
UENUM()
enum class ELyraPlayerConnectionType : uint8
{
	// 正常活跃玩家
	Player = 0,

	// 正在观看当前运行中比赛的在线旁观者。
	LiveSpectator,

	// 正在观看 demo / replay 回放的旁观者。
	ReplaySpectator,

	// 已停用玩家，通常表示断线 / 离开后的 PlayerState 状态。
	InactivePlayer
};

UCLASS(MinimalAPI, Config = Game)
class ALyraPlayerState : public AModularPlayerState, public IAbilitySystemInterface, public ILyraTeamAgentInterface

{
	GENERATED_BODY()

public:
	UE_API ALyraPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerState")
	UE_API ALyraPlayerController* GetLyraPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "Lyra|PlayerState")
	ULyraAbilitySystemComponent* GetLyraAbilitySystemComponent() const { return AbilitySystemComponent; }

	UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	UE_API void SetPawnData(const ULyraPawnData* InPawnData);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//~AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	UE_API virtual void Reset() override;
	UE_API virtual void ClientInitialize(AController* C) override;
	UE_API virtual void CopyProperties(APlayerState* PlayerState) override;
	UE_API virtual void OnDeactivated() override;
	UE_API virtual void OnReactivated() override;
	//~End of APlayerState interface

	//~ILyraTeamAgentInterface interface
	UE_API virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	UE_API virtual FGenericTeamId GetGenericTeamId() const override;
	UE_API virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface

	UE_API void SetPlayerConnectionType(ELyraPlayerConnectionType NewType);
	ELyraPlayerConnectionType GetPlayerConnectionType() const { return MyPlayerConnectionType; }

	/** 返回玩家所在小队ID. */
	UFUNCTION(BlueprintCallable)
	int32 GetSquadId() const
	{
		return MySquadID;
	}

	/** 返回玩家所在队伍ID. */
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId() const
	{
		return GenericTeamIdToInteger(MyTeamID);
	}

	UE_API void SetSquadID(int32 NewSquadID);

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	UE_API void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	UE_API void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Teams)
	UE_API int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Teams)
	UE_API bool HasStatTag(FGameplayTag Tag) const;

	//向该玩家所在客户端发送信息
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "Lyra|PlayerState")
	UE_API void ClientBroadcastMessage(const FLyraVerbMessage Message);


	// 获取该玩家复制过来的视角旋转，用于观战。
	UE_API FRotator GetReplicatedViewRotation() const;

	// 设置复制的视角旋转，仅在服务器上有效
	UE_API void SetReplicatedViewRotation(const FRotator& NewRotation);

	static UE_API const FName NAME_LyraAbilityReady;

protected:
	UFUNCTION()
	UE_API void OnRep_PawnData();

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const ULyraPawnData> PawnData;

private:
	UE_API void OnExperienceLoaded(const ULyraExperienceDefinition* CurrentExperience);

	UPROPERTY(VisibleAnywhere, Category = "Lyra|PlayerState")
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemComponent;

	// // Health attribute set used by this actor.
	// UPROPERTY()
	// TObjectPtr<const class ULyraHealthSet> HealthSet;
	// // Combat attribute set used by this actor.
	// UPROPERTY()
	// TObjectPtr<const class ULyraCombatSet> CombatSet;

	UPROPERTY(Replicated)
	ELyraPlayerConnectionType MyPlayerConnectionType;

	UPROPERTY(ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(ReplicatedUsing=OnRep_MySquadID)
	int32 MySquadID;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	FRotator ReplicatedViewRotation;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

private:
	UFUNCTION()
	UE_API void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	UE_API void OnRep_MySquadID();
};

#undef UE_API
