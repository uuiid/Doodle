// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoodleAbc : ModuleRules
{
    public DoodleAbc(ReadOnlyTargetRules Target) : base(Target)
	{
        CppStandard = CppStandardVersion.Cpp20;
        //PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
                "UnrealEd",
                "GeometryCache",
                "Imath",
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
				"EditorFramework",
                "UnrealEd",
                "MeshDescription",
				"StaticMeshDescription",
            }
        );

    }
}
