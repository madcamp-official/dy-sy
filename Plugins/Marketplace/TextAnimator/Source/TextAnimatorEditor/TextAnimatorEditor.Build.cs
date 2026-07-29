// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
using UnrealBuildTool;

public class TextAnimatorEditor : ModuleRules
{
	public TextAnimatorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ApplicationCore",
			"Slate",
			"SlateCore",
			"UMG",
			"UMGEditor",
			"UnrealEd",
			"WorkspaceMenuStructure",
			"MainFrame",
			"Projects",
			"PropertyEditor",
			"AssetRegistry",
			"TextAnimator"
		});
	}
}
