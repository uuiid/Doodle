#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

#include "DNACharacterImporterTypes.h"

class UDNACharacterImporterSettings;
class USkeletalMesh;

namespace DNACharacterImporterInternal
{
	enum class EMorphGenerationError : uint8
	{
		NoMeshDescription,
		FbxUnreadable,
		NoSourceGeometryParts,
		UnmappedMeshParts,
	};

	struct FMorphGenerationError
	{
		EMorphGenerationError Code = EMorphGenerationError::NoMeshDescription;
		FString Detail;
	};

	struct FMorphGenerationStats
	{
		int32 ShapeCount = 0;
		int32 MorphTargetCount = 0;
		int32 MorphTargetsWithData = 0;
		int32 MeshPartCount = 0;
		int32 MappedMeshPartCount = 0;
		int32 VertexCount = 0;
		int64 DeltaCount = 0;
		TArray<FString> Log;
	};

	TValueOrError<FMorphGenerationStats, FMorphGenerationError> GenerateMorphTargets(
		USkeletalMesh& InMesh,
		const FDNACharacterImportRequest& InRequest,
		const UDNACharacterImporterSettings& InSettings);
}
