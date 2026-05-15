// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/// <summary>
/// 客户端构建目标 - 用于打包后的独立客户端（非编辑器、非服务器）
/// </summary>
public class LyraClientTarget : TargetRules
{
	public LyraClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;

		// 客户端只加载 LyraGame 模块，不需要编辑器模块
		ExtraModuleNames.Add("LyraGame");

		// 应用 LyraGame 和 LyraClient 共享的构建配置
		LyraGameTarget.ApplySharedLyraTargetSettings(this);
	}
}
