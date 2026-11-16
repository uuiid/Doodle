#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

#include "DNACharacterImporterTypes.h"

class UDNACharacterImporterSettings;

namespace DNACharacterImporterInternal
{
	enum class EInterchangeImportError : uint8
	{
		InvalidContentPath,
		PipelineMissing,
		SkeletonMissing,
		SourceDataUnreadable,
		MeshNotImported,
	};

	struct FInterchangeImportError
	{
		EInterchangeImportError Code = EInterchangeImportError::MeshNotImported;
		FString Detail;
	};

	struct FImportedMeshInfo
	{
		FString MeshObjectPath;
		FString SkeletonObjectPath;
	};

	TValueOrError<FImportedMeshInfo, FInterchangeImportError> ImportSkeletalMeshFromFbx(
		const FDNACharacterImportRequest& InRequest,
		const UDNACharacterImporterSettings& InSettings,
		bool bInImportMorphTargets);
}
