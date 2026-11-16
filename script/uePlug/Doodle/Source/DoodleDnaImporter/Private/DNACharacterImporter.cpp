// Copyright Epic Games, Inc. All Rights Reserved.

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
	/** Object names cannot contain path separators or spaces. */
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

	void FillMeshStats(USkeletalMesh& Mesh, FDNACharacterImportResult& OutResult)
	{
		OutResult.BoneCount = Mesh.GetRefSkeleton().GetNum();

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
				OutResult.VertexCount = static_cast<int32>(LODModel.NumVertices);
				OutResult.TriangleCount = static_cast<int32>(TriangleCount);
				OutResult.SectionCount = LODModel.Sections.Num();
				return;
			}
		}
		OutResult.VertexCount = Mesh.GetNumImportedVertices();
	}

	/** Number of morph targets that ended up with at least one delta after the build. */
	int32 CountMorphTargetsWithData(USkeletalMesh& Mesh, int64& OutDeltaCount, int32& OutMaxDeltasPerTarget)
	{
		int32 WithData = 0;
		OutDeltaCount = 0;
		OutMaxDeltasPerTarget = 0;

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
				OutDeltaCount += LODModel.Vertices.Num();
				OutMaxDeltasPerTarget = FMath::Max(OutMaxDeltasPerTarget, LODModel.Vertices.Num());
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
}

void FDNACharacterImporter::NormalizeRequest(FDNACharacterImportRequest& Request)
{
	if (!Request.FbxFilename.IsEmpty())
	{
		Request.FbxFilename = FPaths::ConvertRelativePathToFull(Request.FbxFilename);
		FPaths::NormalizeFilename(Request.FbxFilename);
	}

	if (Request.AssetName.IsEmpty())
	{
		Request.AssetName = SanitizeAssetName(Request.FbxFilename);
	}
	else
	{
		Request.AssetName = SanitizeAssetName(Request.AssetName);
	}

	if (Request.DestinationPath.IsEmpty())
	{
		Request.DestinationPath = TEXT("/Game/DNA");
	}
	Request.DestinationPath.RemoveFromEnd(TEXT("/"));
}

FString FDNACharacterImporter::ModeToString(EDNACharacterImportMode Mode)
{
	switch (Mode)
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

bool FDNACharacterImporter::ParseMode(const FString& Text, EDNACharacterImportMode& OutMode)
{
	if (Text.Equals(TEXT("stock"), ESearchCase::IgnoreCase))
	{
		OutMode = EDNACharacterImportMode::Stock;
		return true;
	}
	if (Text.Equals(TEXT("lowmem"), ESearchCase::IgnoreCase)
		|| Text.Equals(TEXT("lowmemory"), ESearchCase::IgnoreCase)
		|| Text.Equals(TEXT("plugin"), ESearchCase::IgnoreCase))
	{
		OutMode = EDNACharacterImportMode::LowMemory;
		return true;
	}
	if (Text.Equals(TEXT("morphs"), ESearchCase::IgnoreCase)
		|| Text.Equals(TEXT("morph"), ESearchCase::IgnoreCase)
		|| Text.Equals(TEXT("morphsonly"), ESearchCase::IgnoreCase))
	{
		OutMode = EDNACharacterImportMode::MorphTargetsOnly;
		return true;
	}
	return false;
}

bool FDNACharacterImporter::Run(const FDNACharacterImportRequest& InRequest, FDNACharacterImportResult& OutResult)
{
	OutResult = FDNACharacterImportResult();
	TArray<FString>& Log = OutResult.Log;

	auto AddLog = [&Log](const FString& Line)
	{
		Log.Add(Line);
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("%s"), *Line);
	};

	// Failure paths still have to report how much memory the attempt burned.
	auto FillMemory = [&OutResult]()
	{
		const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		OutResult.ProcessPeakPhysicalBytes = static_cast<int64>(MemoryStats.PeakUsedPhysical);
		OutResult.UsedPhysicalBytes = static_cast<int64>(MemoryStats.UsedPhysical);
	};

	if (!IsInGameThread())
	{
		OutResult.Message = TEXT("A DNA character import has to run on the game thread.");
		AddLog(OutResult.Message);
		return false;
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
			OutResult.Message = FString::Printf(TEXT("FBX file not found: %s"), *Request.FbxFilename);
			AddLog(OutResult.Message);
			FillMemory();
			return false;
		}
	}

	FDNAMemoryTrace Trace;
	Trace.Reset();
	Trace.Checkpoint(TEXT("start"));
	OutResult.BaselinePeakPhysicalBytes = static_cast<int64>(Trace.GetBaselinePeakPhysical());

	USkeletalMesh* Mesh = nullptr;
	FString Error;
	DNACharacterImporterInternal::FImportedMeshInfo ImportedInfo;
	DNACharacterImporterInternal::FMorphGenerationStats MorphStats;

	// ------------------------------------------------------------------
	// 1. The mesh itself.
	// ------------------------------------------------------------------
	const double ImportStart = FPlatformTime::Seconds();
	if (Request.Mode == EDNACharacterImportMode::MorphTargetsOnly)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"),
			*Request.DestinationPath, *Request.AssetName, *Request.AssetName);
		Mesh = LoadObject<USkeletalMesh>(nullptr, *ObjectPath);
		if (Mesh == nullptr)
		{
			OutResult.Message = FString::Printf(TEXT("Skeletal mesh '%s' could not be loaded."), *ObjectPath);
			AddLog(OutResult.Message);
			FillMemory();
			return false;
		}
		ImportedInfo.MeshObjectPath = ObjectPath;
		AddLog(FString::Printf(TEXT("Reusing the skeletal mesh '%s'."), *ObjectPath));
	}
	else
	{
		// The stock mode lets Interchange build the blend shapes, every other mode keeps them away
		// from Interchange so that this plugin can build them one at a time afterwards.
		const bool bImportMorphTargets = (Request.Mode == EDNACharacterImportMode::Stock);
		if (!DNACharacterImporterInternal::ImportSkeletalMeshFromFbx(Request, Settings, bImportMorphTargets, ImportedInfo, Error))
		{
			OutResult.Message = Error;
			AddLog(Error);
			FillMemory();
			return false;
		}

		Mesh = LoadObject<USkeletalMesh>(nullptr, *ImportedInfo.MeshObjectPath);
		if (Mesh == nullptr)
		{
			OutResult.Message = FString::Printf(TEXT("The imported skeletal mesh '%s' could not be loaded."), *ImportedInfo.MeshObjectPath);
			AddLog(OutResult.Message);
			FillMemory();
			return false;
		}
	}
	OutResult.ImportSeconds = FPlatformTime::Seconds() - ImportStart;
	OutResult.MeshObjectPath = ImportedInfo.MeshObjectPath;
	OutResult.SkeletonObjectPath = ImportedInfo.SkeletonObjectPath;
	if (USkeleton* Skeleton = Mesh->GetSkeleton())
	{
		OutResult.SkeletonObjectPath = Skeleton->GetPathName();
	}
	Trace.Checkpoint(TEXT("skeletal mesh imported"));
	AddLog(FString::Printf(TEXT("Skeletal mesh '%s' ready in %.2f s."), *OutResult.MeshObjectPath, OutResult.ImportSeconds));

	// ------------------------------------------------------------------
	// 2. Morph targets.
	// ------------------------------------------------------------------
	if (Request.Mode != EDNACharacterImportMode::Stock && Settings.bGenerateMorphTargetsFromFbx
		&& !Request.bVerifyOnly && !Request.bSkipMorphGeneration)
	{
		const double MorphStart = FPlatformTime::Seconds();
		const bool bGenerated = DNACharacterImporterInternal::GenerateMorphTargets(
			*Mesh, Request, Settings, MorphStats, &Trace, Log);
		OutResult.MorphTargetSeconds = FPlatformTime::Seconds() - MorphStart;
		Trace.Checkpoint(TEXT("morph targets generated"));

		OutResult.ShapeCount = MorphStats.ShapeCount;
		OutResult.MeshPartCount = MorphStats.MeshPartCount;
		OutResult.MappedMeshPartCount = MorphStats.MappedMeshPartCount;

		if (!bGenerated)
		{
			OutResult.Message = TEXT("The blend shapes of the FBX could not be turned into morph targets.");
			AddLog(OutResult.Message);
			FillMemory();
			return false;
		}
	}
	else if (Request.bSkipMorphGeneration)
	{
		AddLog(TEXT("Morph target generation was skipped on request: the mesh keeps the imported base geometry only."));
	}
	else if (Request.bVerifyOnly)
	{
		AddLog(TEXT("Verify only: the mesh is reported as it is, nothing was generated or saved."));
	}

	// ------------------------------------------------------------------
	// 3. Physics asset.
	// ------------------------------------------------------------------
	// Done here, after the mesh has been rebuilt: Interchange would generate the bodies from the mesh
	// as it was imported, before the blend shapes were written into its mesh description.
	if (!Request.bVerifyOnly)
	{
		const double PhysicsStart = FPlatformTime::Seconds();
		const bool bPhysicsAssetCreated = DNACharacterImporterInternal::CreatePhysicsAssetForMesh(
			*Mesh, Settings, OutResult.PhysicsAssetObjectPath, Error);
		Trace.Checkpoint(TEXT("physics asset created"));
		OutResult.PhysicsAssetSeconds = FPlatformTime::Seconds() - PhysicsStart;

		if (!bPhysicsAssetCreated)
		{
			OutResult.Message = Error;
			AddLog(Error);
			FillMemory();
			return false;
		}
		if (!OutResult.PhysicsAssetObjectPath.IsEmpty())
		{
			AddLog(FString::Printf(TEXT("Physics asset: %s (%.2f s)."), *OutResult.PhysicsAssetObjectPath, OutResult.PhysicsAssetSeconds));
		}
	}

	// ------------------------------------------------------------------
	// 4. What came out of it.
	// ------------------------------------------------------------------
	FillMeshStats(*Mesh, OutResult);

	int64 DeltaCount = 0;
	int32 MaxDeltasPerTarget = 0;
	OutResult.MorphTargetCount = Mesh->GetMorphTargets().Num();
	OutResult.MorphTargetsWithData = CountMorphTargetsWithData(*Mesh, DeltaCount, MaxDeltasPerTarget);

	AddLog(FString::Printf(
		TEXT("Result: %d vertices, %d triangles, %d bones, %d section(s), %d morph target(s) (%d with data, %lld deltas, up to %d per target)."),
		OutResult.VertexCount, OutResult.TriangleCount, OutResult.BoneCount, OutResult.SectionCount,
		OutResult.MorphTargetCount, OutResult.MorphTargetsWithData, DeltaCount, MaxDeltasPerTarget));

	if (Settings.bVerifyGeneratedMorphTargets && OutResult.MorphTargetCount > 0 && OutResult.MorphTargetsWithData == 0)
	{
		AddLog(TEXT("Warning: every morph target is empty. The deltas were either filtered out by the LOD build settings or mapped to the wrong vertices."));
	}

	// ------------------------------------------------------------------
	// 5. Save.
	// ------------------------------------------------------------------
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
		Log.Add(Trace.ToString());
	}

	OutResult.bSucceeded = true;
	OutResult.Message = FString::Printf(
		TEXT("Imported '%s' in %.2f s (mesh) + %.2f s (morph targets) + %.2f s (physics asset); process peak %s."),
		*Request.AssetName,
		OutResult.ImportSeconds,
		OutResult.MorphTargetSeconds,
		OutResult.PhysicsAssetSeconds,
		*FDNAMemoryTrace::FormatBytes(static_cast<uint64>(OutResult.ProcessPeakPhysicalBytes)));
	AddLog(OutResult.Message);
	return true;
}
