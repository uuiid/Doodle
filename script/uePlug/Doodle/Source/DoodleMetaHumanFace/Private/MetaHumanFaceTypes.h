#pragma once

#include "CoreMinimal.h"

class UFootageCaptureData;

enum class EMetaHumanFaceStage : uint8
{
    Queued,
    Building,
    Solving,
    Exporting,
    Done,
    Failed
};

enum class EMetaHumanFaceErrorCode : uint8
{
    None,
    InvalidRequest,
    FramesNotFound,
    FramesFormatMismatch,
    FrameCountMismatch,
    CaptureDataBuildFailed,
    MetaHumanModelMissing,
    InferenceRuntimeMissing,
    PerformanceCreationFailed,
    ProcessingFailed,
    NoAnimationData,
    CurveSetMismatch,
    ExportFailed,
    FileSystemFailed,
    Cancelled,
    TimedOut
};

struct FMetaHumanFaceJobRequest
{
    FString FramesDir;
    FString SequencePattern;
    FString OutDir;
    FString AssetPackagePath;
    uint32 FpsNum = 24000;
    uint32 FpsDen = 1;
    int32 FirstFrame = 0;
    int32 FrameCount = 0;
    bool bHeadStabilize = true;
    bool bSkipTongueSolve = true;
};

struct FMetaHumanFaceFrameRange
{
    int32 Start = 0;
    int32 End = 0;
};

struct FMetaHumanFaceJobResult
{
    FString JobId;
    EMetaHumanFaceStage Stage = EMetaHumanFaceStage::Queued;
    EMetaHumanFaceErrorCode Code = EMetaHumanFaceErrorCode::None;
    FString Detail;
    FString CurvesPath;
    FString SummaryPath;
    FString LogPath;
    int32 FrameCount = 0;
    int32 CurveCount = 0;
    TArray<FMetaHumanFaceFrameRange> DroppedFrameRanges;
    TArray<FString> MissingModelAssets;
    double ElapsedSeconds = 0.0;
};

struct FMetaHumanFaceCurveSet
{
    TArray<FString> CurveNames;
    uint32 FpsNum = 24000;
    uint32 FpsDen = 1;
    int32 FirstFrame = 0;
    int32 FrameCount = 0;
};

struct OutCaptureDataResult
{
    bool bOk = false;
    FString Error;
    UFootageCaptureData* CaptureData = nullptr;
};

struct CurveSetResult
{
    bool bOk = false;
    FString Error;
    FMetaHumanFaceCurveSet CurveSet;
};
