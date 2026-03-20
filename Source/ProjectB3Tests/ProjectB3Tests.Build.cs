using UnrealBuildTool;

public class ProjectB3Tests : ModuleRules
{
	public ProjectB3Tests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"FunctionalTesting",
			"ProjectB3",
			"GameplayTags",
			"GameplayAbilities"
		});
	}
}
