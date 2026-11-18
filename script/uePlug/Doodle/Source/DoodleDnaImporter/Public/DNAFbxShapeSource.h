#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Templates/ValueOrError.h"

enum class EDNAFbxShapeSourceError : uint8
{
	FileNotFound,
	ParseFailed,
	NotOpen,
	InvalidMeshIndex,
	InvalidShapeIndex,
};

struct DOODLEDNAIMPORTER_API FDNAFbxShapeSourceError
{
	EDNAFbxShapeSourceError Code = EDNAFbxShapeSourceError::ParseFailed;
	FString Detail;
};

struct DOODLEDNAIMPORTER_API FDNAFbxMeshInfo
{
	FString Name;
	TArray<FString> NodeNames;
	int32 VertexCount = 0;
	int32 TriangleCount = 0;
	TArray<int32> ShapeIndices;
};

struct DOODLEDNAIMPORTER_API FDNAFbxShapeInfo
{
	FString Name;
	FString ShapeName;
	int32 MeshIndex = INDEX_NONE;
	int32 OffsetCount = 0;
	int32 KeyframeCount = 1;
	float TargetWeight = 1.0f;
};

struct DOODLEDNAIMPORTER_API FDNAFbxShapeVertexOffsets
{
	TArray<FVector3f> Offsets;
	TArray<int32> VertexIndices;
};

struct DOODLEDNAIMPORTER_API FDNAFbxShapeOffsetProbe
{
	FString ShapeName;
	int32 OffsetCount = 0;
	int32 StoredVertexIndexCount = 0;
	int32 ControlPointCount = 0;
	bool bSparse = false;
	int32 NonZeroOffsetCount = 0;
	float MaxOffsetLength = 0.0f;
	float FirstOffsetLength = 0.0f;
};

class DOODLEDNAIMPORTER_API FDNAFbxShapeSource
{
public:
	FDNAFbxShapeSource();
	~FDNAFbxShapeSource();

	FDNAFbxShapeSource(const FDNAFbxShapeSource&) = delete;
	FDNAFbxShapeSource& operator=(const FDNAFbxShapeSource&) = delete;

	TValueOrError<void, FDNAFbxShapeSourceError> Open(const FString& InFilename);

	void Close();

	bool IsOpen() const;

	const FString& GetFilename() const;

	const TArray<FDNAFbxMeshInfo>& GetMeshes() const;
	const TArray<FDNAFbxShapeInfo>& GetShapes() const;

	int64 GetTotalVertexCount() const;

	TValueOrError<TArray<FVector3f>, FDNAFbxShapeSourceError> GetMeshBasePositions(int32 InMeshIndex) const;
	TValueOrError<TArray<FVector3f>, FDNAFbxShapeSourceError> GetShapePositions(int32 InShapeIndex) const;
	TValueOrError<FDNAFbxShapeVertexOffsets, FDNAFbxShapeSourceError> GetShapeVertexOffsets(int32 InShapeIndex) const;

	int32 CountNonZeroOffsets(int32 InShapeIndex) const;

	TValueOrError<FDNAFbxShapeOffsetProbe, FDNAFbxShapeSourceError> ProbeShapeOffsets(int32 InShapeIndex) const;

private:
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};
