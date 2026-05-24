// Fill out your copyright notice in the Description page of Project Settings.


#include "LyraGameData.h"

#include "LyraAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameData)

ULyraGameData::ULyraGameData()
{
}

const ULyraGameData& ULyraGameData::Get()
{
	return ULyraAssetManager::Get().GetGameData();
}
