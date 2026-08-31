// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoodleLiveLink : ModuleRules
{
	public DoodleLiveLink(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"ApplicationCore",
			"LiveLink",
			"LiveLinkInterface",
			"Networking",
			"Sockets",
			"NetCore",
		});
	}
}
