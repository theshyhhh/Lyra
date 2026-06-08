#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "LyraVerbMessage.generated.h"

/**
 * FLyraVerbMessage 是 Lyra 的通用 gameplay event payload，用来表达“Instigator 对 Target 做了某个 Verb，带有 tags 上下文和数值强度”
 * 没有这个结构，Damage、Elimination、Assist、Streak、UI notification 等系统就要互相直接调用。
 * FLyraVerbMessage 把这些系统解耦成“发布者只广播事件，监听者按 GameplayTag 订阅并处理”。
 */
USTRUCT(BlueprintType)
struct FLyraVerbMessage
{
	GENERATED_BODY()

	/**
	 * 事件动词，也是通常使用的 message channel。例如 Damage、Elimination、Assist、Streak。
	 * 最核心字段。广播时通常 BroadcastMessage(Message.Verb, Message)，所以它决定谁能收到消息。不要把它当普通分类标签；它是事件语义入口。
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTag Verb;

	/**
	 * 行为发起者，即“谁做的”。Damage 中可能是 EffectCauser，Elimination 中可能是击杀者。
	 * 是 UObject 引用，不代表所有权。消费者常用 ULyraVerbMessageHelpers::GetPlayerStateFromObject 把 Controller、PlayerState、Pawn 统一转成 PlayerState
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Instigator = nullptr;

	/**
	 * 行为承受者，即“对谁做的”。Damage 中是被伤害 Actor，Elimination 中常转成被淘汰玩家的 PlayerState。
	 * 也不代表所有权。跨网络时最好放可复制、客户端可解析的 Actor / PlayerState；不要放纯本地临时 UObject。
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Target = nullptr;

	/**
	 * 发起者当时的 tag 快照。Damage 中来自 GAS captured source tags。
	 * 这是事件发生瞬间的上下文，不应之后再去实时查询 ASC 来替代它。适合记录攻击者状态、武器、Ability、阵营等语义。
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer InstigatorTags;

	/**
	 * 目标当时的 tag 快照。Damage 中来自 GAS captured target tags。
	 * 用于监听端判断目标状态，例如是否护盾、死亡、队伍、特殊状态。Assist 处理器会把它继续传给派生消息。
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer TargetTags;

	/**
	 * 非 Instigator / Target 本身的额外语境，例如 headshot、team kill、environmental、weapon type、objective context。
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer ContextTags;

	/**
	 * 事件数值强度。Damage 中是伤害值，Assist 中是累计伤害，Streak 中是连杀数。
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	double Magnitude = 1.0;

	/**
	 * 
	 * @return 用 Reflection 导出可读调试字符串。
	 */
	LYRAGAME_API FString ToString() const;
};
