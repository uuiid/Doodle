// Some copyright should be here...

using UnrealBuildTool;

public class BatchRender : ModuleRules
{
	public BatchRender(ReadOnlyTargetRules Target) : base(Target)
	{
		//PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "UnrealEd",
                "MovieScene",
				"MovieRenderPipelineCore",
                "MovieRenderPipelineEditor",
				"MovieRenderPipelineRenderPasses",
				"MovieRenderPipelineSettings",
                "MovieSceneTracks",
                "LevelSequence",
                "Engine",
                "ImageWriteQueue", // For debug tile writing
				"OpenColorIO",
                "CinematicCamera",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "InputCore",
                "RenderCore",
                "RHI",
                "UMG",
                "Landscape", // To flush grass
				"AudioMixer",
                "NonRealtimeAudioRenderer",
                "Sockets",
                "Networking",
                "HTTP",
                "DeveloperSettings",
                "ClothingSystemRuntimeInterface",
                "WebSockets",
                "Projects"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
