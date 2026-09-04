// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Core;
using UnrealBuildTool;

public class DoodleLiveLinkTarget : TargetRules
{
	// Restrict OptedInModulePlatforms to the current Target.Platform.
	// Used during staging, which otherwise fails in TargetPlatform-related restricted
	// subdirectories (Engine/Binaries/Win64/{Android,IOS,Linux,LinuxArm64,...}).
	[CommandLine("-SingleModulePlatform")]
	public bool bSingleModulePlatform = false;

	public DoodleLiveLinkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		bExplicitTargetForType = true;
		bGenerateProgramProject = true;

		LaunchModuleName = "DoodleLiveLinkLauncher";

		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		GeneratedProjectName = "DoodleLiveLink";

		// LiveLink 接收与 WebSocket 转发所需插件
		EnablePlugins.AddRange(new string[]
		{
			"LiveLink",
			"MetaHumanLiveLink",
			"UdpMessaging",
			"WebSocketNetworking",
		});

		if (bSingleModulePlatform)
		{
			// Necessary for staging, but avoided otherwise because it dirties
			// Definitions.CookedEditor.h and triggers rebuilds (incl. UnrealEditor).
			OptedInModulePlatforms = new UnrealTargetPlatform[] { Target.Platform };
		}

		bAllowEnginePluginsEnabledByDefault = false;
		bBuildAdditionalConsoleApp = false;

		OutputFile = "Binaries/" + Platform.ToString() + "/DoodleLiveLink";
		if (Configuration != UndecoratedConfiguration)
		{
			OutputFile += "-" + Platform.ToString() + "-" + Configuration.ToString();
		}
		if (Platform == UnrealTargetPlatform.Win64)
		{
			OutputFile += ".exe";
		}
	}
}
