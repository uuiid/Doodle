#pragma once

#include "CoreMinimal.h"
#include "AbcWarpHeader.h"

namespace doodle
{
    enum class EFrameReadFlags : uint8
    {
        None = 0,
        /** Will only read position data for the objects */
        PositionOnly = 1 << 1,
        /** Will pre-multiply the world matrix with the read sample positions */
        ApplyMatrix = 1 << 2,
        /** Will force single thread processing */
        ForceSingleThreaded = 1 << 4,
    };
    static const int32 MaxNumberOfResidentSamples = 8;
    class FAbcFile;
    class IAbcObject
    {
        friend class FAbcFile;

    protected:
        /*这个obj的名称*/
        FString Name;

        IAbcObject *parent;
        const FAbcFile *File;
        const Alembic::Abc::IObject Object;

        //是否是常量//
        bool bConstant;
        //最大时间和最小时间//
        float MinTime;
        float MaxTime;

        //开始帧索引//
        int32 StartFrameIndex;

        //abc采样//
        int32 NumSamples;

        float FrameTimes[MaxNumberOfResidentSamples];
        int32 ResidentSampleIndices[MaxNumberOfResidentSamples];
        bool InUseSamples[MaxNumberOfResidentSamples];

        static FBoxSphereBounds ExtractBounds(Alembic::Abc::IBox3dProperty InBoxBoundsProperty);
        static std::tuple<float, float> GetMinAndMaxTime(const Alembic::AbcGeom::IPolyMeshSchema &InSchema);
        static std::tuple<float, float> GetStartTimeAndFrame(const Alembic::AbcGeom::IPolyMeshSchema &InSchema);

    public:
        IAbcObject(const Alembic::Abc::IObject &InObject, const FAbcFile *InFile, IAbcObject *InParent);
        virtual ~IAbcObject() {}

        const FString &GetName() const { return Name; }
        //! 这里记得创建方法
        float GetTimeForFrameIndex(const int32 FrameIndex) const { return {}; };
        
        float GetTimeForFirstData() const { return MinTime; }
        float GetTimeForLastData() const { return MaxTime; }
        int32 GetFrameIndexForFirstData() const { return StartFrameIndex; }
        int32 GetNumberOfSamples() const { return NumSamples; }
        bool IsConstant() const { return bConstant; }
        IAbcObject *GetParent() const { return parent; }

        virtual FMatrix GetMatrix(const int32 FrameIndex) const { return FMatrix::Identity; };
        // virtual void SetFrameAndTime(const float InTime, const int32 FrameIndex, const EFrameReadFlags InFlags, const int32 TargetIndex = INDEX_NONE) = 0;
        virtual bool HasConstantTransform() const = 0;
        virtual bool ReadFirstFrame(const float InTime, const int32 FrameIndex) = 0;
        virtual void PurgeFrameData(const int32 FrameIndex) = 0;
    };

}