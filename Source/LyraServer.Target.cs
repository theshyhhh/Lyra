// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LyraServerTarget : TargetRules
{
	public LyraServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		ExtraModuleNames.Add("LyraGame");
		LyraGameTarget.ApplySharedLyraTargetSettings(this);
		
		//是否为 Test/Shipping 构建启用检查（断言）。
		bUseChecksInShipping = true;
	}
	
}
