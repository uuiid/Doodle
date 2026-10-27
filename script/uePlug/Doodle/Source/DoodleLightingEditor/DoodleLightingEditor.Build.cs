using UnrealBuildTool;

public class DoodleLightingEditor : ModuleRules
{
	public DoodleLightingEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DoodleLighting",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Json",
				"JsonUtilities",
				"UnrealEd",
				"AssetRegistry",
				"AssetTools",
				"EditorSubsystem",
				"EditorScriptingUtilities",
				"LevelSequence",
				"MovieScene",
				"MovieSceneTracks",
				"CinematicCamera",
				"Sequencer",
				"MovieSceneTools",
				"MovieRenderPipelineCore",
				"MovieRenderPipelineEditor",
				"MovieRenderPipelineRenderPasses",
				"MovieRenderPipelineSettings",
				"Imath",
				"UEOpenExr",
				"UEOpenExrRTTI",
				"FBX",
				"Projects",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"WorkspaceMenuStructure",
			}
		);
	}
}
