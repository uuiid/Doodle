
#pragma once

#include "CoreMinimal.h"

#include "DNACharacterImporterTypes.generated.h"

UENUM()
enum class EDNACharacterImportMode : uint8
{
	Stock UMETA(DisplayName = "Stock (engine default)"),

	LowMemory UMETA(DisplayName = "Low memory (plugin)"),

	MorphTargetsOnly UMETA(DisplayName = "Morph targets only"),
};

USTRUCT()
struct DOODLEDNAIMPORTER_API FDNACharacterImportRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "DNA")
	FString FbxFilename;

	UPROPERTY(EditAnywhere, Category = "DNA")
	FString DestinationPath = TEXT("/Game/DNA");

	UPROPERTY(EditAnywhere, Category = "DNA")
	FString AssetName;

	UPROPERTY(EditAnywhere, Category = "DNA")
	EDNACharacterImportMode Mode = EDNACharacterImportMode::LowMemory;

	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bReplaceExisting = true;

	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bSaveAssets = true;

	UPROPERTY(EditAnywhere, Category = "DNA")
	FString SkeletonPath;

	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bVerifyOnly = false;

	UPROPERTY(EditAnywhere, Category = "DNA")
	bool bSkipMorphGeneration = false;
};

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

	UPROPERTY()
	int32 MorphTargetCount = 0;

	UPROPERTY()
	int32 MorphTargetsWithData = 0;

	UPROPERTY()
	int32 ShapeCount = 0;

	UPROPERTY()
	int32 MeshPartCount = 0;

	UPROPERTY()
	int32 MappedMeshPartCount = 0;

	UPROPERTY()
	int32 MorphCurveCount = 0;

	UPROPERTY()
	double ImportSeconds = 0.0;

	UPROPERTY()
	double MorphTargetSeconds = 0.0;

	UPROPERTY()
	double PhysicsAssetSeconds = 0.0;

	UPROPERTY()
	int64 ProcessPeakPhysicalBytes = 0;

	UPROPERTY()
	int64 BaselinePeakPhysicalBytes = 0;

	UPROPERTY()
	int64 UsedPhysicalBytes = 0;

	UPROPERTY()
	TArray<FString> Log;
};
