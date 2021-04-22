#pragma once

#include "IAbcObject.h"
#include "Components.h"
enum class EDoodleSampleReadFlags : uint8;

namespace doodle
{
    class FAbcMeshSample
    {
    public:
        FAbcMeshSample() : NumSmoothingGroups(0), NumUVSets(1), NumMaterials(0), SampleTime(0.0f)
        {
        }

        /** Constructing from other sample*/
        FAbcMeshSample(const FAbcMeshSample &InSample)
        {
            Vertices = InSample.Vertices;
            Indices = InSample.Indices;
            Normals = InSample.Normals;
            TangentX = InSample.TangentX;
            TangentY = InSample.TangentY;
            for (uint32 UVIndex = 0; UVIndex < InSample.NumUVSets; ++UVIndex)
            {
                UVs[UVIndex] = InSample.UVs[UVIndex];
            }
            Colors = InSample.Colors;
            /*Visibility = InSample.Visibility;
		    VisibilityIndices = InSample.VisibilityIndices;*/
            MaterialIndices = InSample.MaterialIndices;
            SmoothingGroupIndices = InSample.SmoothingGroupIndices;
            NumSmoothingGroups = InSample.NumSmoothingGroups;
            NumMaterials = InSample.NumMaterials;
            SampleTime = InSample.SampleTime;
            NumUVSets = InSample.NumUVSets;
        }

        void Reset(const EDoodleSampleReadFlags ReadFlags);
        void Copy(const FAbcMeshSample &InSample, const EDoodleSampleReadFlags ReadFlags);
        void Copy(const FAbcMeshSample *InSample, const EDoodleSampleReadFlags ReadFlags);

        TArray<FVector> Vertices;
        TArray<uint32> Indices;

        // Vertex attributes (per index based)
        TArray<FVector> Normals;
        TArray<FVector> TangentX;
        TArray<FVector> TangentY;
        TArray<FVector2D> UVs[MAX_TEXCOORDS];

        TArray<FLinearColor> Colors;
        /*TArray<FVector2D> Visibility;
	    TArray<uint32> VisibilityIndices;*/

        // Per Face material and smoothing group index
        TArray<int32> MaterialIndices;
        TArray<uint32> SmoothingGroupIndices;

        /** Number of smoothing groups and different materials (will always be at least 1) */
        uint32 NumSmoothingGroups;
        uint32 NumUVSets;
        uint32 NumMaterials;

        // Time in track this sample was taken from
        float SampleTime;
    };

    class IAbcPolyMesh : public IAbcObject
    {
    protected:
        //abc mesh
        Alembic::AbcGeom::IPolyMesh PolyMesh;
        //网格对象提取模式
        Alembic::AbcGeom::IPolyMeshSchema Schema;

        TArray<FString> materalName;
        void GetMaterial();

    public:
        IAbcPolyMesh(Alembic::AbcGeom::IPolyMesh &InPolyMesh,
                     const FAbcFile *InFile,
                     IAbcObject *InParent);
        virtual ~IAbcPolyMesh() override{};

        //! 这里所有的方法都要创建
        /** Begin IAbcObject overrides */
        virtual bool ReadFirstFrame(const float InTime, const int32 FrameIndex) final { return {}; };
        virtual void SetFrameAndTime(const float InTime, const int32 FrameIndex, const EFrameReadFlags InFlags, const int32 TargetIndex = INDEX_NONE) final{};
        virtual FMatrix GetMatrix(const int32 FrameIndex) const final { return {}; };
        virtual bool HasConstantTransform() const final { return {}; };
        virtual void PurgeFrameData(const int32 FrameIndex) final{};
        /** End IAbcObject overrides */

        /** Returns sample for the given frame index (if it is part of the resident samples) */
        const FAbcMeshSample *GetSample(const int32 FrameIndex) const { return {}; };
        /** Returns the first sample available for this object */
        const FAbcMeshSample *GetFirstSample() const { return {}; };
        /** Returns the first sample available for this object transformed by first available matrix */
        const FAbcMeshSample *GetTransformedFirstSample() const { return {}; };
        /** Returns the value of the bitmask used for skipping constant vertex attributes while reading samples*/
        EDoodleSampleReadFlags GetSampleReadFlags() const { return {}; };
        /** Returns the visibility value (true = visible, false = hidden) for the given frame index  (if it is part of the resident samples)*/
        const bool GetVisibility(const int32 FrameIndex) const { return {}; };

        /** Flag whether or not this object has constant topology (used for eligibility for PCA compression) */
        bool bConstantTopology;
        /** Flag whether or not this object has a constant world matrix (used whether to incorporate into PCA compression) */
        bool bConstantTransformation;
        /** Flag whether or not this object has a constant visibility value across the entire animated range */
        bool bConstantVisibility;

        /** Cached self and child bounds for entire duration of the animation */
        FBoxSphereBounds SelfBounds;
        FBoxSphereBounds ChildBounds;

        /** Array of face set names found for this object */
        TArray<FString> FaceSetNames;

        /** Whether or not this Mesh object should be imported */
        bool bShouldImport;
    };

}