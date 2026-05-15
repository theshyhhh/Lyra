// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/// <summary>
/// 编辑器构建目标 - 用于在 UE 编辑器中开发和调试
/// </summary>
public class LyraEditorTarget : TargetRules
{
	public LyraEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		// 编辑器需要同时加载 LyraGame 运行时模块和 LyraEditor 编辑器模块
		ExtraModuleNames.AddRange(new string[] { "LyraGame", "LyraEditor" });

		//当构建 LyraEditor 时，如果不是全模块构建，就开启更严格的 UHT 检查，禁止反射类型里的裸 UObject 成员指针，要求使用 TObjectPtr。
		//但在 bBuildAllModules == true 的全模块构建场景下不启用这个限制，避免因为引擎、插件或额外模块中未迁移的代码导致大范围构建失败。
		if (!bBuildAllModules)
		{
			NativePointerMemberBehaviorOverride = PointerMemberBehavior.Disallow;
		}

		// 应用 LyraGame 和 LyraEditor 共享的构建配置
		LyraGameTarget.ApplySharedLyraTargetSettings(this);
		
		//这用于配合 “Unreal Remote 2” 应用进行触摸屏开发
		EnablePlugins.Add("RemoteSession");
	}
}