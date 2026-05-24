#pragma once

#include "CoreMinimal.h"
#include "LyraAssetManagerStartupJob.h"
#include "Engine/AssetManager.h"
#include "LyraAssetManager.generated.h"

#define UE_API LYRAGAME_API

class ULyraPawnData;
class ULyraGameData;

struct FLyraBundles
{
	static const FName Equipped;
};

/**
 * Asset Manager 的游戏实现，用于重写功能并存储游戏特定类型。
 * 预计大多数游戏都会想要重写 AssetManager，因为它为游戏特定的加载逻辑提供了一个合适的位置。
 * 通过在 DefaultEngine.ini 中设置 'AssetManagerClassName' 来使用该类。
 */
UCLASS(MinimalAPI, Config=Game)
class ULyraAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UE_API ULyraAssetManager();

	static UE_API ULyraAssetManager& Get();

	UE_API const ULyraGameData& GetGameData();

	UE_API const ULyraPawnData* GetDefaultPawnData() const;

	UE_API virtual void StartInitialLoading() override;

protected:
	/**
	 * 按指定的 GameData 类型获取全局游戏数据资产；
	 * 如果已经加载过，就直接从缓存返回；如果没加载过，就阻塞加载并缓存，然后返回强类型引用。
	 * @tparam GameDataClass 要加载的资产的类
	 * @param DataPath 资产软引用路径
	 * @return 该资产的引用
	 */
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		//检查该资产是否已经加载过
		if (const TObjectPtr<UPrimaryDataAsset>* pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(pResult);
		}
		return *CastChecked<GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

#if WITH_EDITOR

	UE_API virtual void PreBeginPIE(bool bStartSimulate) override;

#endif
	/**
	 * 按给定类型和软引用路径加载 GameData 资产，成功后缓存到 GameDataMap，失败就直接 Fatal 终止。
	 * @param DataClass 要加载的资产类
	 * @param DataClassPath 资产软引用路径
	 * @param PrimaryAssetType 资产类型
	 * @return 对应资产指针
	 */
	UE_API UPrimaryDataAsset* LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath,
	                                              FPrimaryAssetType PrimaryAssetType);

	/**
	 * 全局的游戏数据资产
	 * 通过ini配置，不配置会报错
	 */
	UPROPERTY(Config)
	TSoftObjectPtr<ULyraGameData> LyraGameDataPath;

	/**
	 * 缓存已经加载过的全局 GameData 资产，按资产类类型索引，避免重复同步加载。
	 * Transient：运行时缓存，不保存到磁盘、蓝图或配置。
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	/**
	 * 如果 Player State 上没有设置 Pawn Data，则在生成玩家 Pawn 时使用该 Pawn Data。
	 */
	UPROPERTY(Config)
	TSoftObjectPtr<ULyraPawnData> DefaultPawnData;

private:
	UE_API void DoAllStartupJobs();

	// 初始化GameplayCueManager
	UE_API void InitializeGameplayCueManager();

	UE_API void UpdateInitialGameContentLoadPercent(float GameContentPercent);

	TArray<FLyraAssetManagerStartupJob> StartupJobs;
};
#undef UE_API
