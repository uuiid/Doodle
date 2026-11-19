#include "DNACharacterDiagnoseCommandlet.h"

#include "Animation/AnimSequence.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Engine/SkeletalMesh.h"
#include "DNACharacterImporterModule.h"
#include "DNAFbxShapeSource.h"
#include "Misc/Parse.h"

namespace
{
	void DumpFbxShapeNames(const FString& InFilename)
	{
		FDNAFbxShapeSource Source;
		const auto OpenResult = Source.Open(InFilename);
		if (OpenResult.HasError())
		{
			UE_LOG(LogDoodleDnaImporter, Error, TEXT("[Diagnose] fbx open failed: %s"), *OpenResult.GetError().Detail);
			return;
		}

		const TArray<FDNAFbxShapeInfo>& Shapes = Source.GetShapes();
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] ===== fbx %s : %d shape(s) ====="),
			*InFilename, Shapes.Num());

		int32 Shown = 0;
		for (const FDNAFbxShapeInfo& Shape : Shapes)
		{
			if (Shown >= 40)
			{
				break;
			}
			UE_LOG(LogDoodleDnaImporter, Display,
				TEXT("[Diagnose]   mesh=%d keyframes=%d shape='%s' resolved='%s'"),
				Shape.MeshIndex, Shape.KeyframeCount, *Shape.ShapeName, *Shape.Name);
			++Shown;
		}
	}

	void DumpFbxOffsetProbes(const FString& InFilename, const FString& InNameFilter)
	{
		FDNAFbxShapeSource Source;
		const auto OpenResult = Source.Open(InFilename);
		if (OpenResult.HasError())
		{
			UE_LOG(LogDoodleDnaImporter, Error, TEXT("[Diagnose] fbx open failed: %s"), *OpenResult.GetError().Detail);
			return;
		}

		const TArray<FDNAFbxShapeInfo>& Shapes = Source.GetShapes();
		int32 Shown = 0;
		int32 WithData = 0;
		for (int32 ShapeIndex = 0; ShapeIndex < Shapes.Num(); ++ShapeIndex)
		{
			const FDNAFbxShapeInfo& Shape = Shapes[ShapeIndex];
			if (!InNameFilter.IsEmpty() && !Shape.Name.StartsWith(InNameFilter))
			{
				continue;
			}

			const auto ProbeResult = Source.ProbeShapeOffsets(ShapeIndex);
			if (ProbeResult.HasError())
			{
				continue;
			}
			const FDNAFbxShapeOffsetProbe& Probe = ProbeResult.GetValue();
			if (Probe.NonZeroOffsetCount > 0)
			{
				++WithData;
			}

			if (Shown < 30)
			{
				UE_LOG(LogDoodleDnaImporter, Display,
					TEXT("[Probe] '%s' mesh=%d offsets=%d controlPoints=%d sparse=%s storedIdx=%d nonZero=%d maxLen=%.9f"),
					*Probe.ShapeName, Shape.MeshIndex, Probe.OffsetCount, Probe.ControlPointCount,
					Probe.bSparse ? TEXT("yes") : TEXT("no"), Probe.StoredVertexIndexCount,
					Probe.NonZeroOffsetCount, Probe.MaxOffsetLength);
				++Shown;
			}
		}

		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("[Probe] filter '%s': total %d shape(s), %d with non-zero offsets"),
			*InNameFilter, Shapes.Num(), WithData);
	}

	void DumpFbxShapeGroups(const FString& InFilename)
	{
		FDNAFbxShapeSource Source;
		const auto OpenResult = Source.Open(InFilename);
		if (OpenResult.HasError())
		{
			UE_LOG(LogDoodleDnaImporter, Error, TEXT("[Diagnose] fbx open failed: %s"), *OpenResult.GetError().Detail);
			return;
		}

		const TArray<FDNAFbxMeshInfo>& Meshes = Source.GetMeshes();
		const TArray<FDNAFbxShapeInfo>& Shapes = Source.GetShapes();

		struct FMeshStats
		{
			int32 ShapeCount = 0;
			int32 NonZeroShapes = 0;
			int32 TotalNonZeroVertices = 0;
			TArray<FString> SampleNames;
		};

		TMap<int32, FMeshStats> Stats;
		for (int32 ShapeIndex = 0; ShapeIndex < Shapes.Num(); ++ShapeIndex)
		{
			const FDNAFbxShapeInfo& Shape = Shapes[ShapeIndex];
			const auto ProbeResult = Source.ProbeShapeOffsets(ShapeIndex);
			if (ProbeResult.HasError())
			{
				continue;
			}
			const FDNAFbxShapeOffsetProbe& Probe = ProbeResult.GetValue();

			FMeshStats& MeshStats = Stats.FindOrAdd(Shape.MeshIndex);
			++MeshStats.ShapeCount;
			if (Probe.NonZeroOffsetCount > 0)
			{
				++MeshStats.NonZeroShapes;
				MeshStats.TotalNonZeroVertices += Probe.NonZeroOffsetCount;
				if (MeshStats.SampleNames.Num() < 3)
				{
					MeshStats.SampleNames.Add(Shape.Name);
				}
			}
		}

		TArray<int32> MeshIndices;
		Stats.GetKeys(MeshIndices);
		MeshIndices.Sort();

		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Groups] ===== %s ====="), *InFilename);
		for (const int32 MeshIndex : MeshIndices)
		{
			const FMeshStats& MeshStats = Stats.FindChecked(MeshIndex);
			const FString MeshName = Meshes.IsValidIndex(MeshIndex) ? Meshes[MeshIndex].Name : FString();
			const FString NodeName = Meshes.IsValidIndex(MeshIndex) && Meshes[MeshIndex].NodeNames.Num() > 0
				? Meshes[MeshIndex].NodeNames[0]
				: FString();
			UE_LOG(LogDoodleDnaImporter, Display,
				TEXT("[Groups] mesh=%d fbxMesh='%s' node='%s' vertices=%d shapes=%d withData=%d nonZeroVerts=%d samples=[%s]"),
				MeshIndex, *MeshName, *NodeName,
				Meshes.IsValidIndex(MeshIndex) ? Meshes[MeshIndex].VertexCount : 0,
				MeshStats.ShapeCount, MeshStats.NonZeroShapes, MeshStats.TotalNonZeroVertices,
				*FString::Join(MeshStats.SampleNames, TEXT(",")));
		}
	}
	void DumpMesh(const FString& InObjectPath)
	{
		const USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *InObjectPath);
		if (Mesh == nullptr)
		{
			UE_LOG(LogDoodleDnaImporter, Error, TEXT("[Diagnose] mesh not found: %s"), *InObjectPath);
			return;
		}

		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] ===== mesh %s ====="), *InObjectPath);
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] material slots : %d"), Mesh->GetMaterials().Num());
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] morph targets  : %d"), Mesh->GetMorphTargets().Num());

		int32 WithData = 0;
		for (const TObjectPtr<UMorphTarget>& MorphTargetPtr : Mesh->GetMorphTargets())
		{
			const UMorphTarget* MorphTarget = MorphTargetPtr.Get();
			if (MorphTarget == nullptr)
			{
				continue;
			}
			if (MorphTarget->HasValidData())
			{
				++WithData;
			}
		}
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] morph with data: %d"), WithData);

		int32 Shown = 0;
		for (const TObjectPtr<UMorphTarget>& MorphTargetPtr : Mesh->GetMorphTargets())
		{
			if (const UMorphTarget* MorphTarget = MorphTargetPtr.Get(); MorphTarget && Shown < 10)
			{
				UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose]   morph[%d] '%s'"),
					Shown, *MorphTarget->GetName());
				++Shown;
			}
		}

		const USkeleton* Skeleton = Mesh->GetSkeleton();
		if (Skeleton == nullptr)
		{
			UE_LOG(LogDoodleDnaImporter, Error, TEXT("[Diagnose] mesh has no skeleton"));
			return;
		}

		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] skeleton : %s (%d bones)"),
			*Skeleton->GetPathName(), RefSkeleton.GetNum());

		int32 MorphCurves = 0;
		int32 MaterialCurves = 0;
		int32 TotalCurves = 0;
		Skeleton->ForEachCurveMetaData([&MorphCurves, &MaterialCurves, &TotalCurves](FName, const FCurveMetaData& Meta)
		{
			++TotalCurves;
			if (Meta.Type.bMorphtarget)
			{
				++MorphCurves;
			}
			if (Meta.Type.bMaterial)
			{
				++MaterialCurves;
			}
		});
		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("[Diagnose] skeleton curves: total %d, morph %d, material %d"),
			TotalCurves, MorphCurves, MaterialCurves);
	}

	void DumpAnim(const FString& InObjectPath)
	{
		const UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *InObjectPath);
		if (Anim == nullptr)
		{
			UE_LOG(LogDoodleDnaImporter, Warning, TEXT("[Diagnose] anim sequence not found: %s"), *InObjectPath);
			return;
		}

		UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] ===== anim %s ====="), *InObjectPath);
		if (const IAnimationDataModel* DataModel = Anim->GetDataModel())
		{
			const TArray<FFloatCurve>& FloatCurves = DataModel->GetFloatCurves();
			UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose] float curves: %d"), FloatCurves.Num());

			int32 Shown = 0;
			for (const FFloatCurve& Curve : FloatCurves)
			{
				if (Shown >= 15)
				{
					break;
				}
				UE_LOG(LogDoodleDnaImporter, Display, TEXT("[Diagnose]   curve '%s'"),
					*Curve.GetName().ToString());
				++Shown;
			}
		}
	}
}

UDNACharacterDiagnoseCommandlet::UDNACharacterDiagnoseCommandlet()
{
	IsClient  = false;
	IsServer  = false;
	IsEditor  = true;
	LogToConsole  = true;
	ShowErrorCount = true;
}

int32 UDNACharacterDiagnoseCommandlet::Main(const FString& Params)
{
	TArray<FString> MeshPaths;
	TArray<FString> AnimPaths;
	TArray<FString> FbxPaths;
	FString ProbeFbx;
	FString ProbeFilter;
	FString GroupsFbx;
	FString Value;

	if (FParse::Value(*Params, TEXT("Mesh="), Value))
	{
		Value.ParseIntoArray(MeshPaths, TEXT(","), true);
	}
	if (FParse::Value(*Params, TEXT("Anim="), Value))
	{
		Value.ParseIntoArray(AnimPaths, TEXT(","), true);
	}
	if (FParse::Value(*Params, TEXT("Fbx="), Value))
	{
		Value.ParseIntoArray(FbxPaths, TEXT(","), true);
	}
	if (FParse::Value(*Params, TEXT("Probe="), Value))
	{
		ProbeFbx = Value;
	}
	if (FParse::Value(*Params, TEXT("Filter="), Value))
	{
		ProbeFilter = Value;
	}
	if (FParse::Value(*Params, TEXT("Groups="), Value))
	{
		GroupsFbx = Value;
	}

	if (MeshPaths.Num() == 0 && AnimPaths.Num() == 0 && FbxPaths.Num() == 0 && ProbeFbx.IsEmpty() && GroupsFbx.IsEmpty())
	{
		UE_LOG(LogDoodleDnaImporter, Error,
			TEXT("Usage: -run=DNACharacterDiagnose [-Mesh=/Game/A.A] [-Anim=/Game/C.C] [-Fbx=D:/x.fbx] [-Probe=D:/x.fbx] [-Filter=prefix] [-Groups=D:/x.fbx]"));
		return 1;
	}

	if (!GroupsFbx.IsEmpty())
	{
		DumpFbxShapeGroups(GroupsFbx);
	}

	if (!ProbeFbx.IsEmpty())
	{
		DumpFbxOffsetProbes(ProbeFbx, ProbeFilter);
	}
	for (const FString& FbxPath : FbxPaths)
	{
		DumpFbxShapeNames(FbxPath);
	}
	for (const FString& MeshPath : MeshPaths)
	{
		DumpMesh(MeshPath);
	}
	for (const FString& AnimPath : AnimPaths)
	{
		DumpAnim(AnimPath);
	}

	return 0;
}
