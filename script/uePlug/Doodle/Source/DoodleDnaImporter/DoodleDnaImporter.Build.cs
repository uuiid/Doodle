// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoodleDnaImporter : ModuleRules
{
	public DoodleDnaImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		bEnableExceptions = true;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"InterchangePipelines",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetRegistry",
			"EditorSubsystem",
			"Projects",
			"InterchangeCore",
			"InterchangeEngine",
			"InterchangeImport",
			"InterchangeNodes",
			"MeshDescription",
			"SkeletalMeshDescription",
			"StaticMeshDescription",
			"PhysicsUtilities",
			"SkeletalMeshUtilitiesCommon",
		});

		PrivateIncludePaths.Add(ModuleDirectory + "/Private/ufbx");
		PrivateDefinitions.Add("_UCRT_LEGACY_INFINITY");
	}
}
