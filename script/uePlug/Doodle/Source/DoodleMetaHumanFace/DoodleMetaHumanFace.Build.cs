// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoodleMetaHumanFace : ModuleRules
{
    public DoodleMetaHumanFace(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicIncludePaths.AddRange(
            new string[] {
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "MetaHumanPerformance",
                "MetaHumanCoreTech",
                "MetaHumanCoreTechLib",
                "MetaHumanPipelineCore",
                "CaptureDataCore",
                "ImgMedia",
                "MediaAssets",
                "Projects",
                "Json",
                "JsonUtilities",
                "UnrealEd",
                "AssetRegistry",
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
            }
            );
    }
}