// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
THIRD_PARTY_INCLUDES_END

//#if PLATFORM_WINDOWS
//#include "Windows/HideWindowsPlatformTypes.h"
//#endif

#include "GeometryCache.h"
#include "GeometryCacheTrackFlipbookAnimation.h"
#include "GeometryCacheTrackTransformAnimation.h"
#include "GeometryCacheTrackStreamable.h"
#include "GeometryCacheMeshData.h"
#include "GeometryCacheComponent.h"

#include "Async/ParallelFor.h"
#include "MeshUtilities.h"

#include "AbcImportLogger.h"
#include "AbcImportSettings.h"
#include "AbcPolyMesh.h"

enum class EDoodleSampleReadFlags : uint8
{
	Default = 0,
	Positions = 1 << 1,
	Indices = 1 << 2,
	UVs = 1 << 3,
	Normals = 1 << 4,
	Colors = 1 << 5,
	MaterialIndices = 1 << 6
};
ENUM_CLASS_FLAGS(EDoodleSampleReadFlags);

namespace DoodleAbcImporterUtilities
{

	void AppendMeshSample(FAbcMeshSample* MeshSampleOne, const FAbcMeshSample* MeshSampleTwo);

	/** Generates and populates a FGeometryCacheMeshData instance from and for the given mesh sample */
	void GeometryCacheDataForMeshSample(FGeometryCacheMeshData& OutMeshData, const FAbcMeshSample* MeshSample, const uint32 MaterialOffset);

	/**
	 * Merges the given PolyMeshes at the given FrameIndex into a GeometryCacheMeshData
	 *
	 * @param FrameIndex	The frame index to merge the PolyMeshes at
	 * @param FrameStart	The starting frame number of the range being processed
	 * @param PolyMeshes	The PolyMeshes to merge, which will be sampled at FrameIndex
	 * @param UniqueFaceSetNames	The array of unique face set names of the PolyMeshes
	 * @param MeshData		The GeometryCacheMeshData where to output the merged PolyMeshes
	 * @param PreviousNumVertices	The number of vertices in the merged PolyMeshes, used to determine if its topology is constant between 2 frames
	 * @param bConstantTopology		Flag to indicate if the merged PolyMeshes has constant topology
	 */
	void MergePolyMeshesToMeshData(int32 FrameIndex, int32 FrameStart, const TArray<FAbcPolyMesh*>& PolyMeshes, const TArray<FString>& UniqueFaceSetNames, FGeometryCacheMeshData& MeshData, int32& PreviousNumVertices, bool& bConstantTopology);

	/** Retrieves a material from an AbcFile according to the given name and resaves it into the parent package */
	UMaterialInterface* RetrieveMaterial(FAbcFile& AbcFile, const FString& MaterialName, UObject* InParent, EObjectFlags Flags);
}