using UnrealBuildTool;

public class LyraEditor : ModuleRules
{
    public LyraEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(new string[]{"LyraEditor"});
        
        PrivateIncludePaths.AddRange(new string[]{});

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",                
                "CoreUObject",
                "Engine",
                "LyraGame"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore",
                "Slate",
                "SlateCore"
            }
        );
    }
}