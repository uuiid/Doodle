// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "DNACharacterImporterTypes.h"

class UDNACharacterImporterSettings;
class USkeletalMesh;
class FDNAMemoryTrace;

namespace DNACharacterImporterInternal
{
	struct FMorphGenerationStats
	{
		/** Blend shape channels found in the FBX. */
		int32 ShapeCount = 0;
		/** Morph target attributes written into the mesh description. */
		int32 MorphTargetCount = 0;
		/** Morph targets that got at least one non zero delta. */
		int32 MorphTargetsWithData = 0;
		/** Sub meshes of the imported mesh. */
		int32 MeshPartCount = 0;
		/** Sub meshes that were matched to a mesh of the FBX. */
		int32 MappedMeshPartCount = 0;
		/** Vertices of the imported mesh description. */
		int32 VertexCount = 0;
	};

	/**
	 * Rebuilds the blend shapes of the FBX as morph targets of an already imported skeletal mesh.
	 *
	 * Nothing full sized is materialised per blend shape: the offsets are streamed out of the vendored
	 * ufbx parser one shape at a time and written straight into the morph attributes of the mesh
	 * description. This is what keeps the peak memory low compared to the engine importer, which builds
	 * a complete mesh description for every shape.
	 */
	bool GenerateMorphTargets(
		USkeletalMesh& Mesh,
		const FDNACharacterImportRequest& Request,
		const UDNACharacterImporterSettings& Settings,
		FMorphGenerationStats& OutStats,
		FDNAMemoryTrace* Trace,
		TArray<FString>& OutLog);
}
