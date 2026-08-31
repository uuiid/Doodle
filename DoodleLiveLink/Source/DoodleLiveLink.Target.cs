// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoodleLiveLinkTarget : TargetRules
{
	public DoodleLiveLinkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		bExplicitTargetForType = true;
		bGenerateProgramProject = true;

		LaunchModuleName = "DoodleLiveLinkLauncher";

		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// LiveLink 接收与 WebSocket 转发所需插件
		EnablePlugins.AddRange(new string[]
		{
			"LiveLink",
			"UdpMessaging",
		});

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
