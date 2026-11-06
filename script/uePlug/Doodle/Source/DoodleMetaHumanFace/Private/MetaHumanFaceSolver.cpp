#include "MetaHumanFaceSolver.h"

#include "CaptureData.h"
#include "FrameRange.h"
#include "FrameAnimationData.h"
#include "ImgMediaSource.h"
#include "MetaHumanSupportedRHI.h"
#include "MetaHumanPerformance.h"
#include "MetaHumanRealtimeSmoothing.h"
#include "Misc/ConfigCacheIni.h"
#include "NNE.h"
#include "NNERuntimeGPU.h"
#include "NNETypes.h"
#include "Pipeline/PipelineData.h"
#include "Pipeline/DataTreeTypes.h"
#include "RHI.h"
#include "UObject/Package.h"

namespace
{
    const TCHAR* const SettingsSection = TEXT("/Script/DoodleMetaHumanFace.DoodleMetaHumanFaceSettings");
    const TCHAR* const ModelAssetKey = TEXT("MetaHumanModelAsset");
    const TCHAR* const InferenceRuntimeKey = TEXT("NNEInferenceRuntime");
    const TCHAR* const DefaultModelAsset = TEXT("/MetaHumanCoreTech/GenericTracker/FaceDetector.FaceDetector");
    const TCHAR* const DefaultInferenceRuntime = TEXT("NNERuntimeORTDml");
    const TCHAR* const DefaultSmoothingAsset = TEXT("/MetaHumanCoreTech/RealtimeMono/DefaultSmoothing.DefaultSmoothing");

    TArray<FString> ExpectedHeadCurveNames()
    {
        return {TEXT("HeadYaw"),  TEXT("HeadPitch"),        TEXT("HeadRoll"),
                TEXT("HeadTranslationX"), TEXT("HeadTranslationY"), TEXT("HeadTranslationZ")};
    }

    void AppendHeadPoseCurves(const FTransform& InPose, TArray<FString>& InOutNames, TArray<float>& InOutValues)
    {
        const FRotator Rotation = InPose.Rotator();
        const FVector Translation = InPose.GetTranslation();

        InOutNames.Add(TEXT("HeadYaw"));
        InOutValues.Add(static_cast<float>(Rotation.Yaw));
        InOutNames.Add(TEXT("HeadPitch"));
        InOutValues.Add(static_cast<float>(Rotation.Pitch));
        InOutNames.Add(TEXT("HeadRoll"));
        InOutValues.Add(static_cast<float>(Rotation.Roll));
        InOutNames.Add(TEXT("HeadTranslationX"));
        InOutValues.Add(static_cast<float>(Translation.X));
        InOutNames.Add(TEXT("HeadTranslationY"));
        InOutValues.Add(static_cast<float>(Translation.Y));
        InOutNames.Add(TEXT("HeadTranslationZ"));
        InOutValues.Add(static_cast<float>(Translation.Z));
    }

    FString JoinStrings(const TArray<FString>& InValues)
    {
        return FString::Join(InValues, TEXT(", "));
    }

    FString ReadSetting(const TCHAR* InKey, const TCHAR* InDefaultValue)
    {
        FString Value;
        if (GConfig != nullptr && GConfig->GetString(SettingsSection, InKey, Value, GEngineIni) && !Value.IsEmpty())
        {
            return Value;
        }

        return InDefaultValue;
    }

    TArray<FString> CollectMissingValues()
    {
        TArray<FString> Missing;

        const FString ModelAssetPath = ReadSetting(ModelAssetKey, DefaultModelAsset);
        if (LoadObject<UObject>(GetTransientPackage(), *ModelAssetPath) == nullptr)
        {
            Missing.Add(ModelAssetPath);
        }

        const FString InferenceRuntimeName = ReadSetting(InferenceRuntimeKey, DefaultInferenceRuntime);
        if (!UE::NNE::GetRuntime<INNERuntimeGPU>(InferenceRuntimeName).IsValid())
        {
            Missing.Add(InferenceRuntimeName);
        }

        return Missing;
    }

    FString DescribeUnsupportedRHI()
    {
        const TCHAR* const CurrentRHI = GDynamicRHI != nullptr ? GDynamicRHI->GetName() : TEXT("none");
        const TCHAR* const SupportedRHIs = TEXT("D3D12");

        return FString::Printf(
            TEXT("the active RHI is %s but MetaHuman face solving requires %s; pass "
                 "-AllowCommandletRendering -RenderOffscreen and drop -unattended / -nullrhi, or pass "
                 "-ExecCmds=\"mh.Core.CheckRHI 0\" to skip the check at your own risk"),
            CurrentRHI, SupportedRHIs
        );
    }

    TArray<FMetaHumanFaceFrameRange> IntersectWithExcludedFrames(
        const TRange<FFrameNumber>& InRange, const TArray<FFrameRange>& InExcludedFrames
    ) {
        TArray<FMetaHumanFaceFrameRange> Result;

        if (!InRange.HasLowerBound() || !InRange.HasUpperBound())
        {
            return Result;
        }

        const int32 Lower = InRange.GetLowerBoundValue().Value;
        const int32 Upper = InRange.GetUpperBoundValue().Value;

        for (const FFrameRange& Excluded : InExcludedFrames)
        {
            const int32 Start = FMath::Max(Lower, Excluded.StartFrame);
            const int32 End = FMath::Min(Upper - 1, Excluded.EndFrame);

            if (Start > End)
            {
                continue;
            }

            if (!Result.IsEmpty() && Result.Last().End + 1 >= Start)
            {
                Result.Last().End = FMath::Max(Result.Last().End, End);
            }
            else
            {
                FMetaHumanFaceFrameRange Range;
                Range.Start = Start;
                Range.End = End;
                Result.Add(Range);
            }
        }

        return Result;
    }
}

struct FMetaHumanFaceSolver::FImpl
{
    TWeakObjectPtr<UMetaHumanPerformance> Performance;
    FDelegateHandle OnProcessingFinishedHandle;
    FString OuterPackageName;
    int32 FirstFrame = 0;
    int32 FrameCount = 0;
    bool bRunStarted = false;
    bool bRunFinished = false;
    bool bCancelRequested = false;
    UE::MetaHuman::Pipeline::EPipelineExitStatus ExitStatus = UE::MetaHuman::Pipeline::EPipelineExitStatus::Unknown;
};

template <typename ValueType>
TValueOrError<ValueType, FMetaHumanFaceSolverError> FMetaHumanFaceSolver::MakeSolverError(
    EMetaHumanFaceSolverErrorCode InCode, FString InDetail, const TArray<FString>& InMissingValues
) {
    FMetaHumanFaceSolverError Error;
    Error.Code = InCode;
    Error.Detail = MoveTemp(InDetail);
    Error.MissingModelAssets = InMissingValues;
    return MakeError(MoveTemp(Error));
}

FMetaHumanFaceSolver::FMetaHumanFaceSolver() : Impl(MakeUnique<FImpl>()) {
}

FMetaHumanFaceSolver::~FMetaHumanFaceSolver() {
    if (UMetaHumanPerformance* Performance = Impl->Performance.Get())
    {
        if (Impl->OnProcessingFinishedHandle.IsValid())
        {
            Performance->OnProcessingFinished().Remove(Impl->OnProcessingFinishedHandle);
        }

        if (Performance->IsProcessing())
        {
            Performance->CancelPipeline();
        }
    }
}

const TArray<FString>& FMetaHumanFaceSolver::ModelAssetPaths() {
    static const TArray<FString> Paths = { DefaultModelAsset };
    return Paths;
}

FMetaHumanFaceJobResult FMetaHumanFaceSolver::Preflight() const {
    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Solving;
    Result.Code = EMetaHumanFaceErrorCode::None;

    Result.MissingModelAssets = CollectMissingValues();

    if (Result.MissingModelAssets.IsEmpty())
    {
        return Result;
    }

    const FString ModelAssetPath = ReadSetting(ModelAssetKey, DefaultModelAsset);
    const FString InferenceRuntimeName = ReadSetting(InferenceRuntimeKey, DefaultInferenceRuntime);

    if (Result.MissingModelAssets.Contains(InferenceRuntimeName))
    {
        Result.Code = EMetaHumanFaceErrorCode::InferenceRuntimeMissing;
    }
    else if (Result.MissingModelAssets.Contains(ModelAssetPath))
    {
        Result.Code = EMetaHumanFaceErrorCode::MetaHumanModelMissing;
    }
    else
    {
        Result.Code = EMetaHumanFaceErrorCode::MetaHumanModelMissing;
    }

    Result.Detail = FString::Printf(
        TEXT("missing values for MetaHuman face solving: %s"), *JoinStrings(Result.MissingModelAssets)
    );
    Result.Stage = EMetaHumanFaceStage::Failed;
    return Result;
}

TValueOrError<FMetaHumanFaceBuiltPerformance, FMetaHumanFaceSolverError> FMetaHumanFaceSolver::CreatePerformance(
    const FMetaHumanFaceJobRequest& InRequest, UFootageCaptureData* InCaptureData
) {
    if (InCaptureData == nullptr)
    {
        return MakeSolverError<FMetaHumanFaceBuiltPerformance>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady, TEXT("capture data is missing")
        );
    }

    const UFootageCaptureData::FVerifyResult VerifyResult =
        InCaptureData->VerifyData(UCaptureData::EInitializedCheck::ImageSequencesOnly);
    if (VerifyResult.HasError())
    {
        return MakeSolverError<FMetaHumanFaceBuiltPerformance>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady,
            FString::Printf(TEXT("capture data verification failed: %s"), *VerifyResult.GetError())
        );
    }

    const UImgMediaSource* ImageSequence = InCaptureData->ImageSequences[0].Get();
    if (ImageSequence == nullptr || !ImageSequence->FrameRateOverride.IsValid() ||
        ImageSequence->FrameRateOverride.Numerator == 0)
    {
        return MakeSolverError<FMetaHumanFaceBuiltPerformance>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady,
            TEXT("capture data image sequence has no valid frame rate")
        );
    }

    const FString OuterPackageName = !InRequest.AssetPackagePath.IsEmpty()
                                         ? InRequest.AssetPackagePath
                                         : FString::Printf(TEXT("%s/Performance"), *InRequest.OutDir);

    UPackage* OuterPackage = FindObject<UPackage>(nullptr, *OuterPackageName);
    if (OuterPackage == nullptr)
    {
        OuterPackage = CreatePackage(*OuterPackageName);
    }

    if (OuterPackage == nullptr)
    {
        return MakeSolverError<FMetaHumanFaceBuiltPerformance>(
            EMetaHumanFaceSolverErrorCode::PerformanceCreationFailed,
            FString::Printf(TEXT("failed to create package %s"), *OuterPackageName)
        );
    }

    UMetaHumanPerformance* Performance = NewObject<UMetaHumanPerformance>(OuterPackage);
    if (Performance == nullptr)
    {
        return MakeSolverError<FMetaHumanFaceBuiltPerformance>(
            EMetaHumanFaceSolverErrorCode::PerformanceCreationFailed,
            FString::Printf(TEXT("failed to create a performance in %s"), *OuterPackageName)
        );
    }

    Performance->InputType = EDataInputType::MonoFootage;
    Performance->FootageCaptureData = InCaptureData;
    Performance->bSkipTongueSolve = InRequest.bSkipTongueSolve;
    Performance->bHeadStabilization = InRequest.bHeadStabilize;
    Performance->HeadMovementMode = EPerformanceHeadMovementMode::TransformTrack;

    if (Performance->MonoSmoothingParams == nullptr)
    {
        Performance->MonoSmoothingParams =
            LoadObject<UMetaHumanRealtimeSmoothingParams>(GetTransientPackage(), DefaultSmoothingAsset);
    }

    FPropertyChangedEvent ChangeEvent(
        FindFProperty<FProperty>(UMetaHumanPerformance::StaticClass(), TEXT("FootageCaptureData"))
    );
    Performance->PostEditChangeProperty(ChangeEvent);

    const TRange<FFrameNumber>& ProcessingLimit = Performance->GetProcessingLimitFrameRange();
    if (!ProcessingLimit.HasLowerBound() || !ProcessingLimit.HasUpperBound() ||
        ProcessingLimit.GetLowerBoundValue().Value >= ProcessingLimit.GetUpperBoundValue().Value)
    {
        return MakeSolverError<FMetaHumanFaceBuiltPerformance>(
            EMetaHumanFaceSolverErrorCode::PerformanceCreationFailed,
            FString::Printf(
                TEXT("performance %s has an empty processing range, capture data was rejected by MetaHuman"),
                *OuterPackageName
            )
        );
    }

    if (Impl->Performance.IsValid() && Impl->Performance.Get() != Performance)
    {
        if (Impl->OnProcessingFinishedHandle.IsValid())
        {
            Impl->Performance->OnProcessingFinished().Remove(Impl->OnProcessingFinishedHandle);
            Impl->OnProcessingFinishedHandle.Reset();
        }
    }

    Impl->Performance = Performance;
    Impl->OuterPackageName = OuterPackageName;
    Impl->FirstFrame = ProcessingLimit.GetLowerBoundValue().Value;
    Impl->FrameCount = ProcessingLimit.GetUpperBoundValue().Value - Impl->FirstFrame;
    Impl->bRunStarted = false;
    Impl->bRunFinished = false;
    Impl->bCancelRequested = false;
    Impl->ExitStatus = UE::MetaHuman::Pipeline::EPipelineExitStatus::Unknown;

    FMetaHumanFaceBuiltPerformance Built;
    Built.Performance = Performance;
    Built.OuterPackageName = OuterPackageName;
    return MakeValue(MoveTemp(Built));
}

TValueOrError<TArray<FMetaHumanFaceFrameRange>, FMetaHumanFaceSolverError> FMetaHumanFaceSolver::Process(
    UFootageCaptureData* InCaptureData
) {
    UMetaHumanPerformance* Performance = Impl->Performance.Get();
    if (Performance == nullptr)
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::PerformanceCreationFailed, TEXT("performance has not been created")
        );
    }

    if (Performance->IsProcessing() && Impl->bRunStarted && !Impl->bRunFinished)
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::ProcessingRejected, TEXT("a run is already in progress")
        );
    }

    if (!FMetaHumanSupportedRHI::IsSupported())
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::ProcessingRejected, DescribeUnsupportedRHI()
        );
    }

    if (InCaptureData == nullptr || Performance->FootageCaptureData == nullptr)
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady, TEXT("capture data is missing")
        );
    }

    if (Performance->FootageCaptureData != InCaptureData)
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady,
            TEXT("the performance was created for a different capture data")
        );
    }

    const TRange<FFrameNumber>& ProcessingLimit = Performance->GetProcessingLimitFrameRange();
    if (!ProcessingLimit.HasLowerBound() || !ProcessingLimit.HasUpperBound())
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady, TEXT("processing range is not available")
        );
    }

    const int32 LimitLower = ProcessingLimit.GetLowerBoundValue().Value;
    const int32 LimitUpper = ProcessingLimit.GetUpperBoundValue().Value;

    if (LimitLower >= LimitUpper)
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady,
            FString::Printf(TEXT("the processing range %d-%d is empty"), LimitLower, LimitUpper - 1)
        );
    }

    Performance->StartFrameToProcess = static_cast<uint32>(LimitLower);
    Performance->EndFrameToProcess = static_cast<uint32>(LimitUpper);

    if (!Performance->CanProcess())
    {
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::ProcessingRejected, Performance->GetCannotProcessTooltipText().ToString()
        );
    }

    if (Impl->OnProcessingFinishedHandle.IsValid())
    {
        Performance->OnProcessingFinished().Remove(Impl->OnProcessingFinishedHandle);
        Impl->OnProcessingFinishedHandle.Reset();
    }

    Impl->OnProcessingFinishedHandle = Performance->OnProcessingFinished().AddLambda(
        [this](TSharedPtr<const UE::MetaHuman::Pipeline::FPipelineData> InPipelineData)
        {
            Impl->bRunFinished = true;
            Impl->ExitStatus = InPipelineData.IsValid()
                                   ? InPipelineData->GetExitStatus()
                                   : UE::MetaHuman::Pipeline::EPipelineExitStatus::ProcessError;
        }
    );

    Impl->FirstFrame = LimitLower;
    Impl->FrameCount = LimitUpper - LimitLower;
    Impl->bRunStarted = true;
    Impl->bRunFinished = false;
    Impl->bCancelRequested = false;
    Impl->ExitStatus = UE::MetaHuman::Pipeline::EPipelineExitStatus::Unknown;

    Performance->SetBlockingProcessing(false);

    const EStartPipelineErrorType StartError = Performance->StartPipeline(true);
    if (StartError == EStartPipelineErrorType::NoFrames)
    {
        Impl->bRunStarted = false;
        Performance->OnProcessingFinished().Remove(Impl->OnProcessingFinishedHandle);
        Impl->OnProcessingFinishedHandle.Reset();
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::CaptureDataNotReady, TEXT("the processing range has no frames")
        );
    }

    if (StartError == EStartPipelineErrorType::Disabled)
    {
        Impl->bRunStarted = false;
        Performance->OnProcessingFinished().Remove(Impl->OnProcessingFinishedHandle);
        Impl->OnProcessingFinishedHandle.Reset();
        return MakeSolverError<TArray<FMetaHumanFaceFrameRange>>(
            EMetaHumanFaceSolverErrorCode::ProcessingRejected, Performance->GetCannotProcessTooltipText().ToString()
        );
    }

    return MakeValue(
        IntersectWithExcludedFrames(
            TRange<FFrameNumber>(LimitLower, LimitUpper), InCaptureData->CaptureExcludedFrames
        )
    );
}

EMetaHumanFaceProcessState FMetaHumanFaceSolver::PollRun() {
    UMetaHumanPerformance* Performance = Impl->Performance.Get();
    if (Performance == nullptr)
    {
        return EMetaHumanFaceProcessState::Idle;
    }

    if (!Impl->bRunStarted)
    {
        return EMetaHumanFaceProcessState::Idle;
    }

    if (!Impl->bRunFinished)
    {
        return EMetaHumanFaceProcessState::Running;
    }

    if (Impl->bCancelRequested)
    {
        return EMetaHumanFaceProcessState::Cancelled;
    }

    if (Impl->ExitStatus == UE::MetaHuman::Pipeline::EPipelineExitStatus::Ok)
    {
        return EMetaHumanFaceProcessState::Succeeded;
    }

    if (Impl->ExitStatus == UE::MetaHuman::Pipeline::EPipelineExitStatus::Aborted)
    {
        return EMetaHumanFaceProcessState::Cancelled;
    }

    return EMetaHumanFaceProcessState::Failed;
}

TValueOrError<FMetaHumanFaceSolverResult, FMetaHumanFaceSolverError> FMetaHumanFaceSolver::CollectCurveSet() const {
    UMetaHumanPerformance* Performance = Impl->Performance.Get();
    if (Performance == nullptr)
    {
        return MakeSolverError<FMetaHumanFaceSolverResult>(
            EMetaHumanFaceSolverErrorCode::NoAnimationData, TEXT("performance has not been created")
        );
    }

    if (!Impl->bRunFinished || Impl->ExitStatus != UE::MetaHuman::Pipeline::EPipelineExitStatus::Ok)
    {
        return MakeSolverError<FMetaHumanFaceSolverResult>(
            EMetaHumanFaceSolverErrorCode::ProcessingFailed,
            FString::Printf(TEXT("the run has not finished successfully (exit status %d)"), static_cast<int32>(Impl->ExitStatus))
        );
    }

    if (!Performance->ContainsAnimationData() || Performance->GetNumberOfProcessedFrames() == 0)
    {
        return MakeSolverError<FMetaHumanFaceSolverResult>(
            EMetaHumanFaceSolverErrorCode::NoAnimationData, TEXT("the performance produced no animation data")
        );
    }

    const TSet<FString> ProducedCurveNames = Performance->GetAnimationCurveNames();
    if (ProducedCurveNames.IsEmpty())
    {
        return MakeSolverError<FMetaHumanFaceSolverResult>(
            EMetaHumanFaceSolverErrorCode::NoAnimationData, TEXT("the performance produced no animation curves")
        );
    }

    FMetaHumanFaceSolverResult Result;

    for (const FString& HeadCurve : ExpectedHeadCurveNames())
    {
        Result.CurveNames.Add(HeadCurve);
    }

    TArray<FString> RemainingCurveNames;
    for (const FString& CurveName : ProducedCurveNames)
    {
        if (!Result.CurveNames.Contains(CurveName))
        {
            RemainingCurveNames.Add(CurveName);
        }
    }
    RemainingCurveNames.Sort();
    Result.CurveNames.Append(RemainingCurveNames);

    const TRange<FFrameNumber>& ProcessingLimit = Performance->GetProcessingLimitFrameRange();
    const int32 LimitLower = ProcessingLimit.GetLowerBoundValue().Value;
    const int32 LimitUpper = ProcessingLimit.GetUpperBoundValue().Value;

    const int32 BeginFrame = FMath::Max(Impl->FirstFrame, LimitLower);
    const int32 EndFrame = FMath::Min(Impl->FirstFrame + Impl->FrameCount, LimitUpper);

    if (BeginFrame >= EndFrame)
    {
        return MakeSolverError<FMetaHumanFaceSolverResult>(
            EMetaHumanFaceSolverErrorCode::NoAnimationData, TEXT("the processed frame range is empty")
        );
    }

    const TArray<FFrameAnimationData> AnimationData = Performance->GetAnimationData(0, -1);
    if (AnimationData.Num() < LimitUpper - LimitLower)
    {
        return MakeSolverError<FMetaHumanFaceSolverResult>(
            EMetaHumanFaceSolverErrorCode::NoAnimationData,
            FString::Printf(
                TEXT("animation data holds %d frames but the processing range needs %d frames"), AnimationData.Num(),
                LimitUpper - LimitLower
            )
        );
    }

    const int32 FrameCount = EndFrame - BeginFrame;
    const int32 CurveCount = Result.CurveNames.Num();
    Result.FirstFrame = BeginFrame;
    Result.FrameCount = FrameCount;
    Result.Values.Reserve(FrameCount * CurveCount);
    Result.RootPoses.Reserve(FrameCount);

    const FFrameRate FrameRate = Performance->FootageCaptureData != nullptr &&
                                         !Performance->FootageCaptureData->ImageSequences.IsEmpty() &&
                                         Performance->FootageCaptureData->ImageSequences[0] != nullptr
                                     ? Performance->FootageCaptureData->ImageSequences[0]->FrameRateOverride
                                     : FFrameRate(25, 1);
    Result.FpsNum = static_cast<uint32>(FrameRate.Numerator);
    Result.FpsDen = static_cast<uint32>(FrameRate.Denominator);

    for (int32 Frame = BeginFrame; Frame < EndFrame; ++Frame)
    {
        const FFrameAnimationData& FrameData = AnimationData[Frame - LimitLower];
        Result.RootPoses.Add(FrameData.Pose);

        bool bFrameEmpty = FrameData.AnimationData.IsEmpty();

        TArray<FString> HeadCurveNames;
        TArray<float> HeadCurveValues;
        AppendHeadPoseCurves(FrameData.Pose, HeadCurveNames, HeadCurveValues);
        Result.Values.Append(HeadCurveValues);

        for (int32 Index = HeadCurveNames.Num(); Index < CurveCount; ++Index)
        {
            const float* Value = FrameData.AnimationData.Find(Result.CurveNames[Index]);
            if (Value == nullptr)
            {
                bFrameEmpty = true;
                Result.Values.Add(0.0f);
            }
            else
            {
                Result.Values.Add(*Value);
            }
        }

        if (bFrameEmpty)
        {
            if (!Result.DroppedFrameRanges.IsEmpty() && Result.DroppedFrameRanges.Last().End + 1 == Frame)
            {
                Result.DroppedFrameRanges.Last().End = Frame;
            }
            else
            {
                FMetaHumanFaceFrameRange Range;
                Range.Start = Frame;
                Range.End = Frame;
                Result.DroppedFrameRanges.Add(Range);
            }
        }
    }

    return MakeValue(MoveTemp(Result));
}

bool FMetaHumanFaceSolver::IsProcessing() const {
    UMetaHumanPerformance* Performance = Impl->Performance.Get();
    return Performance != nullptr && Performance->IsProcessing();
}

void FMetaHumanFaceSolver::Cancel() {
    UMetaHumanPerformance* Performance = Impl->Performance.Get();
    if (Performance == nullptr)
    {
        return;
    }

    if (!Performance->IsProcessing())
    {
        return;
    }

    Impl->bCancelRequested = true;
    Performance->CancelPipeline();
}
