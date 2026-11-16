#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

class UDNACharacterImporterSettings;
class USkeletalMesh;

namespace DNACharacterImporterInternal
{
	enum class EPhysicsAssetError : uint8
	{
		PackageCreationFailed,
		NameOccupied,
		AssetCreationFailed,
		BodyGenerationFailed,
	};

	struct FPhysicsAssetError
	{
		EPhysicsAssetError Code = EPhysicsAssetError::BodyGenerationFailed;
		FString Detail;
	};

	TValueOrError<FString, FPhysicsAssetError> CreatePhysicsAssetForMesh(
		USkeletalMesh& InMesh, const UDNACharacterImporterSettings& InSettings);
}
