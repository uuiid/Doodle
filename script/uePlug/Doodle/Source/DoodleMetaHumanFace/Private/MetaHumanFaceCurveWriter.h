#pragma once

#include "MetaHumanFaceTypes.h"

class UMetaHumanPerformance;

class FMetaHumanFaceCurveWriter
{
  public:
    FMetaHumanFaceCurveWriter();
    ~FMetaHumanFaceCurveWriter();

    FMetaHumanFaceCurveWriter(const FMetaHumanFaceCurveWriter&) = delete;
    FMetaHumanFaceCurveWriter(FMetaHumanFaceCurveWriter&&) = delete;
    FMetaHumanFaceCurveWriter& operator=(const FMetaHumanFaceCurveWriter&) = delete;
    FMetaHumanFaceCurveWriter& operator=(FMetaHumanFaceCurveWriter&&) = delete;

    FMetaHumanFaceJobResult Write(
        const FMetaHumanFaceCurveSet& InCurveSet, const FString& InCurvesPath, const FString& InSummaryPath
    ) const;
};

struct FMetaHumanFaceCurveExtractResult
{
    bool bOk = false;
    FString Error;
    TArray<FString> CurveNames;
    TArray<float> Values;
    TArray<FTransform> RootPoses;
    int32 FrameCount = 0;
};

FMetaHumanFaceCurveExtractResult ExtractCurvesFromPerformance(UMetaHumanPerformance* InPerformance);
