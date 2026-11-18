#include "DNACharacterImportCommandlet.h"

#include "DNACharacterImporter.h"
#include "DNACharacterImporterModule.h"
#include "DNACharacterImporterTypes.h"

#include "Misc/Parse.h"

UDNACharacterImportCommandlet::UDNACharacterImportCommandlet()
{
	IsClient  = false;
	IsServer  = false;
	IsEditor  = true;
	LogToConsole  = true;
	ShowErrorCount = true;
}

int32 UDNACharacterImportCommandlet::Main(const FString& Params)
{
	FDNACharacterImportRequest Request;
	FString Value;

	if (FParse::Value(*Params, TEXT("Fbx="), Value))
	{
		Request.FbxFilename = Value;
	}
	if (FParse::Value(*Params, TEXT("Dest="), Value))
	{
		Request.DestinationPath = Value;
	}
	if (FParse::Value(*Params, TEXT("Name="), Value))
	{
		Request.AssetName = Value;
	}
	if (FParse::Value(*Params, TEXT("Skeleton="), Value))
	{
		Request.SkeletonPath = Value;
	}
	if (FParse::Value(*Params, TEXT("Mode="), Value))
	{
		const auto ModeResult = FDNACharacterImporter::ParseMode(Value);
		if (ModeResult.HasError())
		{
			UE_LOG(LogDoodleDnaImporter, Error, TEXT("%s"), *ModeResult.GetError());
			return 1;
		}
		Request.Mode = ModeResult.GetValue();
	}
	if (FParse::Param(*Params, TEXT("NoSave")))
	{
		Request.bSaveAssets = false;
	}
	if (FParse::Param(*Params, TEXT("Verify")))
	{
		Request.bVerifyOnly = true;
	}
	if (FParse::Param(*Params, TEXT("NoMorphs")))
	{
		Request.bSkipMorphGeneration = true;
	}

	if (Request.FbxFilename.IsEmpty()
		&& !(Request.Mode == EDNACharacterImportMode::MorphTargetsOnly && Request.bVerifyOnly))
	{
		UE_LOG(LogDoodleDnaImporter, Error,
			TEXT("Usage: -run=DNACharacterImport -Fbx=<file.fbx> [-Dest=/Game/DNA] [-Name=Asset] [-Mode=stock|lowmem|morphs] [-Skeleton=/Game/Path/To/MySkeleton] [-NoSave] [-Verify] [-NoMorphs]"));
		return 1;
	}

	const auto RunResult = FDNACharacterImporter::Run(Request);

	UE_LOG(LogDoodleDnaImporter, Display, TEXT("================= DNA character import ================="));
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  FBX          : %s"), *Request.FbxFilename);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Mode         : %s"), *FDNACharacterImporter::ModeToString(Request.Mode));

	if (RunResult.HasError())
	{
		const FDNACharacterImporterRunError& Error = RunResult.GetError();
		UE_LOG(LogDoodleDnaImporter, Error, TEXT("  Result       : FAILED"));
		UE_LOG(LogDoodleDnaImporter, Error, TEXT("  Message      : %s"), *Error.Detail);
		UE_LOG(LogDoodleDnaImporter, Display, TEXT("========================================================="));
		return 1;
	}

	const FDNACharacterImportResult& Result = RunResult.GetValue();
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Mesh         : %s"), *Result.MeshObjectPath);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Skeleton     : %s"), *Result.SkeletonObjectPath);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Physics asset: %s"),
		Result.PhysicsAssetObjectPath.IsEmpty() ? TEXT("(none)") : *Result.PhysicsAssetObjectPath);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Geometry     : %d vertices, %d triangles, %d bones, %d sections"),
		Result.VertexCount, Result.TriangleCount, Result.BoneCount, Result.SectionCount);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Morph targets: %d (%d with data, %d shape(s) in the FBX, %d/%d mesh parts mapped)"),
		Result.MorphTargetCount, Result.MorphTargetsWithData, Result.ShapeCount, Result.MappedMeshPartCount, Result.MeshPartCount);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Time         : %.2f s mesh + %.2f s morph targets + %.2f s physics asset"),
		Result.ImportSeconds, Result.MorphTargetSeconds, Result.PhysicsAssetSeconds);
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("  Result       : OK"));
	UE_LOG(LogDoodleDnaImporter, Display, TEXT("========================================================="));

	return 0;
}
