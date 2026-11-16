#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

#include "DNACharacterImporterTypes.h"

enum class EDNACharacterImporterRunError : uint8
{
	NotOnGameThread,
	FbxMissing,
	MeshImportFailed,
	MeshNotLoaded,
	MorphGenerationFailed,
	PhysicsAssetFailed,
};

struct DOODLEDNAIMPORTER_API FDNACharacterImporterRunError
{
	EDNACharacterImporterRunError Code = EDNACharacterImporterRunError::MeshImportFailed;
	FString Detail;
	TArray<FString> Log;
};

class DOODLEDNAIMPORTER_API FDNACharacterImporter
{
public:
	static TValueOrError<FDNACharacterImportResult, FDNACharacterImporterRunError> Run(
		const FDNACharacterImportRequest& InRequest);

	static void NormalizeRequest(FDNACharacterImportRequest& InOutRequest);

	static FString ModeToString(EDNACharacterImportMode InMode);

	static TValueOrError<EDNACharacterImportMode, FString> ParseMode(const FString& InText);
};
