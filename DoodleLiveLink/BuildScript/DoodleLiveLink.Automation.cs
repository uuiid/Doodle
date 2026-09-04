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

		protected override void ModifyDeploymentContext(ProjectParams Params, DeploymentContext SC)
		{
			ModifyStageContext Context = CreateContext(Params, SC);

			DefaultModifyDeploymentContext(Params, SC, Context);

			Context.Apply(SC);

			string PlatName = SC.StageTargetPlatform.PlatformType.ToString();

			// Copy .target receipt to project and engine bin
			SC.FilesToStage.NonUFSFiles.Add(
				new StagedFileReference($"{Context.ProjectName}/Binaries/{PlatName}/{Context.ProjectName}.target"),
				new FileReference($"Engine/Binaries/{PlatName}/{Context.ProjectName}.target"));

			SC.FilesToStage.NonUFSFiles.Add(
				new StagedFileReference($"Engine/Binaries/{PlatName}/{Context.ProjectName}.target"),
				new FileReference($"Engine/Binaries/{PlatName}/{Context.ProjectName}.target"));

			// Stage TargetInfo and target script. Necessary to avoid "Running incorrect executable for target (...)"
			SC.FilesToStage.NonUFSFiles.Add(
				new StagedFileReference($"{Context.ProjectName}/Source/{Context.ProjectName}.Target.cs"),
				new FileReference($"Engine/Source/Programs/{Context.ProjectName}/Source/{Context.ProjectName}.Target.cs"));

			SC.FilesToStage.NonUFSFiles.Add(
				new StagedFileReference($"{Context.ProjectName}/Intermediate/TargetInfo.json"),
				new FileReference($"Engine/Source/Programs/{Context.ProjectName}/Intermediate/TargetInfo.json"));

			// Move PDBs to debug (otherwise -nodebuginfo doesn't exclude them and staging fails on missing third-party .pdb)
			SC.FilesToStage.NonUFSDebugFiles.Union(SC.FilesToStage.NonUFSFiles.Where(x => x.Key.HasExtension(".pdb")));

			// Remove any files from NonUFS that were also added to Debug
			SC.FilesToStage.NonUFSFiles = SC.FilesToStage.NonUFSFiles.Where(x => !SC.FilesToStage.NonUFSDebugFiles.ContainsKey(x.Key)).ToDictionary(x => x.Key, x => x.Value);

			// Make sure there are no restricted folders in the output
			HashSet<StagedFileReference> RestrictedFiles = new HashSet<StagedFileReference>();
			foreach (string RestrictedName in SC.RestrictedFolderNames)
			{
				RestrictedFiles.UnionWith(SC.FilesToStage.NonUFSDebugFiles.Keys.Where(x => x.ContainsName(RestrictedName)));
			}
			foreach (StagedFileReference RestrictedFile in RestrictedFiles)
			{
				SC.FilesToStage.NonUFSDebugFiles.Remove(RestrictedFile);
			}
		}
	}
}
