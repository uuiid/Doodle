#include "AbcWrap/IAbcObject.h"

namespace doodle
{

    IAbcObject::IAbcObject(const Alembic::Abc::IObject &InObject, const FAbcFile *InFile, IAbcObject *InParent)
        : parent(InParent),
          File(InFile),
          Object(InObject),
          bConstant(false),
          MinTime(TNumericLimits<float>::Max()),
          MaxTime(TNumericLimits<float>::Min()),
          StartFrameIndex(0),
          NumSamples(0)
    {
        Name = FString(Object.getName().c_str());
        for (int32 Index = 0; Index < MaxNumberOfResidentSamples; ++Index)
        {
            FrameTimes[Index] = 0.0f;
            ResidentSampleIndices[Index] = INDEX_NONE;
            InUseSamples[Index] = false;
        }
    }
    FBoxSphereBounds IAbcObject::ExtractBounds(Alembic::Abc::IBox3dProperty InBoxBoundsProperty)
    {
        FBoxSphereBounds Bounds{EForceInit::ForceInitToZero};

        if (InBoxBoundsProperty.valid())
        {
            const int32 NumSamples = InBoxBoundsProperty.getNumSamples();
            for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
            {
                Alembic::Abc::Box3d BoundsBox{};

                InBoxBoundsProperty.get(BoundsBox, SampleIndex);
                const auto boundSize = BoundsBox.size();
                const auto boundCenter = BoundsBox.center();

                const FBoxSphereBounds converts{
                    FVector{(float)boundCenter.x, (float)boundCenter.y, (float)boundCenter.z},
                    FVector{(float)boundSize.x * 0.5f, (float)boundSize.y * 0.5f, (float)boundSize.z * 0.5f},
                    (const float)boundSize.length() * 0.5f};
                Bounds = (SampleIndex == 0) ? converts : Bounds + converts;
            }
        }
        return Bounds;
    }

    std::tuple<float, float> IAbcObject::GetMinAndMaxTime(const Alembic::AbcGeom::IPolyMeshSchema &InSchema)
    {
        checkf(InSchema.valid(), TEXT("Invalid Schema"));
        auto TimeSampler = InSchema.getTimeSampling();
        return {TimeSampler->getSampleTime(0), TimeSampler->getSampleTime(InSchema.getNumSamples() - 1)};
    }

    std::tuple<float, float> IAbcObject::GetStartTimeAndFrame(const Alembic::AbcGeom::IPolyMeshSchema &InSchema)
    {
        checkf(InSchema.valid(), TEXT("Invalid Schema"));
        auto TimeSampler = InSchema.getTimeSampling();

        auto startTime = TimeSampler->getSampleTime(0);
        auto samplingType = TimeSampler->getTimeSamplingType();

        auto startFrame = FMath::Max<int32>(FMath::CeilToInt(startTime / (float)samplingType.getTimePerCycle()), 0);
        return {startTime, startFrame};
    }

}