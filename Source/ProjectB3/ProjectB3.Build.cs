// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectB3 : ModuleRules
{
	public ProjectB3(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"DialogueSystemRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"FunctionalTesting",
			"GameplayDebugger",
			"DeveloperSettings"
		});
	}
}
