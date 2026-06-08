#pragma once

#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "LyraGameState.generated.h"

#define UE_API LYRAGAME_API
struct FLyraVerbMessage;
class ULyraAbilitySystemComponent;
class ULyraExperienceManagerComponent;
/**
 * AModularGameStateBase 负责把 GameStateBase 注册给 UGameFrameworkComponentManager，让 GameFeature 可以给 GameState 动态加组件、监听扩展事件。
 */
UCLASS(MinimalAPI, Config = Game)
class ALyraGameState : public AModularGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UE_API ALyraGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface

	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void PostInitializeComponents() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UE_API virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	//~AGameStateBase interface

	UE_API virtual void AddPlayerState(APlayerState* PlayerState) override;
	UE_API virtual void RemovePlayerState(APlayerState* PlayerState) override;
	/**
	 * SeamlessTravelTransitionCheckpoint 是 UE 在 Seamless Travel 跨地图切换过程中调用的 GameState 钩子。
	 * Lyra 在这里的作用很明确：在跨地图保留 PlayerState 之前，把不应该带到下一个地图的 PlayerState 从 PlayerArray 里移除。
	 * 注意它不是销毁 PlayerState Actor，只是从 GameState 的 PlayerArray 中移除。
	 * 真正哪些 Actor 被保留、迁移或销毁，仍由 UE 的 Seamless Travel 流程、Controller、GameMode 的 keep actor list 等机制决定。
	 * 调用时机：UE 在 Seamless Travel 过程中会调用它两次：一次进入 transition map，一次进入最终目标 map。
	 * @param bToTransitionMap  表示当前 checkpoint 是否是“去 transition map”这一段
	 */
	UE_API virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IAbilitySystemInterface

	UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	UFUNCTION(BlueprintCallable, Category = "Lyra|GameState")
	ULyraAbilitySystemComponent* GetLyraAbilitySystemComponent() const { return AbilitySystemComponent; }

	// 发送一条所有客户端（大概率）都会收到的消息
	// （仅用于客户端通知，例如淘汰、服务器加入消息等……这些消息即使丢失也能接受）
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Lyra|GameState")
	UE_API void MulticastMessageToClients(const FLyraVerbMessage Message);

	// 发送一条保证所有客户端都会收到的消息
	// （仅用于不能接受丢失的客户端通知）
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Lyra|GameState")
	UE_API void MulticastReliableMessageToClients(const FLyraVerbMessage Message);

	// 获取服务端的FPS
	UE_API float GetServerFPS() const;

	// 表示本地 PlayerState 正在录制回放。
	UE_API void SetRecorderPlayerState(APlayerState* NewPlayerState);

	// Gets the player state that recorded the replay, if valid
	UE_API APlayerState* GetRecorderPlayerState() const;

	// Delegate called when the replay player state changes
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecorderPlayerStateChanged, APlayerState*);
	FOnRecorderPlayerStateChanged OnRecorderPlayerStateChangedEvent;


	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY()
	TObjectPtr<ULyraExperienceManagerComponent> ExperienceManagerComponent;

	UPROPERTY(VisibleAnywhere, Category = "Lyra|GameState")
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemComponent;

protected:
	//服务端的帧率
	UPROPERTY(Replicated)
	float ServerFPS;

	// 录制回放的 PlayerState，用于选择要跟随的正确 Pawn
	// 只在 Replay 场景下复制
	UPROPERTY(Transient, ReplicatedUsing = OnRep_RecorderPlayerState)
	TObjectPtr<APlayerState> RecorderPlayerState;

	UFUNCTION()
	UE_API void OnRep_RecorderPlayerState();
};
#undef UE_API
