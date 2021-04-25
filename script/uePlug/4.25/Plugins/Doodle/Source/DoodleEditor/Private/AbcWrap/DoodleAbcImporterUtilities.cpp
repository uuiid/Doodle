#include "AbcWrap/DoodleAbcImporterUtilities.h"

#include "AbcWrap/AbcFile.h"
#include "AbcWrap/DoodleAbcImport.h"
#include "AbcWrap/IAbcPolyMesh.h"

#include "Stats/StatsMisc.h"

#include "AssetRegistryModule.h"
#include "Materials/Material.h"

#include "Rendering/SkeletalMeshLODModel.h"

#include "AbcWrap/AbcWarpHeader.h"

//#define LOCTEXT_NAMESPACE "DoodleAbcImporterUtilities"

void DoodleAbcImporterUtilities::AppendMeshSample(doodle::FAbcMeshSample *MeshSampleOne, const doodle::FAbcMeshSample *MeshSampleTwo)
{
	const uint32 VertexOffset = MeshSampleOne->Vertices.Num();
	MeshSampleOne->Vertices.Append(MeshSampleTwo->Vertices);

	const uint32 IndicesOffset = MeshSampleOne->Indices.Num();
	MeshSampleOne->Indices.Append(MeshSampleTwo->Indices);

	// Remap indices
	const uint32 NumIndices = MeshSampleOne->Indices.Num();
	for (uint32 IndiceIndex = IndicesOffset; IndiceIndex < NumIndices; ++IndiceIndex)
	{
		MeshSampleOne->Indices[IndiceIndex] += VertexOffset;
	}

	// Vertex attributes (per index based)
	MeshSampleOne->Normals.Append(MeshSampleTwo->Normals);
	MeshSampleOne->TangentX.Append(MeshSampleTwo->TangentX);
	MeshSampleOne->TangentY.Append(MeshSampleTwo->TangentY);

	// Append valid number of UVs and zero padding for unavailable UV channels
	if (MeshSampleTwo->NumUVSets >= MeshSampleOne->NumUVSets)
	{
		for (uint32 UVIndex = 1; UVIndex < MeshSampleTwo->NumUVSets; ++UVIndex)
		{
			const int32 NumMissingUVs = MeshSampleOne->UVs[0].Num() - MeshSampleOne->UVs[UVIndex].Num();
			MeshSampleOne->UVs[UVIndex].AddZeroed(NumMissingUVs);
			MeshSampleOne->UVs[UVIndex].Append(MeshSampleTwo->UVs[UVIndex]);
		}

		MeshSampleOne->NumUVSets = MeshSampleTwo->NumUVSets;
	}
	else
	{
		for (uint32 UVIndex = 1; UVIndex < MeshSampleOne->NumUVSets; ++UVIndex)
		{
			MeshSampleOne->UVs[UVIndex].AddZeroed(MeshSampleTwo->UVs[0].Num());
		}
	}

	MeshSampleOne->UVs[0].Append(MeshSampleTwo->UVs[0]);

	MeshSampleOne->Colors.Append(MeshSampleTwo->Colors);
	// Currently not used but will still merge
	/*MeshSampleOne->Visibility.Append(MeshSampleTwo->Visibility);
	MeshSampleOne->VisibilityIndices.Append(MeshSampleTwo->VisibilityIndices);*/

	const uint32 MaterialIndicesOffset = MeshSampleOne->MaterialIndices.Num();
	const uint32 SmoothingGroupIndicesOffset = MeshSampleOne->SmoothingGroupIndices.Num();

	ensureMsgf(MaterialIndicesOffset == SmoothingGroupIndicesOffset, TEXT("Material and smoothing group indice count should match"));

	// Per Face material and smoothing group index
	MeshSampleOne->MaterialIndices.Append(MeshSampleTwo->MaterialIndices);
	MeshSampleOne->SmoothingGroupIndices.Append(MeshSampleTwo->SmoothingGroupIndices);

	// Remap material and smoothing group indices
	const uint32 NumMaterialIndices = MeshSampleOne->MaterialIndices.Num();
	for (uint32 IndiceIndex = MaterialIndicesOffset; IndiceIndex < NumMaterialIndices; ++IndiceIndex)
	{
		MeshSampleOne->MaterialIndices[IndiceIndex] += MeshSampleOne->NumMaterials;
		MeshSampleOne->SmoothingGroupIndices[IndiceIndex] += MeshSampleOne->NumSmoothingGroups;
	}

	MeshSampleOne->NumSmoothingGroups += (MeshSampleTwo->NumSmoothingGroups != 0) ? MeshSampleTwo->NumSmoothingGroups : 1;
	MeshSampleOne->NumMaterials += (MeshSampleTwo->NumMaterials != 0) ? MeshSampleTwo->NumMaterials : 1;
}

void DoodleAbcImporterUtilities::GeometryCacheDataForMeshSample(FGeometryCacheMeshData &OutMeshData, const doodle::FAbcMeshSample *MeshSample, const uint32 MaterialOffset)
{
	OutMeshData.BoundingBox = FBox(MeshSample->Vertices);

	// We currently always have everything except motion vectors
	// TODO: Make this user configurable
	OutMeshData.VertexInfo.bHasColor0 = true;
	OutMeshData.VertexInfo.bHasTangentX = true;
	OutMeshData.VertexInfo.bHasTangentZ = true;
	OutMeshData.VertexInfo.bHasUV0 = true;
	OutMeshData.VertexInfo.bHasMotionVectors = false;

	uint32 NumMaterials = MaterialOffset;

	const int32 NumTriangles = MeshSample->Indices.Num() / 3;
	const uint32 NumSections = MeshSample->NumMaterials ? MeshSample->NumMaterials : 1;

	TArray<TArray<uint32>> SectionIndices;
	SectionIndices.AddDefaulted(NumSections);

	OutMeshData.Positions.AddZeroed(MeshSample->Normals.Num());
	OutMeshData.TangentsX.AddZeroed(MeshSample->Normals.Num());
	OutMeshData.TangentsZ.AddZeroed(MeshSample->Normals.Num());
	OutMeshData.TextureCoordinates.AddZeroed(MeshSample->Normals.Num());
	OutMeshData.Colors.AddZeroed(MeshSample->Normals.Num());

	for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
	{
		const int32 SectionIndex = MeshSample->MaterialIndices[TriangleIndex];
		TArray<uint32> &Section = SectionIndices[SectionIndex];

		for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
		{
			const int32 CornerIndex = (TriangleIndex * 3) + VertexIndex;
			const int32 Index = MeshSample->Indices[CornerIndex];

			OutMeshData.Positions[CornerIndex] = MeshSample->Vertices[Index];
			OutMeshData.TangentsX[CornerIndex] = MeshSample->TangentX[CornerIndex];
			OutMeshData.TangentsZ[CornerIndex] = MeshSample->Normals[CornerIndex];
			// store determinant of basis in w component of normal vector
			OutMeshData.TangentsZ[CornerIndex].Vector.W = GetBasisDeterminantSignByte(MeshSample->TangentX[CornerIndex], MeshSample->TangentY[CornerIndex], MeshSample->Normals[CornerIndex]);
			OutMeshData.TextureCoordinates[CornerIndex] = MeshSample->UVs[0][CornerIndex];
			OutMeshData.Colors[CornerIndex] = MeshSample->Colors[CornerIndex].ToFColor(false);

			Section.Add(CornerIndex);
		}
	}

	TArray<uint32> &Indices = OutMeshData.Indices;
	for (uint32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
	{
		// Sometimes empty sections seem to be in the file, filter these out
		// as empty batches are not allowed by the geometry cache (They ultimately trigger checks in the renderer)
		// and it seems pretty nasty to filter them out post decode in-game
		if (!SectionIndices[SectionIndex].Num())
		{
			continue;
		}

		FGeometryCacheMeshBatchInfo BatchInfo;
		BatchInfo.StartIndex = Indices.Num();
		BatchInfo.MaterialIndex = NumMaterials;
		NumMaterials++;

		BatchInfo.NumTriangles = SectionIndices[SectionIndex].Num() / 3;
		Indices.Append(SectionIndices[SectionIndex]);
		OutMeshData.BatchesInfo.Add(BatchInfo);
	}
}

void DoodleAbcImporterUtilities::MergePolyMeshesToMeshData(int32 FrameIndex, int32 FrameStart, const TArray<doodle::IAbcPolyMesh *> &PolyMeshes, const TArray<FString> &UniqueFaceSetNames, FGeometryCacheMeshData &MeshData, int32 &PreviousNumVertices, bool &bConstantTopology)
{
	doodle::FAbcMeshSample MergedSample;

	for (auto PolyMesh : PolyMeshes)
	{
		if (PolyMesh->bShouldImport)
		{
			const int32 Offset = MergedSample.MaterialIndices.Num();
			const int32 MaterialIndexOffset = MergedSample.NumMaterials;
			bConstantTopology = bConstantTopology && PolyMesh->bConstantTopology;
			if (PolyMesh->GetVisibility(FrameIndex))
			{
				const doodle::FAbcMeshSample *Sample = PolyMesh->GetSample(FrameIndex);
				DoodleAbcImporterUtilities::AppendMeshSample(&MergedSample, Sample);
				if (PolyMesh->FaceSetNames.Num() == 0)
				{
					FMemory::Memzero(MergedSample.MaterialIndices.GetData() + Offset, (MergedSample.MaterialIndices.Num() - Offset) * sizeof(int32));
				}
				else
				{
					for (int32 Index = Offset; Index < MergedSample.MaterialIndices.Num(); ++Index)
					{
						int32 &MaterialIndex = MergedSample.MaterialIndices[Index];
						if (PolyMesh->FaceSetNames.IsValidIndex(MaterialIndex - MaterialIndexOffset))
						{
							int32 FaceSetMaterialIndex = UniqueFaceSetNames.IndexOfByKey(PolyMesh->FaceSetNames[MaterialIndex - MaterialIndexOffset]);
							MaterialIndex = FaceSetMaterialIndex != INDEX_NONE ? FaceSetMaterialIndex : 0;
						}
						else
						{
							MaterialIndex = 0;
						}
					}
				}
			}
		}
	}

	if (FrameIndex > FrameStart)
	{
		bConstantTopology &= (PreviousNumVertices == MergedSample.Vertices.Num());
	}
	PreviousNumVertices = MergedSample.Vertices.Num();

	MergedSample.NumMaterials = UniqueFaceSetNames.Num();

	// Generate the mesh data for this sample
	DoodleAbcImporterUtilities::GeometryCacheDataForMeshSample(MeshData, &MergedSample, 0);
}

UMaterialInterface *DoodleAbcImporterUtilities::RetrieveMaterial(doodle::FAbcFile &AbcFile, const FString &MaterialName, UObject *InParent, EObjectFlags Flags)
{
	UMaterialInterface *Material = nullptr;
	UMaterialInterface **CachedMaterial = AbcFile.GetMaterialByName(MaterialName);
	if (CachedMaterial)
	{
		Material = *CachedMaterial;
		// Material could have been deleted if we're overriding/reimporting an asset
		if (Material->IsValidLowLevel())
		{
			if (Material->GetOuter() == GetTransientPackage())
			{
				UMaterial *ExistingTypedObject = FindObject<UMaterial>(InParent, *MaterialName);
				if (!ExistingTypedObject)
				{
					// This is in for safety, as we do not expect this to happen
					UObject *ExistingObject = FindObject<UObject>(InParent, *MaterialName);
					if (ExistingObject)
					{
						return nullptr;
					}

					Material->Rename(*MaterialName, InParent);
					Material->SetFlags(Flags);
					FAssetRegistryModule::AssetCreated(Material);
				}
				else
				{
					ExistingTypedObject->PreEditChange(nullptr);
					Material = ExistingTypedObject;
				}
			}
		}
		else
		{
			// In this case recreate the material
			Material = NewObject<UMaterial>(InParent, *MaterialName);
			Material->SetFlags(Flags);
			FAssetRegistryModule::AssetCreated(Material);
		}
	}
	else
	{
		Material = UMaterial::GetDefaultMaterial(MD_Surface);
		check(Material);
	}

	return Material;
}
