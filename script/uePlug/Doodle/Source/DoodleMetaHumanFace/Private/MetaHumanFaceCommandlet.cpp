#include "MetaHumanFaceCommandlet.h"

#include "DoodleMetaHumanFaceModule.h"
#include "HAL/PlatformTime.h"
#include "MetaHumanPerformance.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MetaHumanFaceCaptureDataBuilder.h"
#include "MetaHumanFaceCommandLine.h"
#include "MetaHumanFaceCurveWriter.h"
#include "MetaHumanFaceSolver.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoodleMetaHumanFaceCommandlet, Log, All);

namespace
{
const TCHAR* const CurvesFileName = TEXT("face_curves.dhfc");
const TCHAR* const SummaryFileName = TEXT("face_curves_summary.json");
const TCHAR* const LogFileName = TEXT("face_capture_log.json");

const TCHAR* StageToString(const EMetaHumanFaceStage InStage) {
    switch (InStage)
    {
    case EMetaHumanFaceStage::Queued:
        return TEXT("Queued");
    case EMetaHumanFaceStage::Building:
        return TEXT("Building");
    case EMetaHumanFaceStage::Solving:
        return TEXT("Solving");
    case EMetaHumanFaceStage::Exporting:
        return TEXT("Exporting");
    case EMetaHumanFaceStage::Done:
        return TEXT("Done");
    case EMetaHumanFaceStage::Failed:
        return TEXT("Failed");
    }

    return TEXT("Unknown");
}

const TCHAR* ErrorCodeToString(const EMetaHumanFaceErrorCode InCode) {
    switch (InCode)
    {
    case EMetaHumanFaceErrorCode::None:
        return TEXT("None");
    case EMetaHumanFaceErrorCode::InvalidRequest:
        return TEXT("InvalidRequest");
    case EMetaHumanFaceErrorCode::FramesNotFound:
        return TEXT("FramesNotFound");
    case EMetaHumanFaceErrorCode::FramesFormatMismatch:
        return TEXT("FramesFormatMismatch");
    case EMetaHumanFaceErrorCode::FrameCountMismatch:
        return TEXT("FrameCountMismatch");
    case EMetaHumanFaceErrorCode::CaptureDataBuildFailed:
        return TEXT("CaptureDataBuildFailed");
    case EMetaHumanFaceErrorCode::MetaHumanModelMissing:
        return TEXT("MetaHumanModelMissing");
    case EMetaHumanFaceErrorCode::InferenceRuntimeMissing:
        return TEXT("InferenceRuntimeMissing");
    case EMetaHumanFaceErrorCode::PerformanceCreationFailed:
        return TEXT("PerformanceCreationFailed");
    case EMetaHumanFaceErrorCode::ProcessingFailed:
        return TEXT("ProcessingFailed");
    case EMetaHumanFaceErrorCode::NoAnimationData:
        return TEXT("NoAnimationData");
    case EMetaHumanFaceErrorCode::CurveSetMismatch:
        return TEXT("CurveSetMismatch");
    case EMetaHumanFaceErrorCode::ExportFailed:
        return TEXT("ExportFailed");
    case EMetaHumanFaceErrorCode::FileSystemFailed:
        return TEXT("FileSystemFailed");
    case EMetaHumanFaceErrorCode::Cancelled:
        return TEXT("Cancelled");
    case EMetaHumanFaceErrorCode::TimedOut:
        return TEXT("TimedOut");
    }

    return TEXT("Unknown");
}

EMetaHumanFaceErrorCode BuildErrorToJobError(const EMetaHumanFaceBuildErrorCode InCode) {
    switch (InCode)
    {
    case EMetaHumanFaceBuildErrorCode::InvalidRequest:
        return EMetaHumanFaceErrorCode::InvalidRequest;
    case EMetaHumanFaceBuildErrorCode::FramesNotFound:
        return EMetaHumanFaceErrorCode::FramesNotFound;
    case EMetaHumanFaceBuildErrorCode::FramesFormatMismatch:
        return EMetaHumanFaceErrorCode::FramesFormatMismatch;
    case EMetaHumanFaceBuildErrorCode::CaptureDataBuildFailed:
        return EMetaHumanFaceErrorCode::CaptureDataBuildFailed;
    case EMetaHumanFaceBuildErrorCode::FileSystemFailed:
        return EMetaHumanFaceErrorCode::FileSystemFailed;
    }

    return EMetaHumanFaceErrorCode::CaptureDataBuildFailed;
}

EMetaHumanFaceErrorCode SolverErrorToJobError(const EMetaHumanFaceSolverErrorCode InCode) {
    switch (InCode)
    {
    case EMetaHumanFaceSolverErrorCode::PerformanceCreationFailed:
        return EMetaHumanFaceErrorCode::PerformanceCreationFailed;
    case EMetaHumanFaceSolverErrorCode::MetaHumanModelMissing:
        return EMetaHumanFaceErrorCode::MetaHumanModelMissing;
    case EMetaHumanFaceSolverErrorCode::InferenceRuntimeMissing:
        return EMetaHumanFaceErrorCode::InferenceRuntimeMissing;
    case EMetaHumanFaceSolverErrorCode::CaptureDataNotReady:
        return EMetaHumanFaceErrorCode::CaptureDataBuildFailed;
    case EMetaHumanFaceSolverErrorCode::ProcessingRejected:
        return EMetaHumanFaceErrorCode::ProcessingFailed;
    case EMetaHumanFaceSolverErrorCode::ProcessingFailed:
        return EMetaHumanFaceErrorCode::ProcessingFailed;
    case EMetaHumanFaceSolverErrorCode::NoAnimationData:
        return EMetaHumanFaceErrorCode::NoAnimationData;
    case EMetaHumanFaceSolverErrorCode::CurveSetMismatch:
        return EMetaHumanFaceErrorCode::CurveSetMismatch;
    case EMetaHumanFaceSolverErrorCode::Cancelled:
        return EMetaHumanFaceErrorCode::Cancelled;
    case EMetaHumanFaceSolverErrorCode::TimedOut:
        return EMetaHumanFaceErrorCode::TimedOut;
    }

    return EMetaHumanFaceErrorCode::ProcessingFailed;
}

FMetaHumanFaceJobResult MakeStageFailure(
    const EMetaHumanFaceStage InStage, const EMetaHumanFaceErrorCode InCode, const FString& InDetail
) {
    FMetaHumanFaceJobResult Result;
    Result.Stage = InStage;
    Result.Code = InCode;
    Result.Detail = InDetail;
    return Result;
}

void AppendRangeToStrings(const TArray<FMetaHumanFaceFrameRange>& InRanges, TArray<FString>& InOutLines) {
    for (const FMetaHumanFaceFrameRange& Range : InRanges)
    {
        if (Range.Start == Range.End)
        {
            InOutLines.Add(FString::Printf(TEXT("%d"), Range.Start));
        }
        else
        {
            InOutLines.Add(FString::Printf(TEXT("%d-%d"), Range.Start, Range.End));
        }
    }
}

FString DescribeRanges(const TArray<FMetaHumanFaceFrameRange>& InRanges) {
    if (InRanges.IsEmpty())
    {
        return TEXT("none");
    }

    TArray<FString> Lines;
    AppendRangeToStrings(InRanges, Lines);
    return FString::Join(Lines, TEXT(", "));
}

FString MakeResultJson(const FMetaHumanFaceJobResult& InResult) {
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("stage"), StageToString(InResult.Stage));
    Root->SetStringField(TEXT("code"), ErrorCodeToString(InResult.Code));
    Root->SetStringField(TEXT("detail"), InResult.Detail);
    Root->SetStringField(TEXT("curves_path"), InResult.CurvesPath);
    Root->SetStringField(TEXT("summary_path"), InResult.SummaryPath);
    Root->SetStringField(TEXT("log_path"), InResult.LogPath);
    Root->SetNumberField(TEXT("frame_count"), InResult.FrameCount);
    Root->SetNumberField(TEXT("curve_count"), InResult.CurveCount);
    Root->SetNumberField(TEXT("elapsed_seconds"), InResult.ElapsedSeconds);

    TArray<TSharedPtr<FJsonValue>> DroppedValues;
    for (const FMetaHumanFaceFrameRange& Range : InResult.DroppedFrameRanges)
    {
        TSharedRef<FJsonObject> RangeObject = MakeShared<FJsonObject>();
        RangeObject->SetNumberField(TEXT("start"), Range.Start);
        RangeObject->SetNumberField(TEXT("end"), Range.End);
        DroppedValues.Add(MakeShared<FJsonValueObject>(RangeObject));
    }
    Root->SetArrayField(TEXT("dropped_frame_ranges"), DroppedValues);

    TArray<TSharedPtr<FJsonValue>> MissingValues;
    for (const FString& MissingAsset : InResult.MissingModelAssets)
    {
        MissingValues.Add(MakeShared<FJsonValueString>(MissingAsset));
    }
    Root->SetArrayField(TEXT("missing_model_assets"), MissingValues);

    FString Serialized;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        return FString();
    }

    return Serialized;
}

void SleepSolverTick() {
    CommandletHelpers::TickEngine(nullptr, 1.0 / 120.0);
    FPlatformProcess::Sleep(0.001f);
}
}

UMetaHumanFaceCommandlet::UMetaHumanFaceCommandlet() {
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
    UseCommandletResultAsExitCode = true;
    HelpDescription = TEXT("Converts one frame sequence to MetaHuman face curves");
    HelpUsage = TEXT("-FramesDir=<dir> -OutDir=<dir> [-FPS=num/den] [-Range=start:count]");
    HelpParamNames.Add(TEXT("FramesDir"));
    HelpParamNames.Add(TEXT("OutDir"));
    HelpParamNames.Add(TEXT("FPS"));
    HelpParamNames.Add(TEXT("Range"));
    HelpParamNames.Add(TEXT("SequencePattern"));
    HelpParamNames.Add(TEXT("PackagePath"));
    HelpParamDescriptions.Add(TEXT("Directory holding the frame sequence"));
    HelpParamDescriptions.Add(TEXT("Directory receiving the curve output"));
    HelpParamDescriptions.Add(TEXT("Frame rate as a rational number, default 24000/1"));
    HelpParamDescriptions.Add(TEXT("First frame and frame count, default covers the whole sequence"));
    HelpParamDescriptions.Add(TEXT("Frame file name template containing {frame}"));
    HelpParamDescriptions.Add(TEXT("Package path receiving the intermediate assets"));
}

int32 UMetaHumanFaceCommandlet::Main(const FString& Params) {
    StartSeconds = FPlatformTime::Seconds();
    RunOutDir.Reset();
    Solver.Reset();

    FMetaHumanFaceCommandLine CommandLine;
    const FMetaHumanFaceJobResult Parsed = CommandLine.Parse(Params);
    if (Parsed.Code != EMetaHumanFaceErrorCode::None)
    {
        Report(Parsed);
        return ToExitCode(Parsed);
    }

    const FMetaHumanFaceJobRequest Request = CommandLine.GetRequest();

    RunOutDir = Request.OutDir;

    FMetaHumanFaceJobResult Result = Run(Request);
    RememberElapsed(Result);
    WriteLog(Result);
    Report(Result);
    return ToExitCode(Result);
}

FMetaHumanFaceJobResult UMetaHumanFaceCommandlet::Run(const FMetaHumanFaceJobRequest& InRequest) {
    const FMetaHumanFaceCaptureDataBuilder Builder;

    FMetaHumanFaceJobResult Validated = Builder.Validate(InRequest);
    if (Validated.Code != EMetaHumanFaceErrorCode::None)
    {
        return Validated;
    }

    const FMetaHumanFaceSolver Verified;
    FMetaHumanFaceJobResult Preflight = Verified.Preflight();
    if (!Preflight.MissingModelAssets.IsEmpty())
    {
        return Preflight;
    }

    Validated.Stage = EMetaHumanFaceStage::Building;
    UE_LOG(LogDoodleMetaHumanFaceCommandlet, Display, TEXT("building capture data from %s"), *InRequest.FramesDir);

    TValueOrError<FMetaHumanFaceBuiltCaptureData, FMetaHumanFaceBuildError> Built = Builder.Build(InRequest);
    if (Built.HasError())
    {
        const FMetaHumanFaceBuildError& Error = Built.GetError();
        return MakeStageFailure(
            EMetaHumanFaceStage::Building, BuildErrorToJobError(Error.Code), Error.Detail
        );
    }

    const FMetaHumanFaceBuiltCaptureData CaptureData = MoveTemp(Built.GetValue());
    FMetaHumanFaceJobResult Result;

    Result.CurveCount = 0;
    Result.FrameCount = 0;

    TValueOrError<FMetaHumanFaceSolverResult, FMetaHumanFaceSolverError> Solved =
        MakeError(FMetaHumanFaceSolverError{});

    {
        FMetaHumanFaceJobResult StageResult;
        TValueOrError<FMetaHumanFaceBuiltPerformance, FMetaHumanFaceSolverError> Created =
            MakeError(FMetaHumanFaceSolverError{});
        bool bStagesOk = false;

        Solver = MakeUnique<FMetaHumanFaceSolver>();
        Created = Solver->CreatePerformance(InRequest, CaptureData.CaptureData);

        if (Created.HasError())
        {
            const FMetaHumanFaceSolverError& Error = Created.GetError();
            return MakeStageFailure(
                EMetaHumanFaceStage::Solving, SolverErrorToJobError(Error.Code), Error.Detail
            );
        }

        const FMetaHumanFaceBuiltPerformance Performance = MoveTemp(Created.GetValue());

        const TRange<FFrameNumber>& SolvedRange = Performance.Performance->GetProcessingLimitFrameRange();

        TValueOrError<TArray<FMetaHumanFaceFrameRange>, FMetaHumanFaceSolverError> Started =
            Solver->Process(CaptureData.CaptureData);

        if (Started.HasError())
        {
            const FMetaHumanFaceSolverError& Error = Started.GetError();
            return MakeStageFailure(
                EMetaHumanFaceStage::Solving, SolverErrorToJobError(Error.Code), Error.Detail
            );
        }

        const TArray<FMetaHumanFaceFrameRange> DroppedRanges = Started.GetValue();

        double NextProgressLogSeconds = StartSeconds + 5.0;
        while (true)
        {
            const EMetaHumanFaceProcessState State = Solver->PollRun();

            if (State == EMetaHumanFaceProcessState::Succeeded)
            {
                bStagesOk = true;
                break;
            }

            if (State == EMetaHumanFaceProcessState::Idle)
            {
                return MakeStageFailure(
                    EMetaHumanFaceStage::Solving, EMetaHumanFaceErrorCode::ProcessingFailed,
                    TEXT("the MetaHuman pipeline stopped without reporting a result")
                );
            }

            if (State != EMetaHumanFaceProcessState::Running)
            {
                const EMetaHumanFaceErrorCode Code = State == EMetaHumanFaceProcessState::Cancelled
                                                         ? EMetaHumanFaceErrorCode::Cancelled
                                                         : EMetaHumanFaceErrorCode::ProcessingFailed;
                FMetaHumanFaceJobResult Failed = MakeStageFailure(
                    EMetaHumanFaceStage::Solving, Code,
                    FString::Printf(
                        TEXT("the MetaHuman pipeline exited with %d"), static_cast<int32>(State)
                    )
                );
                Failed.DroppedFrameRanges = DroppedRanges;
                return Failed;
            }

            if (FPlatformTime::Seconds() >= NextProgressLogSeconds)
            {
                NextProgressLogSeconds += 5.0;
                UE_LOG(
                    LogDoodleMetaHumanFaceCommandlet, Display, TEXT("solving frames %d-%d, elapsed %.1fs"),
                    SolvedRange.GetLowerBoundValue().Value, SolvedRange.GetUpperBoundValue().Value - 1,
                    FPlatformTime::Seconds() - StartSeconds
                );
            }

            SleepSolverTick();
        }

        if (!bStagesOk)
        {
            return MakeStageFailure(
                EMetaHumanFaceStage::Solving, EMetaHumanFaceErrorCode::ProcessingFailed,
                TEXT("the MetaHuman pipeline did not finish")
            );
        }

        Solved = Solver->CollectCurveSet();
        if (Solved.HasError())
        {
            const FMetaHumanFaceSolverError& Error = Solved.GetError();
            FMetaHumanFaceJobResult Failed =
                MakeStageFailure(EMetaHumanFaceStage::Solving, SolverErrorToJobError(Error.Code), Error.Detail);
            Failed.DroppedFrameRanges = DroppedRanges;
            Failed.MissingModelAssets = Error.MissingModelAssets;
            return Failed;
        }

        FMetaHumanFaceSolverResult SolvedResult = MoveTemp(Solved.GetValue());
        for (const FMetaHumanFaceFrameRange& Range : DroppedRanges)
        {
            SolvedResult.DroppedFrameRanges.Add(Range);
        }

        Solved = MakeValue(MoveTemp(SolvedResult));
    }

    Result.Stage = EMetaHumanFaceStage::Exporting;

    const FString CurvesPath = FPaths::Combine(InRequest.OutDir, CurvesFileName);
    const FString SummaryPath = FPaths::Combine(InRequest.OutDir, SummaryFileName);
    const FMetaHumanFaceCurveWriter Writer;
    const FMetaHumanFaceJobResult Written = Writer.Write(Solved.GetValue(), CurvesPath, SummaryPath);
    if (Written.Code != EMetaHumanFaceErrorCode::None)
    {
        FMetaHumanFaceJobResult Failed = Written;
        Failed.Stage = EMetaHumanFaceStage::Exporting;
        return Failed;
    }

    return Written;
}

void UMetaHumanFaceCommandlet::RememberElapsed(FMetaHumanFaceJobResult& InResult) const {
    InResult.ElapsedSeconds = FPlatformTime::Seconds() - StartSeconds;
}

void UMetaHumanFaceCommandlet::WriteLog(FMetaHumanFaceJobResult& InResult) const {
    if (RunOutDir.IsEmpty())
    {
        return;
    }

    const FString JobLogPath = FPaths::Combine(RunOutDir, LogFileName);
    InResult.LogPath = JobLogPath;

    const FString LogDirectory = FPaths::GetPath(JobLogPath);
    if (!LogDirectory.IsEmpty() && !IFileManager::Get().MakeDirectory(*LogDirectory, true))
    {
        UE_LOG(LogDoodleMetaHumanFaceCommandlet, Warning, TEXT("failed to create directory %s"), *LogDirectory);
        return;
    }

    const FString Json = MakeResultJson(InResult);
    if (Json.IsEmpty())
    {
        UE_LOG(LogDoodleMetaHumanFaceCommandlet, Warning, TEXT("failed to serialize the job log"));
        return;
    }

    if (!FFileHelper::SaveStringToFile(Json, *JobLogPath))
    {
        UE_LOG(LogDoodleMetaHumanFaceCommandlet, Warning, TEXT("failed to write %s"), *JobLogPath);
    }
}

int32 UMetaHumanFaceCommandlet::ToExitCode(const FMetaHumanFaceJobResult& InResult) {
    switch (InResult.Code)
    {
    case EMetaHumanFaceErrorCode::None:
        return 0;
    case EMetaHumanFaceErrorCode::InvalidRequest:
    case EMetaHumanFaceErrorCode::FramesNotFound:
    case EMetaHumanFaceErrorCode::FramesFormatMismatch:
    case EMetaHumanFaceErrorCode::FrameCountMismatch:
    case EMetaHumanFaceErrorCode::CaptureDataBuildFailed:
        return 2;
    case EMetaHumanFaceErrorCode::MetaHumanModelMissing:
    case EMetaHumanFaceErrorCode::InferenceRuntimeMissing:
        return 3;
    case EMetaHumanFaceErrorCode::ExportFailed:
    case EMetaHumanFaceErrorCode::FileSystemFailed:
        return 5;
    default:
        return 4;
    }
}

void UMetaHumanFaceCommandlet::Report(const FMetaHumanFaceJobResult& InResult) {
    if (InResult.Code == EMetaHumanFaceErrorCode::None)
    {
        UE_LOG(
            LogDoodleMetaHumanFaceCommandlet, Display,
            TEXT("MetaHuman face capture finished: %d frames, %d curves, %.1fs\n  curves  %s\n  summary %s\n  log     %s"),
            InResult.FrameCount, InResult.CurveCount, InResult.ElapsedSeconds, *InResult.CurvesPath,
            *InResult.SummaryPath, *InResult.LogPath
        );

        if (!InResult.DroppedFrameRanges.IsEmpty())
        {
            UE_LOG(
                LogDoodleMetaHumanFaceCommandlet, Warning, TEXT("frames without animation data: %s"),
                *DescribeRanges(InResult.DroppedFrameRanges)
            );
        }

        return;
    }

    UE_LOG(
        LogDoodleMetaHumanFaceCommandlet, Error, TEXT("MetaHuman face capture failed at stage %s, code %s\n  %s"),
        StageToString(InResult.Stage), ErrorCodeToString(InResult.Code), *InResult.Detail
    );

    if (!InResult.MissingModelAssets.IsEmpty())
    {
        UE_LOG(
            LogDoodleMetaHumanFaceCommandlet, Error, TEXT("missing values: %s"),
            *FString::Join(InResult.MissingModelAssets, TEXT(", "))
        );
    }

    if (!InResult.DroppedFrameRanges.IsEmpty())
    {
        UE_LOG(
            LogDoodleMetaHumanFaceCommandlet, Warning, TEXT("frames without animation data: %s"),
            *DescribeRanges(InResult.DroppedFrameRanges)
        );
    }

    if (!InResult.LogPath.IsEmpty())
    {
        UE_LOG(LogDoodleMetaHumanFaceCommandlet, Display, TEXT("log %s"), *InResult.LogPath);
    }
}
