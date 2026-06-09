#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "GameplayTagStack.generated.h"

struct FGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;


USTRUCT(BlueprintType)
struct FGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FGameplayTagStack()
	{
	}

	FGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		  , StackCount(InStackCount)
	{
	}

	FString GetDebugString() const;

private:
	friend FGameplayTagStackContainer;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

USTRUCT(BlueprintType)
struct FGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	FGameplayTagStackContainer()
	//	: Owner(nullptr)
	{
	}

	// 为该 Tag 添加指定数量的 Stack（如果 StackCount 小于 1，则不执行任何操作）。
	void AddStack(FGameplayTag Tag, int32 StackCount);

	// 从该 Tag 中移除指定数量的 Stack（如果 StackCount 小于 1，则不执行任何操作）。
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	// 返回指定 Tag 的 Stack 数量（如果该 Tag 不存在，则返回 0）。
	int32 GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	// 如果指定 Tag 至少有一个 Stack，则返回 true。
	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	//~FFastArraySerializer contract
	//这三个函数用于同步给客户端TagToCountMap
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract


	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayTagStack, FGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	// 复制的 Gameplay Tag Stack 列表
	UPROPERTY()
	TArray<FGameplayTagStack> Stacks;

	// 用于查询的加速版 Tag Stack 列表
	TMap<FGameplayTag, int32> TagToCountMap;
};

template <>
struct TStructOpsTypeTraits<FGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
