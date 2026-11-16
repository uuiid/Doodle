// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "DNACharacterImporterTypes.generated.h"

/** How a DNA character FBX should be imported. */
UENUM()
enum class EDNACharacterImportMode : uint8
{
	/**
	 * The engine's default Interchange behaviour: mesh, materials, animations and every
	 * morph target are imported in one pass. Very memory hungry for DNA characters
	 * (hundreds of blend shapes), this is the baseline the plugin is compared against.
	 */
	Stock UMETA(DisplayName = "Stock (engine default)"),

	/**
	 * Import the mesh with morph targets disabled and rebuild them from the FBX
	 * afterwards, one blend shape at a time. Much lower peak memory.
	 */
	LowMemory UMETA(DisplayName = "Low memory (plugin)"),

	/** Reuse an already imported mesh and only (re)generate its morph targets from the FBX. */
	MorphTargetsOnly UMETA(DisplayName = "Morph targets only"),
};

/** Everything needed to run one import. */
USTRUCT()
struct DOODLEDNAIMPORTER_API FDNACharacterImportRequest
{
	GENERATED_BODY()

	/** Absolute path of the FBX file on disk. */
	UPROPERTY(EditAnywhere, Category = "DNA")
	FString FbxFilename;

	/** Content path the asset is created in, for example "/Game/DNA". */
	UPROPERTY(EditAnywhere, Category = "DNA")
	FString DestinationPath = TEXT("/Game/DNA");

	/** Name of the created asset. Defaults to the FBX file name. */
	UPROPERTY(EditAnywhere, Category = "DNA")
	FString AssetName;

	UPROPERTY(EditAnywhere, Category = "DNA")
	EDNACharacterImportMode Mode = EDNACharacterImportMode::LowMemory;

	/** Delete/replace an existing asset with the same name before importing. */
	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bReplaceExisting = true;

	/** Save the created packages to disk. */
	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bSaveAssets = true;

	/** Optional existing skeleton to bind the mesh to (content path). */
	UPROPERTY(EditAnywhere, Category = "DNA")
	FString SkeletonPath;

	/**
	 * Report what an already imported mesh contains without changing it.
	 * Only meaningful with the MorphTargetsOnly mode, which loads an existing asset instead of importing one.
	 */
	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bVerifyOnly = false;

	/**
	 * Import the base geometry and stop there: no blend shape is turned into a morph target.
	 * This is the control case for checking whether a geometry problem comes from the import itself
	 * or from the morph target pass.
	 */
	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bSkipMorphGeneration = false;
};

/** What actually happened, plus the numbers we care about. */
USTRUCT()
struct DOODLEDNAIMPORTER_API FDNACharacterImportResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bSucceeded = false;

	UPROPERTY()
	FString Message;

	UPROPERTY()
	FString MeshObjectPath;

	UPROPERTY()
	FString SkeletonObjectPath;

	/** Physics asset created next to the mesh (empty when the import does not create one). */
	UPROPERTY()
	FString PhysicsAssetObjectPath;

	UPROPERTY()
	int32 VertexCount = 0;

	UPROPERTY()
	int32 TriangleCount = 0;

	UPROPERTY()
	int32 BoneCount = 0;

	UPROPERTY()
	int32 SectionCount = 0;

	/** Morph targets that exist on the mesh after the import. */
	UPROPERTY()
	int32 MorphTargetCount = 0;

	/** Morph targets of the FBX that carry at least one non zero delta. */
	UPROPERTY()
	int32 MorphTargetsWithData = 0;

	/** Blend shapes found in the FBX file. */
	UPROPERTY()
	int32 ShapeCount = 0;

	/** Mesh parts (geometries) found in the FBX file. */
	UPROPERTY()
	int32 MeshPartCount = 0;

	/** Sub meshes whose blend shapes could be matched back to the imported mesh. */
	UPROPERTY()
	int32 MappedMeshPartCount = 0;

	UPROPERTY()
	double ImportSeconds = 0.0;

	UPROPERTY()
	double MorphTargetSeconds = 0.0;

	/** Time spent generating the bodies of the physics asset. */
	UPROPERTY()
	double PhysicsAssetSeconds = 0.0;

	/** Peak physical memory of this process while the operation was running. */
	UPROPERTY()
	int64 ProcessPeakPhysicalBytes = 0;

	/** Process peak physical memory measured before the operation started. */
	UPROPERTY()
	int64 BaselinePeakPhysicalBytes = 0;

	/** Physical memory used when the operation finished. */
	UPROPERTY()
	int64 UsedPhysicalBytes = 0;

	/** Free text output, one line per event (also written to the log). */
	UPROPERTY()
	TArray<FString> Log;
};
