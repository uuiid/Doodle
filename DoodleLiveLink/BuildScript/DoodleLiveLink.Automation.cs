// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Core;
using Microsoft.Extensions.Logging;
using System.Collections.Generic;
using System.Linq;

namespace AutomationTool
{
	// DoodleLiveLink 是 in-tree 编辑器程序，没有游戏目标，无法用通用的 MakeCookedEditor（会报 "Game target not found"）。
	// 这里仿照 LiveLinkHub 的 MakeLiveLinkHubEditor，覆盖 MakeParams 以避免游戏目标自动探测。
	class MakeDoodleLiveLinkEditor : MakeCookedEditor
	{
		protected override ProjectParams MakeParams(string DLCName, string BasedOnReleaseVersion)
		{
			ProjectParams Params = new ProjectParams(
				Command: this
				, RawProjectPath: ProjectFile
				, NoBootstrapExe: true
				, DLCName: DLCName
				, BasedOnReleaseVersion: BasedOnReleaseVersion
				, DedicatedServer: bIsCookedCooker
				, NoClient: bIsCookedCooker
				, OptionalContent: true
				, ClientCookedTargets: new ParamList<string>() // Prevent AutodetectSettings from looking for a game target
				, EditorTargets: new ParamList<string>("DoodleLiveLink")
				, UbtArgs: "-SingleModulePlatform"
			);

			return Params;
		}

		protected override void ModifyParams(ProjectParams BuildParams)
		{
			base.ModifyParams(BuildParams);

			// We don't want the SDK dir / CookerSupportFiles
			BuildParams.CookerSupportFilesSubdirectory = null;
		}
	}
}
