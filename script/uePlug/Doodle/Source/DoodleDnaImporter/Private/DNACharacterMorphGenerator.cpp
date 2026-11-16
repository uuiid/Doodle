// Copyright Epic Games, Inc. All Rights Reserved.

#include "DNACharacterMorphGenerator.h"

#include "DNACharacterImporterModule.h"
#include "DNACharacterImporterSettings.h"
#include "DNACharacterMemoryTrace.h"
#include "DNAFbxShapeSource.h"

#include "Engine/SkeletalMesh.h"
#include "MeshDescription.h"
#include "SkeletalMeshAttributes.h"

namespace DNACharacterImporterInternal
{
	namespace
	{
		/** Affine transform (3x3 linear part + translation) fitted between FBX space and mesh space. */
		struct FAffineFit
		{
			double Linear[3][3] = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } };
			double Translation[3] = { 0.0, 0.0, 0.0 };
			double RmsError = 0.0;
			double MaxError = 0.0;
		};

		struct FSamplePair
		{
			FVector3f Source;
			FVector3f Dest;
		};

		/**
		 * Gauss-Jordan on a 4x7 augmented matrix: [ M | R ] with M 4x4 and R 4x3.
		 * Returns false when M is singular.
		 */
		bool SolveAugmented(double Aug[4][7], double OutB[4][3])
		{
			for (int32 Column = 0; Column < 4; ++Column)
			{
				int32 Pivot = Column;
				for (int32 Row = Column + 1; Row < 4; ++Row)
				{
					if (FMath::Abs(Aug[Row][Column]) > FMath::Abs(Aug[Pivot][Column]))
					{
						Pivot = Row;
					}
				}
				if (FMath::Abs(Aug[Pivot][Column]) < 1e-12)
				{
					return false;
				}
				if (Pivot != Column)
				{
					for (int32 k = 0; k < 7; ++k)
					{
						const double Temp = Aug[Column][k];
						Aug[Column][k] = Aug[Pivot][k];
						Aug[Pivot][k] = Temp;
					}
				}

				const double Inverse = 1.0 / Aug[Column][Column];
				for (int32 k = 0; k < 7; ++k)
				{
					Aug[Column][k] *= Inverse;
				}

				for (int32 Row = 0; Row < 4; ++Row)
				{
					if (Row == Column)
					{
						continue;
					}
					const double Factor = Aug[Row][Column];
					if (Factor == 0.0)
					{
						continue;
					}
					for (int32 k = 0; k < 7; ++k)
					{
						Aug[Row][k] -= Factor * Aug[Column][k];
					}
				}
			}

			for (int32 Row = 0; Row < 4; ++Row)
			{
				for (int32 Column = 0; Column < 3; ++Column)
				{
					OutB[Row][Column] = Aug[Row][4 + Column];
				}
			}
			return true;
		}

		/** Least squares fit of Dest ~= Linear * Source + Translation, plus its residuals. */
		bool FitAffineSamples(const TArray<FSamplePair>& Samples, FAffineFit& OutFit, double& OutDiagonal)
		{
			if (Samples.Num() < 4)
			{
				return false;
			}

			FVector3f Min = Samples[0].Dest;
			FVector3f Max = Samples[0].Dest;
			double Aug[4][7] = {};
			for (const FSamplePair& Sample : Samples)
			{
				Min.X = FMath::Min(Min.X, Sample.Dest.X);
				Min.Y = FMath::Min(Min.Y, Sample.Dest.Y);
				Min.Z = FMath::Min(Min.Z, Sample.Dest.Z);
				Max.X = FMath::Max(Max.X, Sample.Dest.X);
				Max.Y = FMath::Max(Max.Y, Sample.Dest.Y);
				Max.Z = FMath::Max(Max.Z, Sample.Dest.Z);

				const double X[4] = { Sample.Source.X, Sample.Source.Y, Sample.Source.Z, 1.0 };
				const double D[3] = { Sample.Dest.X, Sample.Dest.Y, Sample.Dest.Z };
				for (int32 i = 0; i < 4; ++i)
				{
					for (int32 j = 0; j < 4; ++j)
					{
						Aug[i][j] += X[i] * X[j];
					}
					for (int32 c = 0; c < 3; ++c)
					{
						Aug[i][4 + c] += X[i] * D[c];
					}
				}
			}

			OutDiagonal = static_cast<double>((Max - Min).Size());

			double B[4][3] = {};
			if (!SolveAugmented(Aug, B))
			{
				return false;
			}

			// B solves Dest = X * B, so B is indexed [input][output]: the 3x3 linear part of the
			// fit (which is what ApplyLinear uses) is its transpose.
			for (int32 Output = 0; Output < 3; ++Output)
			{
				for (int32 Input = 0; Input < 3; ++Input)
				{
					OutFit.Linear[Output][Input] = B[Input][Output];
				}
				OutFit.Translation[Output] = B[3][Output];
			}

			double SumSquared = 0.0;
			double MaxError = 0.0;
			for (const FSamplePair& Sample : Samples)
			{
				const double P[3] = { Sample.Source.X, Sample.Source.Y, Sample.Source.Z };
				const double D[3] = { Sample.Dest.X, Sample.Dest.Y, Sample.Dest.Z };
				double Squared = 0.0;
				for (int32 Row = 0; Row < 3; ++Row)
				{
					const double Value = OutFit.Linear[Row][0] * P[0] + OutFit.Linear[Row][1] * P[1]
						+ OutFit.Linear[Row][2] * P[2] + OutFit.Translation[Row];
					const double Difference = Value - D[Row];
					Squared += Difference * Difference;
				}
				SumSquared += Squared;
				MaxError = FMath::Max(MaxError, FMath::Sqrt(Squared));
			}

			OutFit.RmsError = FMath::Sqrt(SumSquared / static_cast<double>(Samples.Num()));
			OutFit.MaxError = MaxError;
			return true;
		}

		FVector3f ApplyLinear(const FAffineFit& Fit, const FVector3f& Value)
		{
			return FVector3f(
				static_cast<float>(Fit.Linear[0][0] * Value.X + Fit.Linear[0][1] * Value.Y + Fit.Linear[0][2] * Value.Z),
				static_cast<float>(Fit.Linear[1][0] * Value.X + Fit.Linear[1][1] * Value.Y + Fit.Linear[1][2] * Value.Z),
				static_cast<float>(Fit.Linear[2][0] * Value.X + Fit.Linear[2][1] * Value.Y + Fit.Linear[2][2] * Value.Z));
		}
	}

	bool GenerateMorphTargets(
		USkeletalMesh& Mesh,
		const FDNACharacterImportRequest& Request,
		const UDNACharacterImporterSettings& Settings,
		FMorphGenerationStats& OutStats,
		FDNAMemoryTrace* Trace,
		TArray<FString>& OutLog)
	{
		auto Log = [&OutLog](const FString& Line)
		{
			OutLog.Add(Line);
			UE_LOG(LogDoodleDnaImporter, Display, TEXT("%s"), *Line);
		};

		FMeshDescription* MeshDescription = Mesh.GetMeshDescription(0);
		if (MeshDescription == nullptr)
		{
			Log(TEXT("The imported mesh has no mesh description for LOD 0."));
			return false;
		}

		FDNAFbxShapeSource Source;
		FString Error;
		if (!Source.Open(Request.FbxFilename, Error))
		{
			Log(Error);
			return false;
		}

		FSkeletalMeshAttributes Attributes(*MeshDescription);
		Attributes.Register(/*bKeepExistingAttribute=*/true);

		if (!Attributes.HasSourceGeometryParts())
		{
			Log(TEXT("The mesh description carries no source geometry parts: the FBX blend shapes cannot be mapped."));
			return false;
		}

		const TArray<FDNAFbxMeshInfo>& FbxMeshes = Source.GetMeshes();
		const TArray<FDNAFbxShapeInfo>& FbxShapes = Source.GetShapes();

		OutStats.ShapeCount = FbxShapes.Num();
		OutStats.VertexCount = MeshDescription->Vertices().Num();

		// The vertices of the description in creation order: a source geometry part covers the
		// range [offset, offset + count) of that list.
		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(MeshDescription->Vertices().Num());
		for (const FVertexID VertexID : MeshDescription->Vertices().GetElementIDs())
		{
			VertexIDs.Add(VertexID);
		}

		const TVertexAttributesRef<FVector3f> VertexPositions = MeshDescription->GetVertexPositions();
		auto PartNames = Attributes.GetSourceGeometryPartNames();
		auto PartOffsetAndCounts = Attributes.GetSourceGeometryPartVertexOffsetAndCounts();

		TArray<FSourceGeometryPartID> PartIDs;
		for (const FSourceGeometryPartID PartID : Attributes.SourceGeometryParts().GetElementIDs())
		{
			PartIDs.Add(PartID);
		}
		OutStats.MeshPartCount = PartIDs.Num();

		Log(FString::Printf(TEXT("Mesh description: %d vertices, %d source geometry part(s), %d FBX mesh(es), FBX control points in all parts, import point index attribute: %s."),
			MeshDescription->Vertices().Num(), PartIDs.Num(), FbxMeshes.Num(),
			MeshDescription->VertexAttributes().HasAttribute(MeshAttribute::Vertex::ImportPointIndex) ? TEXT("present") : TEXT("absent")));

		// Base positions of the FBX meshes, read once and kept for the fit of the parts.
		TMap<int32, TArray<FVector3f>> BasePositionCache;

		const int32 MaxSamples = FMath::Max(16, Settings.MorphMappingSampleCount);
		const float DeltaThresholdSquared = Settings.MorphTargetDeltaThreshold * Settings.MorphTargetDeltaThreshold;

		TSet<FName> RegisteredMorphs;
		TArray<FVector3f> ShapeOffsets;
		TArray<int32> ShapeOffsetVertices;
		TSet<int32> UsedFbxMeshes;
		TArray<int32> UnmappedParts;
		int64 WrittenDeltas = 0;

		for (const FSourceGeometryPartID PartID : PartIDs)
		{
			const FName PartName = PartNames[PartID];
			const TArrayView<int32> OffsetAndCount = PartOffsetAndCounts[PartID];
			if (OffsetAndCount.Num() < 2)
			{
				continue;
			}

			const int32 PartOffset = OffsetAndCount[0];
			const int32 PartCount = OffsetAndCount[1];
			if (PartCount <= 0 || PartOffset < 0 || PartOffset + PartCount > VertexIDs.Num())
			{
				continue;
			}

			// Candidate meshes: same number of control points. The names are only a hint, the fit decides.
			TArray<int32> Candidates;
			for (int32 MeshIndex = 0; MeshIndex < FbxMeshes.Num(); ++MeshIndex)
			{
				if (FbxMeshes[MeshIndex].VertexCount == PartCount)
				{
					Candidates.Add(MeshIndex);
				}
			}
			Candidates.Sort([&FbxMeshes, &PartName](const int32 Left, const int32 Right)
			{
				const bool bLeftNamed = FbxMeshes[Left].Name.Equals(PartName.ToString(), ESearchCase::IgnoreCase)
					|| FbxMeshes[Left].NodeNames.Contains(PartName.ToString());
				const bool bRightNamed = FbxMeshes[Right].Name.Equals(PartName.ToString(), ESearchCase::IgnoreCase)
					|| FbxMeshes[Right].NodeNames.Contains(PartName.ToString());
				return bLeftNamed && !bRightNamed;
			});

			if (Candidates.Num() == 0)
			{
				UnmappedParts.Add(PartID.GetValue());
				Log(FString::Printf(TEXT("Mesh part '%s' (%d vertices) has no matching FBX mesh; its blend shapes are skipped."),
					*PartName.ToString(), PartCount));
				continue;
			}

			// Destination sample points of this part.
			const int32 Stride = FMath::Max(1, PartCount / MaxSamples);
			TArray<int32> SampleIndices;
			SampleIndices.Reserve(MaxSamples + 1);
			for (int32 Index = 0; Index < PartCount; Index += Stride)
			{
				SampleIndices.Add(Index);
			}

			TArray<FVector3f> DestSamples;
			DestSamples.SetNumUninitialized(SampleIndices.Num());
			for (int32 SampleIndex = 0; SampleIndex < SampleIndices.Num(); ++SampleIndex)
			{
				DestSamples[SampleIndex] = VertexPositions[VertexIDs[PartOffset + SampleIndices[SampleIndex]]];
			}

			FVector3f DestMin = DestSamples[0];
			FVector3f DestMax = DestSamples[0];
			for (const FVector3f& Sample : DestSamples)
			{
				DestMin.X = FMath::Min(DestMin.X, Sample.X);
				DestMin.Y = FMath::Min(DestMin.Y, Sample.Y);
				DestMin.Z = FMath::Min(DestMin.Z, Sample.Z);
				DestMax.X = FMath::Max(DestMax.X, Sample.X);
				DestMax.Y = FMath::Max(DestMax.Y, Sample.Y);
				DestMax.Z = FMath::Max(DestMax.Z, Sample.Z);
			}
			const double DestDiagonal = static_cast<double>((DestMax - DestMin).Size());

			int32 BestMeshIndex = INDEX_NONE;
			FAffineFit BestFit;
			double BestRms = 0.0;

			TArray<FSamplePair> Samples;
			for (const int32 MeshIndex : Candidates)
			{
				TArray<FVector3f>* BasePositions = BasePositionCache.Find(MeshIndex);
				if (BasePositions == nullptr)
				{
					TArray<FVector3f> Loaded;
					if (!Source.GetMeshBasePositions(MeshIndex, Loaded))
					{
						continue;
					}
					BasePositions = &BasePositionCache.Add(MeshIndex, MoveTemp(Loaded));
				}
				if (BasePositions->Num() != PartCount)
				{
					continue;
				}

				Samples.Reset(SampleIndices.Num());
				for (const int32 SampleIndex : SampleIndices)
				{
					FSamplePair Pair;
					Pair.Source = (*BasePositions)[SampleIndex];
					Pair.Dest = DestSamples[Samples.Num()];
					Samples.Add(Pair);
				}

				FAffineFit Fit;
				double FitDiagonal = 0.0;
				if (!FitAffineSamples(Samples, Fit, FitDiagonal))
				{
					continue;
				}

				const double Tolerance = static_cast<double>(Settings.MorphMappingTolerance)
					* FMath::Max(DestDiagonal, FMath::Max(FitDiagonal, 1.0));
				if (Fit.RmsError > Tolerance || Fit.MaxError > Tolerance * 4.0)
				{
					continue;
				}

				if (BestMeshIndex == INDEX_NONE || Fit.RmsError < BestRms)
				{
					BestMeshIndex = MeshIndex;
					BestFit = Fit;
					BestRms = Fit.RmsError;
				}
			}

			if (BestMeshIndex == INDEX_NONE)
			{
				UnmappedParts.Add(PartID.GetValue());

				// Diagnostic: shows whether the part and the FBX mesh cover the same points (a vertex
				// order problem) or live in different spaces (a transform problem).
				{
					FString Probe = FString::Printf(
						TEXT("  probe '%s': dest bbox [%.1f %.1f %.1f]-[%.1f %.1f %.1f] first (%.1f %.1f %.1f)"),
						*PartName.ToString(),
						DestMin.X, DestMin.Y, DestMin.Z, DestMax.X, DestMax.Y, DestMax.Z,
						DestSamples[0].X, DestSamples[0].Y, DestSamples[0].Z);

					if (Candidates.Num() > 0)
					{
						const int32 ProbeMeshIndex = Candidates[0];
						TArray<FVector3f>* ProbePositions = BasePositionCache.Find(ProbeMeshIndex);
						if (ProbePositions == nullptr)
						{
							TArray<FVector3f> Loaded;
							if (Source.GetMeshBasePositions(ProbeMeshIndex, Loaded))
							{
								ProbePositions = &BasePositionCache.Add(ProbeMeshIndex, MoveTemp(Loaded));
							}
						}

						if (ProbePositions != nullptr && ProbePositions->Num() == PartCount)
						{
							const FDNAFbxMeshInfo& ProbeInfo = FbxMeshes[ProbeMeshIndex];
							const FString ProbeLabel = !ProbeInfo.Name.IsEmpty()
								? ProbeInfo.Name
								: (ProbeInfo.NodeNames.Num() > 0 ? ProbeInfo.NodeNames[0] : FString(TEXT("<unnamed>")));

							FVector3f SrcMin = (*ProbePositions)[0];
							FVector3f SrcMax = (*ProbePositions)[0];
							TArray<FSamplePair> ProbeSamples;
							ProbeSamples.Reserve(SampleIndices.Num());
							for (int32 SampleIndex = 0; SampleIndex < SampleIndices.Num(); ++SampleIndex)
							{
								const FVector3f& SourcePoint = (*ProbePositions)[SampleIndices[SampleIndex]];
								SrcMin.X = FMath::Min(SrcMin.X, SourcePoint.X);
								SrcMin.Y = FMath::Min(SrcMin.Y, SourcePoint.Y);
								SrcMin.Z = FMath::Min(SrcMin.Z, SourcePoint.Z);
								SrcMax.X = FMath::Max(SrcMax.X, SourcePoint.X);
								SrcMax.Y = FMath::Max(SrcMax.Y, SourcePoint.Y);
								SrcMax.Z = FMath::Max(SrcMax.Z, SourcePoint.Z);

								FSamplePair Pair;
								Pair.Source = SourcePoint;
								Pair.Dest = DestSamples[SampleIndex];
								ProbeSamples.Add(Pair);
							}

							FAffineFit ProbeFit;
							double FitDiagonal = 0.0;
							const bool bFitted = FitAffineSamples(ProbeSamples, ProbeFit, FitDiagonal);
							Probe += FString::Printf(
								TEXT(" | fbx[%d] '%s' bbox [%.1f %.1f %.1f]-[%.1f %.1f %.1f] first (%.1f %.1f %.1f) rms %.4f max %.4f diag %.1f/%.1f%s"),
								ProbeMeshIndex, *ProbeLabel,
								SrcMin.X, SrcMin.Y, SrcMin.Z, SrcMax.X, SrcMax.Y, SrcMax.Z,
								(*ProbePositions)[0].X, (*ProbePositions)[0].Y, (*ProbePositions)[0].Z,
								ProbeFit.RmsError, ProbeFit.MaxError, DestDiagonal, FitDiagonal,
								bFitted ? TEXT("") : TEXT(" (singular)"));
						}
					}

					Log(Probe);
				}

				Log(FString::Printf(TEXT("Mesh part '%s' (%d vertices) could not be matched to an FBX mesh within the tolerance."),
					*PartName.ToString(), PartCount));
				continue;
			}

			++OutStats.MappedMeshPartCount;
			UsedFbxMeshes.Add(BestMeshIndex);
			Log(FString::Printf(TEXT("Mesh part '%s': %d vertices <- FBX mesh '%s' (rms %.6f cm)."),
				*PartName.ToString(), PartCount, *FbxMeshes[BestMeshIndex].Name, BestRms));

			for (const int32 ShapeIndex : FbxMeshes[BestMeshIndex].ShapeIndices)
			{
				const FDNAFbxShapeInfo& ShapeInfo = FbxShapes[ShapeIndex];
				if (!Source.GetShapeVertexOffsets(ShapeIndex, ShapeOffsets, ShapeOffsetVertices))
				{
					continue;
				}

				const FName MorphName(*ShapeInfo.Name);
				if (!RegisteredMorphs.Contains(MorphName))
				{
					// The mesh may already carry a morph target of that name (re-import): writing into
					// it is what we want there, registering it a second time would fail.
					const bool bAlreadyRegistered = Attributes.GetMorphTargetNames().Contains(MorphName);
					if (!bAlreadyRegistered && !Attributes.RegisterMorphTargetAttribute(MorphName, /*bIncludeNormals=*/false))
					{
						Log(FString::Printf(TEXT("Morph target '%s' could not be registered."), *ShapeInfo.Name));
						continue;
					}
					RegisteredMorphs.Add(MorphName);
					if (!bAlreadyRegistered)
					{
						++OutStats.MorphTargetCount;
					}
				}

				TVertexAttributesRef<FVector3f> Deltas = Attributes.GetVertexMorphPositionDelta(MorphName);

				int32 NonZeroOffsets = 0;
				const int32 OffsetCount = FMath::Min(ShapeOffsets.Num(), ShapeOffsetVertices.Num());
				for (int32 OffsetIndex = 0; OffsetIndex < OffsetCount; ++OffsetIndex)
				{
					const int32 LocalIndex = ShapeOffsetVertices[OffsetIndex];
					if (LocalIndex < 0 || LocalIndex >= PartCount)
					{
						continue;
					}
					const FVector3f& Offset = ShapeOffsets[OffsetIndex];
					if (Offset.SizeSquared() <= DeltaThresholdSquared)
					{
						continue;
					}

					Deltas.Set(VertexIDs[PartOffset + LocalIndex], ApplyLinear(BestFit, Offset));
					++NonZeroOffsets;
				}

				WrittenDeltas += NonZeroOffsets;
				if (NonZeroOffsets > 0)
				{
					++OutStats.MorphTargetsWithData;
				}
			}

			if (Trace != nullptr)
			{
				Trace->Checkpoint(FString::Printf(TEXT("blend shapes of mesh part '%s'"), *PartName.ToString()));
			}
		}

		UE_LOG(LogDoodleDnaImporter, Display,
			TEXT("Morph target generation: %d shape(s), %d morph target(s), %d with data, %lld delta(s), %d/%d mesh part(s) mapped."),
			OutStats.ShapeCount, OutStats.MorphTargetCount, OutStats.MorphTargetsWithData, WrittenDeltas,
			OutStats.MappedMeshPartCount, OutStats.MeshPartCount);

		if (OutStats.MappedMeshPartCount == 0)
		{
			Log(TEXT("No mesh part could be matched; nothing was written."));
			return false;
		}

		if (Settings.bFailOnUnmappedMeshPart && UnmappedParts.Num() > 0)
		{
			Log(FString::Printf(TEXT("%d mesh part(s) could not be mapped."), UnmappedParts.Num()));
			return false;
		}

		// Every LOD has to keep the small deltas of the DNA blend shapes: the default threshold of the
		// build settings (0.015 cm) would silently delete most of them while the LOD models are built.
		for (int32 LODIndex = 0; LODIndex < Mesh.GetLODNum(); ++LODIndex)
		{
			if (FSkeletalMeshLODInfo* LODInfo = Mesh.GetLODInfo(LODIndex))
			{
				LODInfo->BuildSettings.MorphThresholdPosition = Settings.MorphTargetBuildSettingsThreshold;
			}
		}

		if (Trace != nullptr)
		{
			Trace->Checkpoint(TEXT("before CommitMeshDescription"));
		}

		Mesh.CommitMeshDescription(0);

		if (Trace != nullptr)
		{
			Trace->Checkpoint(TEXT("after CommitMeshDescription"));
		}

		Mesh.Build();

		if (Trace != nullptr)
		{
			Trace->Checkpoint(TEXT("after Build"));
		}

		Mesh.PostEditChange();
		return true;
	}
}
