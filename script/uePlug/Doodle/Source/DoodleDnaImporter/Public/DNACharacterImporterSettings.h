
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InterchangeGenericMaterialPipeline.h"

#include "DNACharacterImporterSettings.generated.h"

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "DNA Character Importer"))
class DOODLEDNAIMPORTER_API UDNACharacterImporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDNACharacterImporterSettings();

	virtual FName GetCategoryName() const override;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportSkeletalMeshes = true;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportMaterials = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	EInterchangeMaterialSearchLocation MaterialSearchLocation = EInterchangeMaterialSearchLocation::UnderRoot;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportTextures = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportAnimations = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bImportLods = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bCreatePhysicsAsset = true;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bKeepSectionsSeparate = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bUseFullPrecisionUVs = true;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bRecomputeNormals = false;

	UPROPERTY(EditAnywhere, config, Category = "Import")
	bool bRecomputeTangents = false;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	bool bGenerateMorphTargetsFromFbx = true;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets", meta = (ClampMin = "0.0"))
	float MorphTargetDeltaThreshold = 0.00001f;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	float MorphTargetBuildSettingsThreshold = 0.00001f;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	bool bImportMorphTargetNormals = false;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets", meta = (ClampMin = "0.0"))
	float MorphMappingTolerance = 0.005f;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets", meta = (ClampMin = "16"))
	int32 MorphMappingSampleCount = 1024;

	UPROPERTY(EditAnywhere, config, Category = "Morph Targets")
	bool bFailOnUnmappedMeshPart = false;

	UPROPERTY(EditAnywhere, config, Category = "Diagnostics")
	bool bLogMemoryTrace = true;

	UPROPERTY(EditAnywhere, config, Category = "Diagnostics")
	bool bVerifyGeneratedMorphTargets = true;

	UPROPERTY(EditAnywhere, config, Category = "Physics Asset", meta = (ClampMin = "0.0"))
	float PhysicsAssetMinBoneSize = 20.0f;

	UPROPERTY(EditAnywhere, config, Category = "Physics Asset", meta = (ClampMin = "0"))
	int32 PhysicsAssetLodIndex = 0;
};
