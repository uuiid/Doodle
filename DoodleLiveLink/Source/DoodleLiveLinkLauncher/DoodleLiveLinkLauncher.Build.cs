// Copyright Epic Games, Inc. All Rights Reserved.

#nullable enable

using System;
using System.Collections.Generic;
using UnrealBuildTool;

public class DoodleLiveLinkLauncher : ModuleRules
{
	public DoodleLiveLinkLauncher(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.Add("DoodleLiveLink");

		// LaunchEngineLoop dependencies
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"ApplicationCore",
				"AssetRegistry",
				"Core",
				"CoreUObject",
				"DesktopPlatform",
				"Engine",
				"InputCore",
				"InstallBundleManager",
				"MediaUtils",
				"Messaging",
				"MoviePlayer",
				"MoviePlayerProxy",
				"ProfileVisualizer",
				"Projects",
				"PreLoadScreen",
				"PIEPreviewDeviceProfileSelector",
				"RenderCore",
				"RHI",
				"Slate",
				"SlateCore",
				"StandaloneRenderer",
				"TraceLog",
			}
		);

		// LaunchEngineLoop IncludePath dependencies
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Launch",
				"AutomationWorker",
				"AutomationController",
				"AutomationTest",
				"DerivedDataCache",
				"HeadMountedDisplay",
				"MRMesh",
				"SlateRHIRenderer",
				"SlateNullRenderer",
			}
		);

		// LaunchEngineLoop editor dependencies
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"DerivedDataCache",
				"ToolWidgets",
				"UnrealEd",
			});

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"AutomationController",
					"AutomationTest",
					"AutomationWorker",
				});
		}

		ShortName = "DLHub";
	}
}
