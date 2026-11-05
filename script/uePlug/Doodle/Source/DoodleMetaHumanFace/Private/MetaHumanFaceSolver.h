#pragma once

#include "MetaHumanFaceTypes.h"

class UFootageCaptureData;

class FMetaHumanFaceSolver
{
  public:
    FMetaHumanFaceSolver();
    ~FMetaHumanFaceSolver();

    FMetaHumanFaceSolver(const FMetaHumanFaceSolver&) = delete;
    FMetaHumanFaceSolver(FMetaHumanFaceSolver&&) = delete;
    FMetaHumanFaceSolver& operator=(const FMetaHumanFaceSolver&) = delete;
    FMetaHumanFaceSolver& operator=(FMetaHumanFaceSolver&&) = delete;

    FMetaHumanFaceJobResult Preflight() const;
    FMetaHumanFaceJobResult CreatePerformance(const FMetaHumanFaceJobRequest& InRequest);
    FMetaHumanFaceJobResult Process(UFootageCaptureData* InCaptureData);
    FMetaHumanFaceJobResult CollectCurveSet(CurveSetResult& OutCurveSet) const;
    bool IsProcessing() const;
    void Cancel();

    static TArray<FString> ExpectedHeadCurves();
};
