#pragma once

#include "Components/GameStateComponent.h"
#include "LyraPlayerSpawningManagerComponent.generated.h"

#define UE_API LYRAGAME_API

class ALyraPlayerStart;

UCLASS(MinimalAPI)
class ULyraPlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UE_API ULyraPlayerSpawningManagerComponent(const FObjectInitializer& Initializer = FObjectInitializer::Get());

	/** UActorComponent */

	UE_API virtual void InitializeComponent() override;

	UE_API virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** ~UActorComponent */

protected:
	/**
	 * 获取FoundStartPoints 中Controller 能使用的出生点 
	 */
	UE_API APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<ALyraPlayerStart*>& FoundStartPoints) const;

	//出生点选择逻辑由子类自定义
	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<ALyraPlayerStart*>& PlayerStarts) { return nullptr; }

	//玩家重生时调用，逻辑交由子类重写
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation)
	{
	}

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName=OnFinishRestartPlayer))
	UE_API void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);

private:
	/** 我们将这些调用从 ALyraGameMode 代理到这个组件中，这样每个 Experience 都可以更轻松地自定义它们想要的重生系统。*/
	
	/**
	 * 选择合适的出生点
	 */
	UE_API AActor* ChoosePlayerStart(AController* Player);

	//返回是否可以重生
	UE_API bool ControllerCanRestart(AController* Player);
	
	UE_API void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	friend class ALyraGameMode;

	/** ~ALyraGameMode*/
	/**
	 * 当通过 UWorld::AddToWorld 将 ULevel 添加到 World 时的回调函数
	 * 将新增的 ULevel 中的 ALyraPlayerStart 添加到 CachedPlayerStarts 中
	 */
	UE_API void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);

	/**
	 * 当有 Actor 被动态生成时的回调函数
	 * 判断当前生成的 Actor 是否是 ALyraPlayerStart，如果是则添加到 CachedPlayerStarts
	 * @param SpawnedActor 生成的Actor
	 */
	UE_API void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	/**
	 * 在 PIE 模式下，从当前位置开始时，选择的出生点
	 * 如果找到了第一个“Play from Here”PlayerStart，则始终优先使用它。
	 */
	UE_API APlayerStart* FindPlayFromHereStart(AController* Player);
#endif

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALyraPlayerStart>> CachedPlayerStarts;
};

#undef UE_API
