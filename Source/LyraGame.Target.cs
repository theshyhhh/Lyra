using UnrealBuildTool;
using System;
using System.IO;
using EpicGames.Core;
using System.Collections.Generic;
using UnrealBuildBase;
using Microsoft.Extensions.Logging;

/// <summary>
/// 游戏构建目标 - 用于打包后的独立游戏（不含编辑器功能）
/// </summary>
public class LyraGameTarget : TargetRules
{
	public LyraGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		ApplySharedLyraTargetSettings(this);
		ExtraModuleNames.Add("LyraGame");
	}

	private static bool bHasWarnedAboutShared = false;

	/// <summary>
	/// 为 LyraGame、LyraClient、LyraEditor 三个 Target 统一设置构建参数。
	/// 覆盖：构建设置版本、头文件包含顺序、编译警告等级、Shipping/Test 安全限制、GameFeature 插件启用策略等。
	/// 仅在 Unique 构建环境或 Editor 目标中执行完整配置；Shared 环境下只允许 Editor 配置 GameFeature 插件。
	/// </summary>
	internal static void ApplySharedLyraTargetSettings(TargetRules Target)
	{

		// 从 Target 中取出当前构建日志器，后续用于输出警告或调试日志。
		ILogger Logger = Target.Logger;

		// 使用最新的构建设置版本
		Target.DefaultBuildSettings = BuildSettingsVersion.V6;

		// 统一头文件包含顺序，避免不同平台的编译差异
		Target.IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// 判断当前是否为 Test 配置，用于启用测试构建专用限制。
		bool bIsTest = Target.Configuration == UnrealTargetConfiguration.Test;

		// 判断当前是否为 Shipping 配置，用于启用发行构建专用限制。
		bool bIsShipping = Target.Configuration == UnrealTargetConfiguration.Shipping;

		// 判断当前是否为专用服务器 Target，用于区分服务器和普通客户端/游戏构建。
		bool bIsDedicatedServer = Target.Type == TargetType.Server;

		// 指定是与其他项目共享引擎的引擎二进制文件和中间文件，还是为当前项目创建专用版本。
		// 默认情况下，编辑器构建始终使用共享构建环境（并且引擎二进制文件会写入 Engine/Binaries/Platform），而单体构建和程序构建默认不会这样做（已安装构建除外）。
		// 使用共享构建环境会阻止针对特定目标对构建环境进行修改。
		// 判断当前构建环境是否是 Unique；只有 Unique 环境允许安全地改动插件启用和部分全局编译选项。
		if (Target.BuildEnvironment == TargetBuildEnvironment.Unique)
		{
			// 将变量遮蔽警告提升为错误，避免局部变量隐藏成员或外层变量造成难查的问题。
			Target.CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Error;

			// 即使在 Shipping 构建中也保留日志能力，便于线上问题诊断。
			Target.bUseLoggingInShipping = true;

			// 在 Test 构建配置下，是否追踪 RHI 资源的所有者信息，也就是资源对应的 Asset 名称，帮助排查渲染资源泄漏或占用。
			Target.bTrackRHIResourceInfoForTest = true;

			// 对 Shipping 的非专用服务器构建应用额外安全限制。
			if (bIsShipping && !bIsDedicatedServer)
			{
				// 强制校验 HTTPS 证书，避免发行版本接受未验证证书。
				Target.bDisableUnverifiedCertificates = true;

				// 以下示例定义可启用命令行 allow list，使发行版本只解析指定参数。
				//Target.GlobalDefinitions.Add("UE_COMMAND_LINE_USES_ALLOW_LIST=1");
				// 以下示例定义具体允许的命令行参数列表。
				//Target.GlobalDefinitions.Add("UE_OVERRIDE_COMMAND_LINE_ALLOW_LIST=\"-space -separated -list -of -commands\"");

				// 以下示例定义可过滤敏感命令行参数，避免上传日志时泄露连接信息或令牌。
				//Target.GlobalDefinitions.Add("FILTER_COMMANDLINE_LOGGING=\"-some_connection_id -some_other_arg\"");
			}

			// 对 Shipping 和 Test 构建禁用 cooked 后的外部 ini 读取，收紧运行时配置来源。
			if (bIsShipping || bIsTest)
			{
				// 是否在 Cooked 构建中加载生成的 ini 文件（GameUserSettings.ini 无论如何都会被加载）
				Target.bAllowGeneratedIniWhenCooked = false;
				// Cooked Build 运行时，NonUFS（非 UE 文件系统）ini 是否参与 UE 配置加载。
				Target.bAllowNonUFSIniWhenCooked = false;
			}

			// 对非编辑器目标移除只在开发/截图场景需要的运行时负担。
			if (Target.Type != TargetType.Editor)
			{
				// 禁用 OpenImageDenoise 插件，避免运行时打包携带体积较大的路径追踪降噪 DLL。
				Target.DisablePlugins.Add("OpenImageDenoise");

				//运行时不需要像编辑器那样频繁管理所有资产。                                                                                                                                                                                                                                              
				//内存占用更敏感。                                                                                                                                                                                                                                                                        
				// 启用 AssetRegistry 间接指针模式，以更高查询成本换取常驻资产注册表内存降低。
				Target.GlobalDefinitions.Add("UE_ASSETREGISTRY_INDIRECT_ASSETDATA_POINTERS=1");
			}
			LyraGameTarget.ConfigureGameFeaturePlugins(Target);
		}
		else
		{
			// !!!!!!!!!!!! WARNING !!!!!!!!!!!!!
			// Any changes in here must not affect PCH generation, or the target
			// needs to be set to TargetBuildEnvironment.Unique

			// Shared 环境中只有 Editor 目标可以安全执行 GameFeature 插件配置。
			if (Target.Type == TargetType.Editor)
			{
				// 对 Editor 目标仍然配置 GameFeature 插件，方便编辑器中启用和编译功能插件。
				LyraGameTarget.ConfigureGameFeaturePlugins(Target);
			}
			else
			{
				// 共享的单体构建（Shared monolithic builds）无法启用/禁用插件，也不能更改任何选项，因为它会尝试复用已安装引擎的二进制文件。
				// 如果此前还没有输出过警告，则输出一次 Shared 构建限制提示。
				if (!bHasWarnedAboutShared)
				{
					// 标记已经输出过 Shared 环境警告。
					bHasWarnedAboutShared = true;
					// 告知用户安装版引擎打包时 LyraGameEOS 和动态 Target 选项会被禁用。
					//动态Target:它们会改变当前 Target 的插件启用状态、全局宏、Shipping 日志、Cooked ini 加载策略、GameFeature 插件启用策略等。 
					Logger.LogWarning("LyraGameEOS and dynamic target options are disabled when packaging from an installed version of the engine");
				}
			}
		}
	}

	// 判断是否应强制启用所有 GameFeature 插件。
	public static bool ShouldEnableAllGameFeaturePlugins(TargetRules Target)
	{
		//// 检查当前 Target 是否为 Editor；编辑器构建通常可以选择编译更多功能插件。
		if (Target.Type == UnrealBuildTool.TargetType.Editor)
		{
			// With return true, editor builds will build all game feature plugins, but it may or may not load them all.
			// This is so you can enable plugins in the editor without needing to compile code.
			// 取消下一行注释后，Editor 构建会编译所有 GameFeature 插件，但不一定全部加载。
			//因此可以在编辑器中启用插件无需编译代码
			// return true;
		}
		// 从 UBT 附加属性读取 IsBuildMachine 标记，判断当前是否运行在构建机（专门用于自动编译、Cook、打包、测试的机器）上。
		bool bIsBuildMachine = Target.AdditionalProperties.GetProperty("IsBuildMachine") == "1";

		if (bIsBuildMachine)
		{
			// 取消下一行注释后，构建机会编译所有 GameFeature 插件。
			// return true;
		}
		// 默认不强制启用所有插件，而是沿用编辑器插件浏览器中的启用规则。
		// 这一点很重要，因为对于通过启动器安装的引擎版本，这段代码可能根本不会被执行
		return false;
	}
	
	// 缓存已读取的插件根 JSON，避免同一插件描述文件在一次构建中被重复解析。
	private static Dictionary<string,JsonObject> AllPluginRootJsonObjectsByName = new Dictionary<string, JsonObject>();
	
	
	// 配置我们希望启用哪些 Game Feature 插件
	// 这是一个相当简单的实现，不过你也可以根据当前分支的目标发布版本来构建不同的插件，例如，在 main 分支中启用仍在开发中的功能，而在当前发布分支中将其禁用
	//该函数位于 LyraGame.Target.cs:160，在 UBT（Unreal Build Tool）构建阶段决定 哪些 GameFeature 插件应该被启用或禁用。
	//核心流程：
	//1. 扫描插件目录 — 遍历 Plugins/GameFeatures/ 下所有 .uplugin 文件。
	//2. 逐插件解析 JSON — 读取每个 .uplugin 描述文件，并使用缓存避免重复解析。
	//3. 校验插件约定 — 检查 GameFeature 插件是否正确设置了：
	//- EnabledByDefault = false（避免插件名嵌入可执行文件）
	//- ExplicitlyLoaded = true（保证插件在项目启动后按需加载，而非引擎自动加载）
	//4. 决策启用/禁用，按优先级递减：
	//- NeverBuild = true → 强制禁用
	//- RestrictToBranch 不匹配当前分支 → 强制禁用
	//- EditorOnly = true 且当前为非 Editor Target → 强制禁用
	//- ShouldEnableAllGameFeaturePlugins 返回 true → 启用（当前硬编码返回 false）
	//- 以上均不命中 → 保持插件原始状态（既不加入启用列表也不加入禁用列表）
	//5. 收集依赖关系 — 从 Plugins 字段中提取已启用的依赖项，预留后续校验。
	//6. 应用到 UBT — 将最终决策写入 Target.EnablePlugins 或 Target.DisablePlugins。
	 public static void ConfigureGameFeaturePlugins(TargetRules Target)
	{
		ILogger Logger = Target.Logger;
		
		// 只输出一次当前分支的 GameFeature 编译提示，帮助定位构建来自哪个分支。
		Log.TraceInformationOnce("Compiling GameFeaturePlugins in branch {0}", Target.Version.BranchName);

		bool bBuildAllGameFeaturePlugins = ShouldEnableAllGameFeaturePlugins(Target);
		
		// 创建插件描述文件List，用于汇总所有发现的 GameFeature .uplugin 文件。
		List<FileReference> CombinedPluginList = new List<FileReference>();
		
		// 查找项目 Plugins/GameFeatures 下所有插件描述文件
		List<DirectoryReference> GameFeaturePluginRoots = Unreal.GetExtensionDirs(Target.ProjectFile.Directory, Path.Combine("Plugins", "GameFeatures"));

		foreach (DirectoryReference SearchDir in GameFeaturePluginRoots)
		{
			// 枚举该目录下的插件描述文件，并加入统一插件列表。
			CombinedPluginList.AddRange(PluginsBase.EnumeratePlugins(SearchDir));
		}
		
		// 只有GameFeature 插件描述文件存在时才继续执行配置逻辑。
		if (CombinedPluginList.Count > 0)
		{
			// 记录每个插件声明依赖的其他插件名称，预留给后续依赖校验使用。
			Dictionary<string, List<string>> AllPluginReferencesByName = new Dictionary<string, List<string>>();
			//逐个处理发现的 GameFeature 插件描述文件。
			foreach (FileReference PluginFile in CombinedPluginList)
			{
				// 确认插件文件引用非空且文件实际存在，避免无效路径进入解析流程。
				if (PluginFile != null && FileReference.Exists(PluginFile))
				{
					// 默认不主动启用插件，除非后续规则明确打开。
					bool bEnabled = false;
					// 默认不强制禁用插件，除非后续规则发现必须排除。
					bool bForceDisabled = false;
					try
					{
						// 声明插件描述文件的 JSON 根对象。
						JsonObject RawObject;
						// 对全局 JSON 缓存加锁，确保多线程构建时读写缓存安全。
						lock (AllPluginRootJsonObjectsByName)
						{
							// 如果缓存中没有该插件的 JSON，则从磁盘读取。
							if (!AllPluginRootJsonObjectsByName.TryGetValue(PluginFile.GetFileNameWithoutExtension(), out RawObject))
							{
								// 读取 .uplugin 文件并解析为 JsonObject。
								RawObject = JsonObject.Read(PluginFile);
								// 以插件文件名作为键缓存解析结果。
								AllPluginRootJsonObjectsByName.Add(PluginFile.GetFileNameWithoutExtension(), RawObject);
							}
						}
						//下面两段代码主要是为了确保GameFeature 插件中EnabledByDefault为false，bExplicitlyLoaded为true
						// 验证所有 GameFeature 插件默认都是禁用的
						// 如果 EnabledByDefault 为 true，而某个插件又被禁用，那么该插件名称会被嵌入到可执行文件中
						// 如果这会造成问题，可以启用这个警告，并修改 game feature 编辑器插件模板，让新插件默认将 EnabledByDefault 设为禁用
						bool bEnabledByDefault = false;
						// 如果未显式设置 EnabledByDefault=false，说明插件模板或配置可能不符合 Lyra 的动态加载约定。
						if (!RawObject.TryGetBoolField("EnabledByDefault", out bEnabledByDefault) || bEnabledByDefault == true)
						{
							// 如需强校验，可启用该警告提醒插件应将 EnabledByDefault 设置为 false。
							//Log.TraceWarning("GameFeaturePlugin {0}, does not set EnabledByDefault to false. This is required for built-in GameFeaturePlugins.", PluginFile.GetFileNameWithoutExtension());
						}
						
						// 验证所有 GameFeature 插件都被设置为显式加载
						// 这一点很重要，因为 Game Feature 插件预期是在项目启动之后再被加载。
						// 读取 ExplicitlyLoaded 字段，验证 GameFeature 插件应由系统显式加载。
						bool bExplicitlyLoaded = false;
						// 如果插件未设置 ExplicitlyLoaded=true，则输出警告，因为 GameFeature 通常应在项目启动后按需加载。
						if (!RawObject.TryGetBoolField("ExplicitlyLoaded", out bExplicitlyLoaded) || bExplicitlyLoaded == false)
						{
							// 输出插件缺少 ExplicitlyLoaded=true 的警告。
							Logger.LogWarning("GameFeaturePlugin {0}, does not set ExplicitlyLoaded to true. This is required for GameFeaturePlugins.", PluginFile.GetFileNameWithoutExtension());
						}
						
						// 这里可以读取项目自定义字段，例如插件所属发布版本。
						//string PluginReleaseVersion;
						// 如果插件声明了自定义发布版本，则可据此决定当前分支是否启用它。
						//if (RawObject.TryGetStringField("MyProjectReleaseVersion", out PluginReleaseVersion))
						//{
						//		// 根据插件发布版本、当前分支发布版本或构建全部插件标记(bBuildAllGameFeaturePlugins)计算启用状态。
						//		bEnabled = SomeFunctionOf(PluginReleaseVersion, CurrentReleaseVersion) || bBuildAllGameFeaturePlugins;
						//}
						// 如果当前模式要求构建所有 GameFeature 插件，则尝试启用该插件。
						if (bBuildAllGameFeaturePlugins)
						{
							bEnabled = true;

						}
						// 读取 EditorOnly 字段，用于阻止编辑器专用插件进入非编辑器构建。
						bool bEditorOnly = false;
						// 如果插件声明了 EditorOnly 字段，则按字段值判断是否需要禁用。
						if (RawObject.TryGetBoolField("EditorOnly", out bEditorOnly))
						{
							// 非 Editor Target 在未强制构建全部插件时，不应编译编辑器专用插件。
							if (bEditorOnly && (Target.Type != TargetType.Editor) && !bBuildAllGameFeaturePlugins)
							{
								// 标记插件为强制禁用。
								bForceDisabled = true;
							}
						}
						else
						{
							// 插件没有声明 EditorOnly 时，不做额外处理。
						}
						// 一些插件只允许在特定分支内使用
						// 声明可选的分支限制字段，用于只在指定分支启用插件。
						string RestrictToBranch;
						// 如果插件声明 RestrictToBranch，则比较它和当前构建分支。
						if (RawObject.TryGetStringField("RestrictToBranch", out RestrictToBranch))
						{
							// 如果当前分支与插件限制分支不一致，则禁用该插件。
							if (!Target.Version.BranchName.Equals(RestrictToBranch, StringComparison.OrdinalIgnoreCase))
							{
								// 标记插件为强制禁用。
								bForceDisabled = true;
								// 输出调试日志，说明插件被禁用是因为分支限制不匹配。
								Logger.LogDebug("GameFeaturePlugin {Name} was marked as restricted to other branches. Disabling.", PluginFile.GetFileNameWithoutExtension());
							}
							else
							{
								// 输出调试日志，说明插件限制分支与当前分支匹配，因此保留当前启用决策。
								Logger.LogDebug("GameFeaturePlugin {Name} was marked as restricted to this branch. Leaving enabled.", PluginFile.GetFileNameWithoutExtension());
							}
						}
						
						// 声明 NeverBuild 字段，用于让插件无条件退出编译。
						bool bNeverBuild = false;
						// 如果插件设置 NeverBuild=true，则无论前面是否启用都强制禁用。
						if (RawObject.TryGetBoolField("NeverBuild", out bNeverBuild) && bNeverBuild)
						{
							// 标记插件为强制禁用。
							bForceDisabled = true;
							// 输出调试日志，说明插件因为 NeverBuild 被禁用。
							Logger.LogDebug("GameFeaturePlugin {Name} was marked as NeverBuild, disabling.", PluginFile.GetFileNameWithoutExtension());
						}
						// 保留插件引用信息，以便后续进行校验
						// 声明插件依赖数组，用于读取 .uplugin 中的 Plugins 字段。
						JsonObject[] PluginReferencesArray;
						// 如果插件描述文件包含 Plugins 依赖列表，则收集其中启用的依赖名称。
						if (RawObject.TryGetObjectArrayField("Plugins", out PluginReferencesArray))
						{
							// 遍历每一个插件依赖声明对象。
							foreach (JsonObject ReferenceObject in PluginReferencesArray)
							{
								// 默认认为该依赖未启用。
								bool bRefEnabled = false;
								// 只处理 Enabled=true 的依赖项。
								if (ReferenceObject.TryGetBoolField("Enabled", out bRefEnabled) && bRefEnabled == true)
								{
									// 声明依赖插件名称变量。
									string PluginReferenceName;
									// 尝试读取依赖项的 Name 字段。
									if (ReferenceObject.TryGetStringField("Name", out PluginReferenceName))
									{
										// 使用当前插件文件名作为依赖发起方名称。
										string ReferencerName = PluginFile.GetFileNameWithoutExtension();
										// 如果还没有当前插件的依赖列表，则创建一个新的列表。
										if (!AllPluginReferencesByName.ContainsKey(ReferencerName))
										{
											// 为当前插件初始化依赖名称列表。
											AllPluginReferencesByName[ReferencerName] = new List<string>();
										}

										// 将读取到的依赖插件名记录到当前插件的依赖列表中。
										AllPluginReferencesByName[ReferencerName].Add(PluginReferenceName);
									}
								}
							}
						}




					}
					catch (Exception ParseException)
					{
						// 输出解析失败警告，并附带异常消息方便定位问题插件。
						Logger.LogWarning("Failed to parse GameFeaturePlugin file {Name}, disabling. Exception: {1}", PluginFile.GetFileNameWithoutExtension(), ParseException.Message);
						// 解析失败的插件不应继续参与构建，因此强制禁用。
						bForceDisabled = true;
					}
					// 强制禁用优先级高于启用决策。
					if (bForceDisabled)
					{
						// 如果插件被强制禁用，则清除启用标记。
						bEnabled = false;
					}
					
					// 输出该插件最终的 enable/disable/ignore 决策，便于调试构建行为。
					Logger.LogDebug("ConfigureGameFeaturePlugins() has decided to {Action} feature {Name}", bEnabled ? "enable" : (bForceDisabled ? "disable" : "ignore"), PluginFile.GetFileNameWithoutExtension());
					
					// 根据最终决策把插件加入 Target 的启用或禁用列表。
					if (bEnabled)
					{
						// 将插件名加入启用列表，UBT 会为当前 Target 编译/启用该插件。
						Target.EnablePlugins.Add(PluginFile.GetFileNameWithoutExtension());
					}
					// 如果没有启用但被强制禁用，则把它加入禁用列表。
					else if (bForceDisabled)
					{
						// 将插件名加入禁用列表，确保当前 Target 不编译/加载该插件。
						Target.DisablePlugins.Add(PluginFile.GetFileNameWithoutExtension());
					}

				}

			}
			// 如果你使用了类似发布版本号这样的机制，建议进行一次引用校验，
			// 以确保发布版本更早的插件，不会依赖于发布版本更晚的内容。
		}

	}
}
