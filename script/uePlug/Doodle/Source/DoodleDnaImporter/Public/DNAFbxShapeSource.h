// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"

/** One geometry (mesh) found in the FBX file. */
struct DOODLEDNAIMPORTER_API FDNAFbxMeshInfo
{
	/** Name of the geometry element, for example "head_lod0_mesh". */
	FString Name;

	/** Names of the nodes instancing this geometry. */
	TArray<FString> NodeNames;

	/** Number of control points (logical vertices). */
	int32 VertexCount = 0;

	/** Number of triangles of the mesh. */
	int32 TriangleCount = 0;

	/** Indices into the shape list of this file. */
	TArray<int32> ShapeIndices;
};

/** One blend shape channel (a DNA control / morph target) found in the FBX file. */
struct DOODLEDNAIMPORTER_API FDNAFbxShapeInfo
{
	/** Name of the blend channel, which is the morph target name (for example "brow_down_L"). */
	FString Name;

	/** Name of the underlying blend shape. */
	FString ShapeName;

	/** Index into the mesh list this shape belongs to. */
	int32 MeshIndex = INDEX_NONE;

	/** Number of vertex offsets stored in the file. */
	int32 OffsetCount = 0;

	/** Number of keyframes of the channel (normally 1 for DNA characters). */
	int32 KeyframeCount = 1;

	/** Target weight of the keyframe that is used. */
	float TargetWeight = 1.0f;
};

/**
 * Reads mesh and blend shape data out of an FBX file with the vendored ufbx parser.
 *
 * The file is loaded with exactly the same options the engine's Interchange FBX parser uses,
 * so the vertex positions are in the same space the engine would have produced.
 *
 * The point of this class is that it never materialises a full mesh description per blend
 * shape: a shape is evaluated on demand into a temporary buffer and can be thrown away right
 * after its deltas have been consumed.
 */
class DOODLEDNAIMPORTER_API FDNAFbxShapeSource
{
public:
	FDNAFbxShapeSource();
	~FDNAFbxShapeSource();

	FDNAFbxShapeSource(const FDNAFbxShapeSource&) = delete;
	FDNAFbxShapeSource& operator=(const FDNAFbxShapeSource&) = delete;

	/** Load the file. Returns false and fills OutError when it cannot be parsed. */
	bool Open(const FString& InFilename, FString& OutError);

	/** Release the parsed scene. */
	void Close();

	bool IsOpen() const;

	const FString& GetFilename() const;

	const TArray<FDNAFbxMeshInfo>& GetMeshes() const;
	const TArray<FDNAFbxShapeInfo>& GetShapes() const;

	/** Total number of control points over every mesh. */
	int64 GetTotalVertexCount() const;

	/** Base (rest) positions of a mesh, in file local space. */
	bool GetMeshBasePositions(int32 MeshIndex, TArray<FVector3f>& OutPositions) const;

	/** Positions of a mesh with the blend shape applied at full weight. */
	bool GetShapePositions(int32 ShapeIndex, TArray<FVector3f>& OutPositions) const;

	/**
	 * Raw offsets of a blend shape: the delta of every stored offset plus the control point index
	 * each delta applies to. This is the cheap path, it evaluates nothing and allocates nothing
	 * but the returned arrays, which is what keeps the peak memory low.
	 */
	bool GetShapeVertexOffsets(int32 ShapeIndex, TArray<FVector3f>& OutOffsets, TArray<int32>& OutVertexIndices) const;

	/** Number of offsets of the shape that are not exactly zero. Scans the shape, use sparingly. */
	int32 CountNonZeroOffsets(int32 ShapeIndex) const;

private:
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};
