#pragma once

#include "MetaHumanFaceTypes.h"

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
        const FMetaHumanFaceBakedSet& InResult, const FString& InCurvesPath, const FString& InSummaryPath
    ) const;
};
