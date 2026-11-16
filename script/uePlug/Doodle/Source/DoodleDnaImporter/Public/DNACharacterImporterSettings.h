// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "DNACharacterImporterSettings.generated.h"

/**
 * Project settings of the DNA Character Importer plugin.
 *
 * Project Settings > Plugins > DNA Character Importer.
 */
UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "DNA Character Importer"))
class DOODLEDNAIMPORTER_API UDNACharacterImporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDNACharacterImporterSettings();

	virtual FName GetCategoryName() const override;

	//
	// Import pipeline (engine Interchange settings used by this plugin)
	//

	/** Import the skeletal mesh itself. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportSkeletalMeshes = true;

	/** Import materials. Off by default: DNA characters usually bring hundreds of them. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportMaterials = false;

	/** Import textures. Off by default, see bImportMaterials. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportTextures = false;

	/** Import animations. Off by default: a DNA FBX contains one curve per blend shape. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportAnimations = false;

	/** Import every LOD found in the FBX. Off: only LOD0 is imported unless the file has more. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportLods = false;

	/**
	 * Create a physics asset next to the mesh (named "<MeshName>_PhysicsAsset").
	 * It is built by this plugin after the mesh has been rebuilt, so the bodies match the mesh that
	 * is actually saved - Interchange would build it from the pre-rebuild mesh.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bCreatePhysicsAsset = true;

	/** Keep every geometry section separate instead of merging them by material. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bKeepSectionsSeparate = true;

	/** Use full float precision for UVs. */
	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bUseFullPrecisionUVs = true;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bRecomputeNormals = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bRecomputeTangents = false;

	//
	// Morph targets
	//

	/** Rebuild the FBX blend shapes ourselves instead of letting Interchange do it. */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	bool bGenerateMorphTargetsFromFbx = true;

	/**
	 * Position delta threshold (cm) below which a blend shape delta is dropped.
	 * Keep this small: DNA facial shapes move vertices by fractions of a millimetre, and the
	 * skeletal mesh build settings default (0.015) silently deletes them.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets", meta = (ClampMin = "0.0"))
	float MorphTargetDeltaThreshold = 0.00001f;

	/** Value written to the LOD build settings so the mesh builder does not filter the deltas away. */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	float MorphTargetBuildSettingsThreshold = 0.00001f;

	/** Morph target normals. FBX blend shapes carry no normals, so this stays off. */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	bool bImportMorphTargetNormals = false;

	/**
	 * Maximum position error (relative to the sub mesh bounding box diagonal) accepted when
	 * matching a blend shape source mesh to a part of the imported mesh description.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets", meta = (ClampMin = "0.0"))
	float MorphMappingTolerance = 0.005f;

	/** Vertices sampled per sub mesh when fitting/validating the source mesh transform. */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets", meta = (ClampMin = "16"))
	int32 MorphMappingSampleCount = 1024;

	/** Fail the import when a sub mesh cannot be matched instead of only warning about it. */
	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	bool bFailOnUnmappedMeshPart = false;

	//
	// Diagnostics
	//

	/** Write a memory checkpoint table to the log after every step. */
	UPROPERTY(EditAnywhere, config, Category = "Diagnostics")
	bool bLogMemoryTrace = true;

	/** Verify the generated morph targets after the build and log a summary. */
	UPROPERTY(EditAnywhere, config, Category = "Diagnostics")
	bool bVerifyGeneratedMorphTargets = true;

	//
	// Physics asset
	//

	/**
	 * Bones shorter than this (cm) do not get a body of their own. 20 is the engine default and it
	 * matters here: a DNA character carries hundreds of tiny facial bones, and giving each of them
	 * a body would produce a huge, useless asset.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Physics Asset", meta = (ClampMin = "0.0"))
	float PhysicsAssetMinBoneSize = 20.0f;

	/** Mesh LOD the bodies are generated from. */
	UPROPERTY(EditAnywhere, config, Category = "Physics Asset", meta = (ClampMin = "0"))
	int32 PhysicsAssetLodIndex = 0;
};
