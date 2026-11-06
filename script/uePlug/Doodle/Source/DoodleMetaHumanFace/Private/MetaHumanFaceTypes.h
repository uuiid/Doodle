#pragma once

#include "CoreMinimal.h"
#include "Misc/Timecode.h"

class UFootageCaptureData;
class UMetaHumanPerformance;

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
    FString CaptureDataOuter;
    uint32 FpsNum = 24000;
    uint32 FpsDen = 1;
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

enum class EMetaHumanFaceBuildErrorCode : uint8
{
    InvalidRequest,
    FramesNotFound,
    FramesFormatMismatch,
    CaptureDataBuildFailed,
    FileSystemFailed
};

enum class EMetaHumanFaceScanErrorCode : uint8
{
    FramesNotFound,
    FramesFormatMismatch
};

struct FMetaHumanFaceScanError
{
    EMetaHumanFaceScanErrorCode Code = EMetaHumanFaceScanErrorCode::FramesNotFound;
    FString Detail;
    TArray<FMetaHumanFaceFrameRange> DroppedFrameRanges;
};

struct FMetaHumanFaceBuildError
{
    EMetaHumanFaceBuildErrorCode Code = EMetaHumanFaceBuildErrorCode::InvalidRequest;
    FString Detail;
    TArray<FMetaHumanFaceFrameRange> DroppedFrameRanges;
};

struct FMetaHumanFaceBuiltCaptureData
{
    UFootageCaptureData* CaptureData = nullptr;
    FString OuterPackageName;
    int32 StartFrame = 0;
    int32 NumFrames = 0;
    TArray<FMetaHumanFaceFrameRange> DroppedFrameRanges;
    FTimecode StartTimecode;
};

struct CurveSetResult
{
    bool bOk = false;
    FString Error;
    FMetaHumanFaceCurveSet CurveSet;
};

enum class EMetaHumanFaceSolverErrorCode : uint8
{
    PerformanceCreationFailed,
    MetaHumanModelMissing,
    InferenceRuntimeMissing,
    CaptureDataNotReady,
    ProcessingRejected,
    ProcessingFailed,
    NoAnimationData,
    CurveSetMismatch,
    Cancelled,
    TimedOut
};

struct FMetaHumanFaceSolverError
{
    EMetaHumanFaceSolverErrorCode Code = EMetaHumanFaceSolverErrorCode::ProcessingFailed;
    FString Detail;
    TArray<FString> MissingModelAssets;
};

struct FMetaHumanFaceBuiltPerformance
{
    UMetaHumanPerformance* Performance = nullptr;
    FString OuterPackageName;
};

struct FMetaHumanFaceSolverResult
{
    TArray<FString> CurveNames;
    TArray<float> Values;
    TArray<FTransform> RootPoses;
    TArray<FMetaHumanFaceFrameRange> DroppedFrameRanges;
    int32 FirstFrame = 0;
    int32 FrameCount = 0;
    uint32 FpsNum = 24000;
    uint32 FpsDen = 1;
};

enum class EMetaHumanFaceProcessState : uint8
{
    Idle,
    Running,
    Succeeded,
    Failed,
    Cancelled,
    TimedOut
};