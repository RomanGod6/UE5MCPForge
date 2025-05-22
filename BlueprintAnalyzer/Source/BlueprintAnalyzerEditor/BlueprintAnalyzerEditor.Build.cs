using UnrealBuildTool;

public class BlueprintAnalyzerEditor : ModuleRules
{
	public BlueprintAnalyzerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"BlueprintAnalyzer"
		});
        
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"UnrealEd",
			"BlueprintGraph",
			"KismetCompiler",
			"AssetRegistry",
			"EditorStyle",
			"WorkspaceMenuStructure",
			"ToolMenus"
		});
	}
}