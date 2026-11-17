
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
		const TCHAR* DefaultAssetsPipelinePath = TEXT("/Interchange/Pipelines/DefaultAssetsPipeline");

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
					SearchLocation = MaterialPipeline->SearchLocation;
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
					MaterialPipeline->SearchLocation = SearchLocation;
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
			EInterchangeMaterialSearchLocation SearchLocation = EInterchangeMaterialSearchLocation::Local;
			bool bImportTextures = true;

			bool bPackageWasDirty = false;
		};
	}

	TValueOrError<FImportedMeshInfo, FInterchangeImportError> ImportSkeletalMeshFromFbx(
		const FDNACharacterImportRequest& InRequest,
		const UDNACharacterImporterSettings& InSettings,
		bool bInImportMorphTargets)
	{
		if (!FPackageName::IsValidLongPackageName(InRequest.DestinationPath))
		{
			return MakeError(FInterchangeImportError{
				EInterchangeImportError::InvalidContentPath,
				FString::Printf(TEXT("'%s' is not a valid content path."), *InRequest.DestinationPath)
			});
		}

		UInterchangeGenericAssetsPipeline* Pipeline =
			LoadObject<UInterchangeGenericAssetsPipeline>(nullptr, DefaultAssetsPipelinePath);
		if (Pipeline == nullptr)
		{
			return MakeError(FInterchangeImportError{
				EInterchangeImportError::PipelineMissing,
				FString::Printf(TEXT("The Interchange pipeline '%s' could not be loaded."), DefaultAssetsPipelinePath)
			});
		}

		USkeleton* Skeleton = nullptr;
		if (!InRequest.SkeletonPath.IsEmpty())
		{
			Skeleton = LoadObject<USkeleton>(nullptr, *InRequest.SkeletonPath);
			if (Skeleton == nullptr)
			{
				return MakeError(FInterchangeImportError{
					EInterchangeImportError::SkeletonMissing,
					FString::Printf(TEXT("Skeleton '%s' could not be loaded."), *InRequest.SkeletonPath)
				});
			}
		}

		FScopedPipelineSettings PipelineSettings(*Pipeline);
		PipelineSettings.CommonMeshes.bKeepSectionsSeparate = InSettings.bKeepSectionsSeparate;
		PipelineSettings.CommonMeshes.bImportLods = InSettings.bImportLods;
		PipelineSettings.CommonMeshes.bUseFullPrecisionUVs = InSettings.bUseFullPrecisionUVs;
		PipelineSettings.CommonMeshes.bRecomputeNormals = InSettings.bRecomputeNormals;
		PipelineSettings.CommonMeshes.bRecomputeTangents = InSettings.bRecomputeTangents;
		PipelineSettings.CommonMeshes.bComputeWeightedNormals = false;

		PipelineSettings.MeshPipeline.bImportStaticMeshes = false;
		PipelineSettings.MeshPipeline.bImportSkeletalMeshes = InSettings.bImportSkeletalMeshes;
		PipelineSettings.MeshPipeline.bImportMorphTargets = bInImportMorphTargets;
		PipelineSettings.MeshPipeline.bCreatePhysicsAsset = false;
		PipelineSettings.MeshPipeline.MorphThresholdPosition = InSettings.MorphTargetBuildSettingsThreshold;

		PipelineSettings.SkeletalMeshes.Skeleton = Skeleton;
		PipelineSettings.SkeletalMeshes.bAddCurveMetadataToSkeleton = false;

		if (UInterchangeGenericAnimationPipeline* AnimationPipeline = PipelineSettings.AnimationPipeline)
		{
			AnimationPipeline->bImportAnimations = InSettings.bImportAnimations;
		}
		if (UInterchangeGenericMaterialPipeline* MaterialPipeline = PipelineSettings.MaterialPipeline)
		{
			MaterialPipeline->bImportMaterials = InSettings.bImportMaterials;
			MaterialPipeline->SearchLocation = InSettings.MaterialSearchLocation;
			if (UInterchangeGenericTexturePipeline* TexturePipeline = MaterialPipeline->TexturePipeline)
			{
				TexturePipeline->bImportTextures = InSettings.bImportTextures;
			}
		}

		const FString AssetName = InRequest.AssetName.IsEmpty()
			? FPaths::GetBaseFilename(InRequest.FbxFilename)
			: InRequest.AssetName;

		UE::Interchange::FScopedSourceData SourceData(InRequest.FbxFilename);
		if (SourceData.GetSourceData() == nullptr)
		{
			return MakeError(FInterchangeImportError{
				EInterchangeImportError::SourceDataUnreadable,
				FString::Printf(TEXT("The FBX file '%s' could not be read."), *InRequest.FbxFilename)
			});
		}

		FImportAssetParameters ImportParameters;
		ImportParameters.bIsAutomated = true;
		ImportParameters.bFollowRedirectors = false;
		ImportParameters.bReplaceExisting = InRequest.bReplaceExisting;
		ImportParameters.bForceShowDialog = false;
		ImportParameters.DestinationName = AssetName;
		ImportParameters.OverridePipelines.Add(FSoftObjectPath(Pipeline));

		TArray<UObject*> ImportedObjects;
		UInterchangeManager& InterchangeManager = UInterchangeManager::GetInterchangeManager();
		InterchangeManager.ImportAsset(InRequest.DestinationPath, SourceData.GetSourceData(), ImportParameters, ImportedObjects);

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
			return MakeError(FInterchangeImportError{
				EInterchangeImportError::MeshNotImported,
				FString::Printf(TEXT("Interchange did not create a skeletal mesh from '%s' (imported objects: %d)."),
					*InRequest.FbxFilename, ImportedObjects.Num())
			});
		}

		FImportedMeshInfo Info;
		Info.MeshObjectPath = ImportedMesh->GetPathName();
		if (const USkeleton* MeshSkeleton = ImportedMesh->GetSkeleton())
		{
			Info.SkeletonObjectPath = MeshSkeleton->GetPathName();
		}

		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("Interchange imported '%s' -> %s (morph targets: %s)"),
			*InRequest.FbxFilename, *Info.MeshObjectPath, bInImportMorphTargets ? TEXT("engine") : TEXT("plugin"));
		return MakeValue(MoveTemp(Info));
	}
}
