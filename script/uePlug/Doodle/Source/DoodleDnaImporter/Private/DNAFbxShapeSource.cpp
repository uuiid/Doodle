
#include "DNAFbxShapeSource.h"

#include "DNACharacterImporterModule.h"

#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

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

TValueOrError<void, FDNAFbxShapeSourceError> FDNAFbxShapeSource::Open(const FString& InFilename)
{
	Close();

	if (!FPaths::FileExists(InFilename))
	{
		return MakeError(FDNAFbxShapeSourceError{
			EDNAFbxShapeSourceError::FileNotFound,
			FString::Printf(TEXT("FBX file not found: %s"), *InFilename)
		});
	}

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
		const FString Detail = FString::Printf(TEXT("ufbx could not parse '%s': %s"),
			*InFilename, *ToFString(Error.description));
		UE_LOG(LogDoodleDnaImporter, Error, TEXT("%s"), *Detail);
		return MakeError(FDNAFbxShapeSourceError{EDNAFbxShapeSourceError::ParseFailed, Detail});
	}

	Impl->Filename = InFilename;
	TSet<FString> UsedShapeNames;

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

	for (int32 MeshIndex = 0; MeshIndex < Impl->Meshes.Num(); ++MeshIndex)
	{
		const ufbx_mesh* Mesh = Scene->meshes.data[MeshIndex];
		FDNAFbxMeshInfo& MeshInfo = Impl->Meshes[MeshIndex];

		for (size_t DeformerIndex = 0; DeformerIndex < Mesh->blend_deformers.count; ++DeformerIndex)
		{
			const ufbx_blend_deformer* Deformer = Mesh->blend_deformers.data[DeformerIndex];
			const FString DeformerName = ToFString(Deformer->element.name);
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
				ShapeInfo.Name = ToFString(Shape->name);
				if (ShapeInfo.Name.IsEmpty())
				{
					FString ChannelName = ToFString(Channel->name);
					if (ChannelName.StartsWith(DeformerName) && ChannelName.Len() > DeformerName.Len() + 1)
					{
						ChannelName.RightInline(ChannelName.Len() - (DeformerName.Len() + 1), EAllowShrinking::No);
					}
					ShapeInfo.Name = ChannelName;
				}
				if (ShapeInfo.Name.IsEmpty())
				{
					ShapeInfo.Name = FString::Printf(TEXT("%s_MorphTarget_%d"), *DeformerName, static_cast<int32>(ChannelIndex));
				}
				ShapeInfo.ShapeName = ToFString(Shape->name);
				ShapeInfo.MeshIndex = MeshIndex;
				ShapeInfo.OffsetCount = static_cast<int32>(Shape->num_offsets);
				ShapeInfo.KeyframeCount = static_cast<int32>(Channel->keyframes.count);
				ShapeInfo.TargetWeight = TargetWeight;

				int32 NameClashIndex = 1;
				const FString NameBase = ShapeInfo.Name;
				while (UsedShapeNames.Contains(ShapeInfo.Name))
				{
					ShapeInfo.Name = FString::Printf(TEXT("%s_ncl_%d"), *NameBase, NameClashIndex++);
				}
				UsedShapeNames.Add(ShapeInfo.Name);

				MeshInfo.ShapeIndices.Add(Impl->Shapes.Num());
				Impl->ShapePointers.Add(Shape);
				Impl->Shapes.Add(MoveTemp(ShapeInfo));
			}
		}
	}

	UE_LOG(LogDoodleDnaImporter, Display,
		TEXT("Parsed '%s': %d mesh(es), %d blend shape channel(s), %lld control point(s) in total."),
		*InFilename, Impl->Meshes.Num(), Impl->Shapes.Num(), GetTotalVertexCount());
	return MakeValue();
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

TValueOrError<TArray<FVector3f>, FDNAFbxShapeSourceError> FDNAFbxShapeSource::GetMeshBasePositions(int32 InMeshIndex) const
{
	if (Impl->Scene == nullptr)
	{
		return MakeError(FDNAFbxShapeSourceError{EDNAFbxShapeSourceError::NotOpen, TEXT("the FBX file is not open")});
	}
	if (!Impl->Meshes.IsValidIndex(InMeshIndex))
	{
		return MakeError(FDNAFbxShapeSourceError{
			EDNAFbxShapeSourceError::InvalidMeshIndex,
			FString::Printf(TEXT("mesh index %d is out of range (%d mesh(es))"), InMeshIndex, Impl->Meshes.Num())
		});
	}

	const ufbx_mesh* Mesh = Impl->Scene->meshes.data[InMeshIndex];
	const int32 VertexCount = static_cast<int32>(Mesh->num_vertices);
	TArray<FVector3f> Positions;
	Positions.SetNumUninitialized(VertexCount);
	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		Positions[VertexIndex] = ToVector3f(Mesh->vertices.data[VertexIndex]);
	}
	return MakeValue(MoveTemp(Positions));
}

TValueOrError<TArray<FVector3f>, FDNAFbxShapeSourceError> FDNAFbxShapeSource::GetShapePositions(int32 InShapeIndex) const
{
	if (Impl->Scene == nullptr)
	{
		return MakeError(FDNAFbxShapeSourceError{EDNAFbxShapeSourceError::NotOpen, TEXT("the FBX file is not open")});
	}
	if (!Impl->Shapes.IsValidIndex(InShapeIndex))
	{
		return MakeError(FDNAFbxShapeSourceError{
			EDNAFbxShapeSourceError::InvalidShapeIndex,
			FString::Printf(TEXT("shape index %d is out of range (%d shape(s))"), InShapeIndex, Impl->Shapes.Num())
		});
	}

	const FDNAFbxShapeInfo& ShapeInfo = Impl->Shapes[InShapeIndex];
	const ufbx_blend_shape* Shape = Impl->ShapePointers[InShapeIndex];
	const ufbx_mesh* Mesh = Impl->Scene->meshes.data[ShapeInfo.MeshIndex];
	const int32 VertexCount = static_cast<int32>(Mesh->num_vertices);

	TArray<ufbx_vec3> Scratch;
	Scratch.SetNumUninitialized(VertexCount);
	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		Scratch[VertexIndex] = Mesh->vertices.data[VertexIndex];
	}

	ufbx_add_blend_shape_vertex_offsets(Shape, Scratch.GetData(), static_cast<size_t>(VertexCount),
		static_cast<ufbx_real>(ShapeInfo.TargetWeight == 0.0f ? 1.0f : ShapeInfo.TargetWeight));

	TArray<FVector3f> Positions;
	Positions.SetNumUninitialized(VertexCount);
	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		Positions[VertexIndex] = ToVector3f(Scratch[VertexIndex]);
	}
	return MakeValue(MoveTemp(Positions));
}

int32 FDNAFbxShapeSource::CountNonZeroOffsets(int32 InShapeIndex) const
{
	if (Impl->Scene == nullptr || !Impl->Shapes.IsValidIndex(InShapeIndex))
	{
		return 0;
	}

	const ufbx_blend_shape* Shape = Impl->ShapePointers[InShapeIndex];
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

TValueOrError<FDNAFbxShapeVertexOffsets, FDNAFbxShapeSourceError> FDNAFbxShapeSource::GetShapeVertexOffsets(int32 InShapeIndex) const
{
	if (Impl->Scene == nullptr)
	{
		return MakeError(FDNAFbxShapeSourceError{EDNAFbxShapeSourceError::NotOpen, TEXT("the FBX file is not open")});
	}
	if (!Impl->Shapes.IsValidIndex(InShapeIndex))
	{
		return MakeError(FDNAFbxShapeSourceError{
			EDNAFbxShapeSourceError::InvalidShapeIndex,
			FString::Printf(TEXT("shape index %d is out of range (%d shape(s))"), InShapeIndex, Impl->Shapes.Num())
		});
	}

	const ufbx_blend_shape* Shape = Impl->ShapePointers[InShapeIndex];
	const int32 OffsetCount = static_cast<int32>(Shape->num_offsets);
	const bool bSparse = Shape->offset_vertices.count > 0;

	FDNAFbxShapeVertexOffsets Offsets;
	Offsets.Offsets.SetNumUninitialized(OffsetCount);
	Offsets.VertexIndices.SetNumUninitialized(OffsetCount);
	for (int32 OffsetIndex = 0; OffsetIndex < OffsetCount; ++OffsetIndex)
	{
		Offsets.Offsets[OffsetIndex] = ToVector3f(Shape->position_offsets.data[OffsetIndex]);
		Offsets.VertexIndices[OffsetIndex] = bSparse
			? static_cast<int32>(Shape->offset_vertices.data[OffsetIndex])
			: OffsetIndex;
	}
	return MakeValue(MoveTemp(Offsets));
}
