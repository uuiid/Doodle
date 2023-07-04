// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoodleAbc : ModuleRules
{
    public DoodleAbc(ReadOnlyTargetRules Target) : base(Target)
	{
        //PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
                "UnrealEd",
                "GeometryCache",
                "AlembicLib",
                "MeshUtilities",
                "MaterialUtilities",
                "PropertyEditor",
                "SlateCore",
                "Slate",
                "Eigen",
                "RenderCore",
                "RHI"
			}
		);

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "MeshDescription",
				"StaticMeshDescription",
            }
        );

    }
}
