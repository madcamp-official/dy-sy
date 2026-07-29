// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
using UnrealBuildTool;

public class TextAnimator : ModuleRules
{
	public TextAnimator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Projects",
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
