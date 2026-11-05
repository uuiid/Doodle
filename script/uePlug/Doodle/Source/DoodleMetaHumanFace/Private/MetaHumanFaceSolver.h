#pragma once

#include "CoreMinimal.h"
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
    TValueOrError<FMetaHumanFaceBuiltPerformance, FMetaHumanFaceSolverError> CreatePerformance(
        const FMetaHumanFaceJobRequest& InRequest, UFootageCaptureData* InCaptureData
    );
    TValueOrError<TArray<FMetaHumanFaceFrameRange>, FMetaHumanFaceSolverError> Process(
        UFootageCaptureData* InCaptureData, int32 InFirstFrame, int32 InFrameCount
    );
    EMetaHumanFaceProcessState PollRun();
    TValueOrError<FMetaHumanFaceSolverResult, FMetaHumanFaceSolverError> CollectCurveSet() const;
    bool IsProcessing() const;
    void Cancel();

    static TArray<FString> ExpectedHeadCurves();
    static const TArray<FString>& ModelAssetPaths();

  private:
    struct FImpl;
    TUniquePtr<FImpl> Impl;

    template <typename ValueType>
    static TValueOrError<ValueType, FMetaHumanFaceSolverError> MakeSolverError(
        EMetaHumanFaceSolverErrorCode InCode, FString InDetail,
        const TArray<FString>& InMissingValues = TArray<FString>()
    );
};
