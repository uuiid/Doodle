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
			"InputCore",
			"LiveLink",
			"LiveLinkInterface",
			"LiveLinkFaceSource",
			"Networking",
			"Sockets",
			"NetCore",
			"WebSocketNetworking",
		});
	}
}
