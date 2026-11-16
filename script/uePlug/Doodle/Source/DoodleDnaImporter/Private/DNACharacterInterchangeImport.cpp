// Copyright Epic Games, Inc. All Rights Reserved.

#include "DNACharacterInterchangeImport.h"

#include "DNACharacterImporterModule.h"
#include "DNACharacterImporterSettings.h"

#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "InterchangeGenericAnimationPipeline.h"
#include "InterchangeGenericAssetsPipeline.h"
#include "InterchangeGenericAssetsPipelineSharedSettings.h"
#include "InterchangeGenericMaterialPipeline.h"
#include "InterchangeGenericMeshPipeline.h"
#include "InterchangeGenericTexturePipeline.h"
#include "InterchangeManager.h"
#include "InterchangeSourceData.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace DNACharacterImporterInternal
{
	namespace
	{
		/** The engine pipeline that drives FBX -> mesh description translation. */
		const TCHAR* DefaultAssetsPipelinePath = TEXT("/Interchange/Pipelines/DefaultAssetsPipeline");

		/**
		 * Changes the settings of the shared default assets pipeline for the duration of one import and
		 * puts everything (including the dirty flag of the pipeline asset) back afterwards.
		 */
		struct FScopedPipelineSettings
		{
			explicit FScopedPipelineSettings(UInterchangeGenericAssetsPipeline& InPipeline)
				: Pipeline(InPipeline)
				, CommonMeshes(*InPipeline.CommonMeshesProperties)
				, MeshPipeline(*InPipeline.MeshPipeline)
				, SkeletalMeshes(*InPipeline.CommonSkeletalMeshesAndAnimationsProperties)
				, AnimationPipeline(InPipeline.AnimationPipeline)
				, MaterialPipeline(InPipeline.MaterialPipeline)
			{
				bKeepSectionsSeparate = CommonMeshes.bKeepSectionsSeparate;
				bImportLods = CommonMeshes.bImportLods;
				bUseFullPrecisionUVs = CommonMeshes.bUseFullPrecisionUVs;
				bRecomputeNormals = CommonMeshes.bRecomputeNormals;
				bRecomputeTangents = CommonMeshes.bRecomputeTangents;
				bComputeWeightedNormals = CommonMeshes.bComputeWeightedNormals;

				bImportSkeletalMeshes = MeshPipeline.bImportSkeletalMeshes;
				bImportStaticMeshes = MeshPipeline.bImportStaticMeshes;
				bImportMorphTargets = MeshPipeline.bImportMorphTargets;
				bCreatePhysicsAsset = MeshPipeline.bCreatePhysicsAsset;
				MorphThresholdPosition = MeshPipeline.MorphThresholdPosition;

				Skeleton = SkeletalMeshes.Skeleton;
				bAddCurveMetadataToSkeleton = SkeletalMeshes.bAddCurveMetadataToSkeleton;

				if (AnimationPipeline != nullptr)
				{
					bImportAnimations = AnimationPipeline->bImportAnimations;
				}
				if (MaterialPipeline != nullptr)
				{
					bImportMaterials = MaterialPipeline->bImportMaterials;
					TexturePipeline = MaterialPipeline->TexturePipeline;
					if (TexturePipeline != nullptr)
					{
						bImportTextures = TexturePipeline->bImportTextures;
					}
				}

				if (UPackage* Package = Pipeline.GetOutermost())
				{
					bPackageWasDirty = Package->IsDirty();
				}
			}

			~FScopedPipelineSettings()
			{
				CommonMeshes.bKeepSectionsSeparate = bKeepSectionsSeparate;
				CommonMeshes.bImportLods = bImportLods;
				CommonMeshes.bUseFullPrecisionUVs = bUseFullPrecisionUVs;
				CommonMeshes.bRecomputeNormals = bRecomputeNormals;
				CommonMeshes.bRecomputeTangents = bRecomputeTangents;
				CommonMeshes.bComputeWeightedNormals = bComputeWeightedNormals;

				MeshPipeline.bImportSkeletalMeshes = bImportSkeletalMeshes;
				MeshPipeline.bImportStaticMeshes = bImportStaticMeshes;
				MeshPipeline.bImportMorphTargets = bImportMorphTargets;
				MeshPipeline.bCreatePhysicsAsset = bCreatePhysicsAsset;
				MeshPipeline.MorphThresholdPosition = MorphThresholdPosition;

				SkeletalMeshes.Skeleton = Skeleton;
				SkeletalMeshes.bAddCurveMetadataToSkeleton = bAddCurveMetadataToSkeleton;

				if (AnimationPipeline != nullptr)
				{
					AnimationPipeline->bImportAnimations = bImportAnimations;
				}
				if (MaterialPipeline != nullptr)
				{
					MaterialPipeline->bImportMaterials = bImportMaterials;
				}
				if (TexturePipeline != nullptr)
				{
					TexturePipeline->bImportTextures = bImportTextures;
				}

				if (UPackage* Package = Pipeline.GetOutermost())
				{
					Package->SetDirtyFlag(bPackageWasDirty);
				}
			}

			UInterchangeGenericAssetsPipeline& Pipeline;
			UInterchangeGenericCommonMeshesProperties& CommonMeshes;
			UInterchangeGenericMeshPipeline& MeshPipeline;
			UInterchangeGenericCommonSkeletalMeshesAndAnimationsProperties& SkeletalMeshes;
			TObjectPtr<UInterchangeGenericAnimationPipeline> AnimationPipeline = nullptr;
			TObjectPtr<UInterchangeGenericMaterialPipeline> MaterialPipeline = nullptr;
			TObjectPtr<UInterchangeGenericTexturePipeline> TexturePipeline = nullptr;

		private:
			bool bKeepSectionsSeparate = true;
			bool bImportLods = true;
			bool bUseFullPrecisionUVs = true;
			bool bRecomputeNormals = true;
			bool bRecomputeTangents = true;
			bool bComputeWeightedNormals = false;

			bool bImportSkeletalMeshes = true;
			bool bImportStaticMeshes = true;
			bool bImportMorphTargets = true;
			bool bCreatePhysicsAsset = true;
			float MorphThresholdPosition = 0.015f;

			TWeakObjectPtr<USkeleton> Skeleton = nullptr;
			bool bAddCurveMetadataToSkeleton = false;

			bool bImportAnimations = true;
			bool bImportMaterials = true;
			bool bImportTextures = true;

			bool bPackageWasDirty = false;
		};
	}

	bool ImportSkeletalMeshFromFbx(
		const FDNACharacterImportRequest& Request,
		const UDNACharacterImporterSettings& Settings,
		bool bImportMorphTargets,
		FImportedMeshInfo& OutInfo,
		FString& OutError)
	{
		if (!FPackageName::IsValidLongPackageName(Request.DestinationPath))
		{
			OutError = FString::Printf(TEXT("'%s' is not a valid content path."), *Request.DestinationPath);
			return false;
		}

		UInterchangeGenericAssetsPipeline* Pipeline =
			LoadObject<UInterchangeGenericAssetsPipeline>(nullptr, DefaultAssetsPipelinePath);
		if (Pipeline == nullptr)
		{
			OutError = FString::Printf(TEXT("The Interchange pipeline '%s' could not be loaded."), DefaultAssetsPipelinePath);
			return false;
		}

		USkeleton* Skeleton = nullptr;
		if (!Request.SkeletonPath.IsEmpty())
		{
			Skeleton = LoadObject<USkeleton>(nullptr, *Request.SkeletonPath);
			if (Skeleton == nullptr)
			{
				OutError = FString::Printf(TEXT("Skeleton '%s' could not be loaded."), *Request.SkeletonPath);
				return false;
			}
		}

		FScopedPipelineSettings PipelineSettings(*Pipeline);
		PipelineSettings.CommonMeshes.bKeepSectionsSeparate = Settings.bKeepSectionsSeparate;
		PipelineSettings.CommonMeshes.bImportLods = Settings.bImportLods;
		PipelineSettings.CommonMeshes.bUseFullPrecisionUVs = Settings.bUseFullPrecisionUVs;
		PipelineSettings.CommonMeshes.bRecomputeNormals = Settings.bRecomputeNormals;
		PipelineSettings.CommonMeshes.bRecomputeTangents = Settings.bRecomputeTangents;
		PipelineSettings.CommonMeshes.bComputeWeightedNormals = false;

		PipelineSettings.MeshPipeline.bImportStaticMeshes = false;
		PipelineSettings.MeshPipeline.bImportSkeletalMeshes = Settings.bImportSkeletalMeshes;
		PipelineSettings.MeshPipeline.bImportMorphTargets = bImportMorphTargets;
		// Interchange would build the physics asset from the mesh as imported, i.e. before this plugin
		// writes the blend shapes into its mesh description. The plugin creates it afterwards instead
		// (see DNACharacterPhysicsAsset.h), from the mesh that is actually saved.
		PipelineSettings.MeshPipeline.bCreatePhysicsAsset = false;
		// Interchange copies this into the LOD build settings of the mesh; it must not filter the
		// deltas of DNA blend shapes away.
		PipelineSettings.MeshPipeline.MorphThresholdPosition = Settings.MorphTargetBuildSettingsThreshold;

		PipelineSettings.SkeletalMeshes.Skeleton = Skeleton;
		PipelineSettings.SkeletalMeshes.bAddCurveMetadataToSkeleton = false;

		if (UInterchangeGenericAnimationPipeline* AnimationPipeline = PipelineSettings.AnimationPipeline)
		{
			AnimationPipeline->bImportAnimations = Settings.bImportAnimations;
		}
		if (UInterchangeGenericMaterialPipeline* MaterialPipeline = PipelineSettings.MaterialPipeline)
		{
			MaterialPipeline->bImportMaterials = Settings.bImportMaterials;
			if (UInterchangeGenericTexturePipeline* TexturePipeline = PipelineSettings.TexturePipeline)
			{
				TexturePipeline->bImportTextures = Settings.bImportTextures;
			}
		}

		const FString AssetName = Request.AssetName.IsEmpty()
			? FPaths::GetBaseFilename(Request.FbxFilename)
			: Request.AssetName;

		UE::Interchange::FScopedSourceData SourceData(Request.FbxFilename);
		if (SourceData.GetSourceData() == nullptr)
		{
			OutError = FString::Printf(TEXT("The FBX file '%s' could not be read."), *Request.FbxFilename);
			return false;
		}

		FImportAssetParameters ImportParameters;
		ImportParameters.bIsAutomated = true;
		ImportParameters.bFollowRedirectors = false;
		ImportParameters.bReplaceExisting = Request.bReplaceExisting;
		ImportParameters.bForceShowDialog = false;
		ImportParameters.DestinationName = AssetName;
		ImportParameters.OverridePipelines.Add(FSoftObjectPath(Pipeline));

		TArray<UObject*> ImportedObjects;
		UInterchangeManager& InterchangeManager = UInterchangeManager::GetInterchangeManager();
		InterchangeManager.ImportAsset(Request.DestinationPath, SourceData.GetSourceData(), ImportParameters, ImportedObjects);

		USkeletalMesh* ImportedMesh = nullptr;
		for (UObject* ImportedObject : ImportedObjects)
		{
			if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(ImportedObject))
			{
				ImportedMesh = SkeletalMesh;
				break;
			}
		}

		if (ImportedMesh == nullptr)
		{
			OutError = FString::Printf(TEXT("Interchange did not create a skeletal mesh from '%s' (imported objects: %d)."),
				*Request.FbxFilename, ImportedObjects.Num());
			return false;
		}

		OutInfo.MeshObjectPath = ImportedMesh->GetPathName();
		if (const USkeleton* MeshSkeleton = ImportedMesh->GetSkeleton())
		{
			OutInfo.SkeletonObjectPath = MeshSkeleton->GetPathName();
		}

		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("Interchange imported '%s' -> %s (morph targets: %s)"),
			*Request.FbxFilename, *OutInfo.MeshObjectPath, bImportMorphTargets ? TEXT("engine") : TEXT("plugin"));
		return true;
	}
}
