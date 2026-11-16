// Copyright Epic Games, Inc. All Rights Reserved.

#include "DNAFbxShapeSource.h"

#include "DNACharacterImporterModule.h"

#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

// Vendored ufbx, same version the engine's Interchange FBX parser uses.
#include "ufbx.h"

namespace
{
	FVector3f ToVector3f(const ufbx_vec3& V)
	{
		return FVector3f(static_cast<float>(V.x), static_cast<float>(V.y), static_cast<float>(V.z));
	}

	FString ToFString(const ufbx_string& Str)
	{
		if (Str.data == nullptr || Str.length == 0)
		{
			return FString();
		}
		// ufbx strings are not guaranteed to be null terminated.
		TArray<ANSICHAR> Buffer;
		Buffer.Append(Str.data, static_cast<int32>(Str.length));
		Buffer.Add('\0');
		return FString(UTF8_TO_TCHAR(Buffer.GetData()));
	}

	bool IsNonZero(const ufbx_vec3& V)
	{
		return FMath::Abs(static_cast<float>(V.x)) > 1e-9f
			|| FMath::Abs(static_cast<float>(V.y)) > 1e-9f
			|| FMath::Abs(static_cast<float>(V.z)) > 1e-9f;
	}
}

struct FDNAFbxShapeSource::FImpl
{
	ufbx_scene* Scene = nullptr;
	FString Filename;
	TArray<FDNAFbxMeshInfo> Meshes;
	TArray<FDNAFbxShapeInfo> Shapes;
	/** Parallel to Shapes: the ufbx shape each entry evaluates. */
	TArray<const ufbx_blend_shape*> ShapePointers;
};

FDNAFbxShapeSource::FDNAFbxShapeSource()
	: Impl(MakeUnique<FImpl>())
{
}

FDNAFbxShapeSource::~FDNAFbxShapeSource()
{
	Close();
}

bool FDNAFbxShapeSource::Open(const FString& InFilename, FString& OutError)
{
	Close();

	if (!FPaths::FileExists(InFilename))
	{
		OutError = FString::Printf(TEXT("FBX file not found: %s"), *InFilename);
		return false;
	}

	// Exactly the options the engine's Interchange FBX parser uses, so positions are in the
	// same space the engine would produce (see UfbxParser.cpp).
	ufbx_load_opts LoadOptions = {};
	LoadOptions.evaluate_skinning = false;
	LoadOptions.load_external_files = true;
	LoadOptions.use_root_transform = false;
	LoadOptions.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	LoadOptions.handedness_conversion_axis = UFBX_MIRROR_AXIS_Y;
	LoadOptions.target_axes = ufbx_axes_left_handed_z_up;
	LoadOptions.target_unit_meters = 0.01;
	LoadOptions.reverse_winding = true;

	FTCHARToUTF8 FilenameUtf8(*InFilename);
	ufbx_error Error = {};
	Impl->Scene = ufbx_load_file(FilenameUtf8.Get(), &LoadOptions, &Error);
	if (Impl->Scene == nullptr)
	{
		OutError = FString::Printf(TEXT("ufbx could not parse '%s': %s"),
			*InFilename, *ToFString(Error.description));
		UE_LOG(LogDoodleDnaImporter, Error, TEXT("%s"), *OutError);
		return false;
	}

	Impl->Filename = InFilename;

	// Meshes (geometries).
	const ufbx_scene* Scene = Impl->Scene;
	Impl->Meshes.SetNum(static_cast<int32>(Scene->meshes.count));
	for (int32 MeshIndex = 0; MeshIndex < Impl->Meshes.Num(); ++MeshIndex)
	{
		const ufbx_mesh* Mesh = Scene->meshes.data[MeshIndex];
		FDNAFbxMeshInfo& Info = Impl->Meshes[MeshIndex];
		Info.Name = ToFString(Mesh->name);
		Info.VertexCount = static_cast<int32>(Mesh->num_vertices);
		Info.TriangleCount = static_cast<int32>(Mesh->num_triangles);
	}

	// Node names that instance each mesh: the engine names source geometry parts after them.
	for (size_t NodeIndex = 0; NodeIndex < Scene->nodes.count; ++NodeIndex)
	{
		const ufbx_node* Node = Scene->nodes.data[NodeIndex];
		if (Node->mesh == nullptr)
		{
			continue;
		}
		const int32 MeshIndex = static_cast<int32>(Node->mesh->typed_id);
		if (Impl->Meshes.IsValidIndex(MeshIndex))
		{
			Impl->Meshes[MeshIndex].NodeNames.AddUnique(ToFString(Node->name));
		}
	}

	// Blend shapes, grouped per mesh in a deterministic order.
	for (int32 MeshIndex = 0; MeshIndex < Impl->Meshes.Num(); ++MeshIndex)
	{
		const ufbx_mesh* Mesh = Scene->meshes.data[MeshIndex];
		FDNAFbxMeshInfo& MeshInfo = Impl->Meshes[MeshIndex];

		for (size_t DeformerIndex = 0; DeformerIndex < Mesh->blend_deformers.count; ++DeformerIndex)
		{
			const ufbx_blend_deformer* Deformer = Mesh->blend_deformers.data[DeformerIndex];
			for (size_t ChannelIndex = 0; ChannelIndex < Deformer->channels.count; ++ChannelIndex)
			{
				const ufbx_blend_channel* Channel = Deformer->channels.data[ChannelIndex];

				const ufbx_blend_shape* Shape = Channel->target_shape;
				float TargetWeight = 1.0f;
				if (Channel->keyframes.count > 0)
				{
					const ufbx_blend_keyframe& Keyframe = Channel->keyframes.data[0];
					if (Keyframe.shape != nullptr)
					{
						Shape = Keyframe.shape;
					}
					if (Keyframe.target_weight != 0.0)
					{
						TargetWeight = static_cast<float>(Keyframe.target_weight);
					}
				}

				if (Shape == nullptr)
				{
					continue;
				}

				FDNAFbxShapeInfo ShapeInfo;
				ShapeInfo.Name = ToFString(Channel->name);
				if (ShapeInfo.Name.IsEmpty())
				{
					ShapeInfo.Name = ToFString(Shape->name);
				}
				ShapeInfo.ShapeName = ToFString(Shape->name);
				ShapeInfo.MeshIndex = MeshIndex;
				ShapeInfo.OffsetCount = static_cast<int32>(Shape->num_offsets);
				ShapeInfo.KeyframeCount = static_cast<int32>(Channel->keyframes.count);
				ShapeInfo.TargetWeight = TargetWeight;

				MeshInfo.ShapeIndices.Add(Impl->Shapes.Num());
				Impl->ShapePointers.Add(Shape);
				Impl->Shapes.Add(MoveTemp(ShapeInfo));
			}
		}
	}

	UE_LOG(LogDoodleDnaImporter, Display,
		TEXT("Parsed '%s': %d mesh(es), %d blend shape channel(s), %lld control point(s) in total."),
		*InFilename, Impl->Meshes.Num(), Impl->Shapes.Num(), GetTotalVertexCount());
	return true;
}

void FDNAFbxShapeSource::Close()
{
	if (Impl->Scene != nullptr)
	{
		ufbx_free_scene(Impl->Scene);
		Impl->Scene = nullptr;
	}
	Impl->Filename.Reset();
	Impl->Meshes.Reset();
	Impl->Shapes.Reset();
	Impl->ShapePointers.Reset();
}

bool FDNAFbxShapeSource::IsOpen() const
{
	return Impl->Scene != nullptr;
}

const FString& FDNAFbxShapeSource::GetFilename() const
{
	return Impl->Filename;
}

const TArray<FDNAFbxMeshInfo>& FDNAFbxShapeSource::GetMeshes() const
{
	return Impl->Meshes;
}

const TArray<FDNAFbxShapeInfo>& FDNAFbxShapeSource::GetShapes() const
{
	return Impl->Shapes;
}

int64 FDNAFbxShapeSource::GetTotalVertexCount() const
{
	int64 Total = 0;
	for (const FDNAFbxMeshInfo& Mesh : Impl->Meshes)
	{
		Total += Mesh.VertexCount;
	}
	return Total;
}

bool FDNAFbxShapeSource::GetMeshBasePositions(int32 MeshIndex, TArray<FVector3f>& OutPositions) const
{
	OutPositions.Reset();
	if (Impl->Scene == nullptr || !Impl->Meshes.IsValidIndex(MeshIndex))
	{
		return false;
	}

	const ufbx_mesh* Mesh = Impl->Scene->meshes.data[MeshIndex];
	const int32 VertexCount = static_cast<int32>(Mesh->num_vertices);
	OutPositions.SetNumUninitialized(VertexCount);
	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		OutPositions[VertexIndex] = ToVector3f(Mesh->vertices.data[VertexIndex]);
	}
	return true;
}

bool FDNAFbxShapeSource::GetShapePositions(int32 ShapeIndex, TArray<FVector3f>& OutPositions) const
{
	OutPositions.Reset();
	if (Impl->Scene == nullptr || !Impl->Shapes.IsValidIndex(ShapeIndex))
	{
		return false;
	}

	const FDNAFbxShapeInfo& ShapeInfo = Impl->Shapes[ShapeIndex];
	const ufbx_blend_shape* Shape = Impl->ShapePointers[ShapeIndex];
	const ufbx_mesh* Mesh = Impl->Scene->meshes.data[ShapeInfo.MeshIndex];
	const int32 VertexCount = static_cast<int32>(Mesh->num_vertices);

	// ufbx applies the offsets in place, so evaluate into a scratch buffer and convert.
	TArray<ufbx_vec3> Scratch;
	Scratch.SetNumUninitialized(VertexCount);
	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		Scratch[VertexIndex] = Mesh->vertices.data[VertexIndex];
	}

	ufbx_add_blend_shape_vertex_offsets(Shape, Scratch.GetData(), static_cast<size_t>(VertexCount),
		static_cast<ufbx_real>(ShapeInfo.TargetWeight == 0.0f ? 1.0f : ShapeInfo.TargetWeight));

	OutPositions.SetNumUninitialized(VertexCount);
	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		OutPositions[VertexIndex] = ToVector3f(Scratch[VertexIndex]);
	}
	return true;
}

int32 FDNAFbxShapeSource::CountNonZeroOffsets(int32 ShapeIndex) const
{
	if (Impl->Scene == nullptr || !Impl->Shapes.IsValidIndex(ShapeIndex))
	{
		return 0;
	}

	const ufbx_blend_shape* Shape = Impl->ShapePointers[ShapeIndex];
	const int32 OffsetCount = static_cast<int32>(Shape->num_offsets);
	int32 NonZeroCount = 0;
	for (int32 OffsetIndex = 0; OffsetIndex < OffsetCount; ++OffsetIndex)
	{
		if (IsNonZero(Shape->position_offsets.data[OffsetIndex]))
		{
			++NonZeroCount;
		}
	}
	return NonZeroCount;
}

bool FDNAFbxShapeSource::GetShapeVertexOffsets(int32 ShapeIndex, TArray<FVector3f>& OutOffsets, TArray<int32>& OutVertexIndices) const
{
	OutOffsets.Reset();
	OutVertexIndices.Reset();
	if (Impl->Scene == nullptr || !Impl->Shapes.IsValidIndex(ShapeIndex))
	{
		return false;
	}

	const ufbx_blend_shape* Shape = Impl->ShapePointers[ShapeIndex];
	const int32 OffsetCount = static_cast<int32>(Shape->num_offsets);
	// A shape either stores a delta for every control point, or a sparse list of indices.
	const bool bSparse = Shape->offset_vertices.count > 0;

	OutOffsets.SetNumUninitialized(OffsetCount);
	OutVertexIndices.SetNumUninitialized(OffsetCount);
	for (int32 OffsetIndex = 0; OffsetIndex < OffsetCount; ++OffsetIndex)
	{
		OutOffsets[OffsetIndex] = ToVector3f(Shape->position_offsets.data[OffsetIndex]);
		OutVertexIndices[OffsetIndex] = bSparse
			? static_cast<int32>(Shape->offset_vertices.data[OffsetIndex])
			: OffsetIndex;
	}
	return true;
}
