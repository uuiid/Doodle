// Copyright Epic Games, Inc. All Rights Reserved.

#nullable enable

using System;
using System.Collections.Generic;
using UnrealBuildTool;

public class DoodleLiveLinkLauncher : ModuleRules
{
	public DoodleLiveLinkLauncher(ReadOnlyTargetRules Target) : base(Target)
	{
		// 核心业务模块（LiveLink 接收 + WebSocket 转发）
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"LiveLink",
				"LiveLinkInterface",
				"LiveLinkComponents",
				"WebSockets",
				"Sockets",
				"Networking",
			});

		// LaunchEngineLoop 依赖
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

		// LaunchEngineLoop IncludePath 依赖
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

		// LaunchEngineLoop editor 依赖
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

		if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PrivateDependencyModuleNames.Add("AgilitySDK");
		}

		ShortName = "DLHub";
	}
}
