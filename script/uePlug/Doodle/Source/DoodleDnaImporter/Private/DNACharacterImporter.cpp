#include "DNACharacterImporter.h"

#include "DNACharacterImporterModule.h"
#include "DNACharacterImporterSettings.h"
#include "DNACharacterInterchangeImport.h"
#include "DNACharacterMemoryTrace.h"
#include "DNACharacterMorphGenerator.h"
#include "DNACharacterPhysicsAsset.h"

#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "FileHelpers.h"
#include "HAL/PlatformMemory.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "UObject/Package.h"

namespace
{
	FString SanitizeAssetName(const FString& InName)
	{
		FString Name = FPaths::GetBaseFilename(InName);
		Name.TrimStartAndEndInline();
		for (int32 Index = 0; Index < Name.Len(); ++Index)
		{
			const TCHAR Character = Name[Index];
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Name[Index] = TEXT('_');
			}
		}
		return Name.IsEmpty() ? TEXT("DNACharacter") : Name;
	}

	void FillMeshStats(USkeletalMesh& Mesh, FDNACharacterImportResult& InOutResult)
	{
		InOutResult.BoneCount = Mesh.GetRefSkeleton().GetNum();

		if (FSkeletalMeshModel* Model = Mesh.GetImportedModel())
		{
			if (Model->LODModels.Num() > 0)
			{
				const FSkeletalMeshLODModel& LODModel = Model->LODModels[0];
				int64 TriangleCount = 0;
				for (const FSkelMeshSection& Section : LODModel.Sections)
				{
					TriangleCount += static_cast<int64>(Section.NumTriangles);
				}
				InOutResult.VertexCount = static_cast<int32>(LODModel.NumVertices);
				InOutResult.TriangleCount = static_cast<int32>(TriangleCount);
				InOutResult.SectionCount = LODModel.Sections.Num();
				return;
			}
		}
		InOutResult.VertexCount = Mesh.GetNumImportedVertices();
	}

	int32 CountMorphTargetsWithData(USkeletalMesh& Mesh, int64& InOutDeltaCount, int32& InOutMaxDeltasPerTarget)
	{
		int32 WithData = 0;
		InOutDeltaCount = 0;
		InOutMaxDeltasPerTarget = 0;

		for (const TObjectPtr<UMorphTarget>& MorphTargetPtr : Mesh.GetMorphTargets())
		{
			const UMorphTarget* MorphTarget = MorphTargetPtr.Get();
			if (MorphTarget == nullptr)
			{
				continue;
			}

			bool bHasData = false;
			for (const FMorphTargetLODModel& LODModel : MorphTarget->GetMorphLODModels())
			{
				InOutDeltaCount += LODModel.Vertices.Num();
				InOutMaxDeltasPerTarget = FMath::Max(InOutMaxDeltasPerTarget, LODModel.Vertices.Num());
				if (LODModel.Vertices.Num() > 0)
				{
					bHasData = true;
				}
			}

			if (bHasData && MorphTarget->HasValidData())
			{
				++WithData;
			}
		}

		return WithData;
	}

	using FRunResult = TValueOrError<FDNACharacterImportResult, FDNACharacterImporterRunError>;

	FRunResult MakeRunError(
		EDNACharacterImporterRunError InCode, const FString& InDetail, const FDNACharacterImportResult& InResult)
	{
		FDNACharacterImporterRunError Error;
		Error.Code = InCode;
		Error.Detail = InDetail;
		Error.Log = InResult.Log;
		return MakeError(MoveTemp(Error));
	}
}

void FDNACharacterImporter::NormalizeRequest(FDNACharacterImportRequest& InOutRequest)
{
	if (!InOutRequest.FbxFilename.IsEmpty())
	{
		InOutRequest.FbxFilename = FPaths::ConvertRelativePathToFull(InOutRequest.FbxFilename);
		FPaths::NormalizeFilename(InOutRequest.FbxFilename);
	}

	if (InOutRequest.AssetName.IsEmpty())
	{
		InOutRequest.AssetName = SanitizeAssetName(InOutRequest.FbxFilename);
	}
	else
	{
		InOutRequest.AssetName = SanitizeAssetName(InOutRequest.AssetName);
	}

	if (InOutRequest.DestinationPath.IsEmpty())
	{
		InOutRequest.DestinationPath = TEXT("/Game/DNA");
	}
	InOutRequest.DestinationPath.RemoveFromEnd(TEXT("/"));
}

FString FDNACharacterImporter::ModeToString(EDNACharacterImportMode InMode)
{
	switch (InMode)
	{
	case EDNACharacterImportMode::Stock:
		return TEXT("stock");
	case EDNACharacterImportMode::MorphTargetsOnly:
		return TEXT("morphs");
	case EDNACharacterImportMode::LowMemory:
	default:
		return TEXT("lowmem");
	}
}

TValueOrError<EDNACharacterImportMode, FString> FDNACharacterImporter::ParseMode(const FString& InText)
{
	if (InText.Equals(TEXT("stock"), ESearchCase::IgnoreCase))
	{
		return MakeValue(EDNACharacterImportMode::Stock);
	}
	if (InText.Equals(TEXT("lowmem"), ESearchCase::IgnoreCase)
		|| InText.Equals(TEXT("lowmemory"), ESearchCase::IgnoreCase)
		|| InText.Equals(TEXT("plugin"), ESearchCase::IgnoreCase))
	{
		return MakeValue(EDNACharacterImportMode::LowMemory);
	}
	if (InText.Equals(TEXT("morphs"), ESearchCase::IgnoreCase)
		|| InText.Equals(TEXT("morph"), ESearchCase::IgnoreCase)
		|| InText.Equals(TEXT("morphsonly"), ESearchCase::IgnoreCase))
	{
		return MakeValue(EDNACharacterImportMode::MorphTargetsOnly);
	}
	return MakeError(FString::Printf(TEXT("unknown import mode '%s'"), *InText));
}

FRunResult FDNACharacterImporter::Run(const FDNACharacterImportRequest& InRequest)
{
	FDNACharacterImportResult Result;

	auto AddLog = [&Result](const FString& Line)
	{
		Result.Log.Add(Line);
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("%s"), *Line);
	};

	auto FillMemory = [&Result]()
	{
		const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		Result.ProcessPeakPhysicalBytes = static_cast<int64>(MemoryStats.PeakUsedPhysical);
		Result.UsedPhysicalBytes = static_cast<int64>(MemoryStats.UsedPhysical);
	};

	if (!IsInGameThread())
	{
		const FString Detail = TEXT("A DNA character import has to run on the game thread.");
		AddLog(Detail);
		return MakeRunError(EDNACharacterImporterRunError::NotOnGameThread, Detail, Result);
	}

	const UDNACharacterImporterSettings& Settings = *GetDefault<UDNACharacterImporterSettings>();

	FDNACharacterImportRequest Request = InRequest;
	NormalizeRequest(Request);

	AddLog(FString::Printf(TEXT("DNA character import: '%s' -> %s/%s (mode %s)"),
		*Request.FbxFilename, *Request.DestinationPath, *Request.AssetName, *ModeToString(Request.Mode)));

	if (Request.Mode != EDNACharacterImportMode::MorphTargetsOnly)
	{
		if (Request.FbxFilename.IsEmpty() || !FPaths::FileExists(Request.FbxFilename))
		{
			const FString Detail = FString::Printf(TEXT("FBX file not found: %s"), *Request.FbxFilename);
			AddLog(Detail);
			FillMemory();
			return MakeRunError(EDNACharacterImporterRunError::FbxMissing, Detail, Result);
		}
	}

	FDNAMemoryTrace Trace;
	Trace.Reset();
	Trace.Checkpoint(TEXT("start"));
	Result.BaselinePeakPhysicalBytes = static_cast<int64>(Trace.GetBaselinePeakPhysical());

	USkeletalMesh* Mesh = nullptr;
	DNACharacterImporterInternal::FImportedMeshInfo ImportedInfo;

	const double ImportStart = FPlatformTime::Seconds();
	if (Request.Mode == EDNACharacterImportMode::MorphTargetsOnly)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"),
			*Request.DestinationPath, *Request.AssetName, *Request.AssetName);
		Mesh = LoadObject<USkeletalMesh>(nullptr, *ObjectPath);
		if (Mesh == nullptr)
		{
			const FString Detail = FString::Printf(TEXT("Skeletal mesh '%s' could not be loaded."), *ObjectPath);
			AddLog(Detail);
			FillMemory();
			return MakeRunError(EDNACharacterImporterRunError::MeshNotLoaded, Detail, Result);
		}
		ImportedInfo.MeshObjectPath = ObjectPath;
		AddLog(FString::Printf(TEXT("Reusing the skeletal mesh '%s'."), *ObjectPath));
	}
	else
	{
		const bool bImportMorphTargets = (Request.Mode == EDNACharacterImportMode::Stock);
		auto ImportResult = DNACharacterImporterInternal::ImportSkeletalMeshFromFbx(Request, Settings, bImportMorphTargets);
		if (ImportResult.HasError())
		{
			const FString Detail = ImportResult.GetError().Detail;
			AddLog(Detail);
			FillMemory();
			return MakeRunError(EDNACharacterImporterRunError::MeshImportFailed, Detail, Result);
		}
		ImportedInfo = MoveTemp(ImportResult.GetValue());

		Mesh = LoadObject<USkeletalMesh>(nullptr, *ImportedInfo.MeshObjectPath);
		if (Mesh == nullptr)
		{
			const FString Detail = FString::Printf(TEXT("The imported skeletal mesh '%s' could not be loaded."), *ImportedInfo.MeshObjectPath);
			AddLog(Detail);
			FillMemory();
			return MakeRunError(EDNACharacterImporterRunError::MeshNotLoaded, Detail, Result);
		}
	}
	Result.ImportSeconds = FPlatformTime::Seconds() - ImportStart;
	Result.MeshObjectPath = ImportedInfo.MeshObjectPath;
	Result.SkeletonObjectPath = ImportedInfo.SkeletonObjectPath;
	if (const USkeleton* Skeleton = Mesh->GetSkeleton())
	{
		Result.SkeletonObjectPath = Skeleton->GetPathName();
	}
	Trace.Checkpoint(TEXT("skeletal mesh imported"));
	AddLog(FString::Printf(TEXT("Skeletal mesh '%s' ready in %.2f s."), *Result.MeshObjectPath, Result.ImportSeconds));

	if (Request.Mode != EDNACharacterImportMode::Stock && Settings.bGenerateMorphTargetsFromFbx
		&& !Request.bVerifyOnly && !Request.bSkipMorphGeneration)
	{
		const double MorphStart = FPlatformTime::Seconds();
		auto MorphResult = DNACharacterImporterInternal::GenerateMorphTargets(*Mesh, Request, Settings);
		Result.MorphTargetSeconds = FPlatformTime::Seconds() - MorphStart;
		Trace.Checkpoint(TEXT("morph targets generated"));

		if (MorphResult.HasError())
		{
			const FString Detail = MorphResult.GetError().Detail;
			AddLog(Detail);
			FillMemory();
			return MakeRunError(EDNACharacterImporterRunError::MorphGenerationFailed, Detail, Result);
		}

		const DNACharacterImporterInternal::FMorphGenerationStats& MorphStats = MorphResult.GetValue();
		for (const FString& Line : MorphStats.Log)
		{
			Result.Log.Add(Line);
		}
		Result.ShapeCount = MorphStats.ShapeCount;
		Result.MeshPartCount = MorphStats.MeshPartCount;
		Result.MappedMeshPartCount = MorphStats.MappedMeshPartCount;
	}
	else if (Request.bSkipMorphGeneration)
	{
		AddLog(TEXT("Morph target generation was skipped on request: the mesh keeps the imported base geometry only."));
	}
	else if (Request.bVerifyOnly)
	{
		AddLog(TEXT("Verify only: the mesh is reported as it is, nothing was generated or saved."));
	}

	if (!Request.bVerifyOnly)
	{
		const double PhysicsStart = FPlatformTime::Seconds();
		auto PhysicsResult = DNACharacterImporterInternal::CreatePhysicsAssetForMesh(*Mesh, Settings);
		Trace.Checkpoint(TEXT("physics asset created"));
		Result.PhysicsAssetSeconds = FPlatformTime::Seconds() - PhysicsStart;

		if (PhysicsResult.HasError())
		{
			const FString Detail = PhysicsResult.GetError().Detail;
			AddLog(Detail);
			FillMemory();
			return MakeRunError(EDNACharacterImporterRunError::PhysicsAssetFailed, Detail, Result);
		}

		Result.PhysicsAssetObjectPath = PhysicsResult.GetValue();
		if (!Result.PhysicsAssetObjectPath.IsEmpty())
		{
			AddLog(FString::Printf(TEXT("Physics asset: %s (%.2f s)."), *Result.PhysicsAssetObjectPath, Result.PhysicsAssetSeconds));
		}
	}

	FillMeshStats(*Mesh, Result);

	int64 DeltaCount = 0;
	int32 MaxDeltasPerTarget = 0;
	Result.MorphTargetCount = Mesh->GetMorphTargets().Num();
	Result.MorphTargetsWithData = CountMorphTargetsWithData(*Mesh, DeltaCount, MaxDeltasPerTarget);

	AddLog(FString::Printf(
		TEXT("Result: %d vertices, %d triangles, %d bones, %d section(s), %d morph target(s) (%d with data, %lld deltas, up to %d per target)."),
		Result.VertexCount, Result.TriangleCount, Result.BoneCount, Result.SectionCount,
		Result.MorphTargetCount, Result.MorphTargetsWithData, DeltaCount, MaxDeltasPerTarget));

	if (Settings.bVerifyGeneratedMorphTargets && Result.MorphTargetCount > 0 && Result.MorphTargetsWithData == 0)
	{
		AddLog(TEXT("Warning: every morph target is empty. The deltas were either filtered out by the LOD build settings or mapped to the wrong vertices."));
	}

	if (Request.bSaveAssets && !Request.bVerifyOnly)
	{
		TArray<UPackage*> PackagesToSave;
		PackagesToSave.AddUnique(Mesh->GetOutermost());
		Mesh->MarkPackageDirty();

		if (USkeleton* Skeleton = Mesh->GetSkeleton())
		{
			PackagesToSave.AddUnique(Skeleton->GetOutermost());
			Skeleton->MarkPackageDirty();
		}

		if (UPhysicsAsset* PhysicsAsset = Mesh->GetPhysicsAsset())
		{
			PackagesToSave.AddUnique(PhysicsAsset->GetOutermost());
			PhysicsAsset->MarkPackageDirty();
		}

		const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, /*bOnlyDirty=*/true);
		AddLog(bSaved
			? FString::Printf(TEXT("Saved %d package(s)."), PackagesToSave.Num())
			: TEXT("The packages could not be saved."));
	}

	FillMemory();
	Trace.Checkpoint(TEXT("finished"));

	if (Settings.bLogMemoryTrace)
	{
		Trace.Log(FString::Printf(TEXT("DNA character import (%s)"), *ModeToString(Request.Mode)));
		Result.Log.Add(Trace.ToString());
	}

	Result.bSucceeded = true;
	Result.Message = FString::Printf(
		TEXT("Imported '%s' in %.2f s (mesh) + %.2f s (morph targets) + %.2f s (physics asset); process peak %s."),
		*Request.AssetName,
		Result.ImportSeconds,
		Result.MorphTargetSeconds,
		Result.PhysicsAssetSeconds,
		*FDNAMemoryTrace::FormatBytes(static_cast<uint64>(Result.ProcessPeakPhysicalBytes)));
	AddLog(Result.Message);
	return MakeValue(MoveTemp(Result));
}
