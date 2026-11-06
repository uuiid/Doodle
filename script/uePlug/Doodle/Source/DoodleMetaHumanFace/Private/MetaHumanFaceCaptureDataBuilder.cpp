#include "MetaHumanFaceCaptureDataBuilder.h"

#include "CaptureData.h"
#include "FrameRange.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImgMediaSource.h"
#include "Internationalization/Regex.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrackingPathUtils.h"
#include "UObject/Package.h"

namespace
{
const TCHAR* const SequenceFilePattern = TEXT("^(([a-zA-Z0-9]*[_\\s-])*)([0-9]+)[a-zA-Z_\\s-]*\\.[a-zA-Z]+$");

TArray<FString> GatherSequenceFiles(const FString& InFramesDir)
{
    TArray<FString> AllFileNames;
    IFileManager::Get().FindFiles(AllFileNames, *FPaths::Combine(InFramesDir, TEXT("*.*")), true, false);
    return AllFileNames;
}

FString JoinErrors(const TArray<FString>& InErrors)
{
    return FString::Join(InErrors, TEXT("\n"));
}

FMetaHumanFaceJobResult MakeFailure(EMetaHumanFaceErrorCode InCode, const FString& InDetail)
{
    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Building;
    Result.Code = InCode;
    Result.Detail = InDetail;
    Result.LogPath = TEXT("");
    return Result;
}

TValueOrError<FMetaHumanFaceBuiltCaptureData, FMetaHumanFaceBuildError> MakeBuildError(
    EMetaHumanFaceBuildErrorCode InCode, FString InDetail,
    const TArray<FMetaHumanFaceFrameRange>& InDroppedFrameRanges = TArray<FMetaHumanFaceFrameRange>()
) {
    FMetaHumanFaceBuildError Error;
    Error.Code = InCode;
    Error.Detail = MoveTemp(InDetail);
    Error.DroppedFrameRanges = InDroppedFrameRanges;
    return MakeError(MoveTemp(Error));
}

void AppendRangeGap(TArray<FMetaHumanFaceFrameRange>& InOutGaps, int32 InStart, int32 InEnd)
{
    if (InStart > InEnd)
    {
        return;
    }

    if (!InOutGaps.IsEmpty() && InOutGaps.Last().End + 1 == InStart)
    {
        InOutGaps.Last().End = InEnd;
        return;
    }

    FMetaHumanFaceFrameRange Gap;
    Gap.Start = InStart;
    Gap.End = InEnd;
    InOutGaps.Add(Gap);
}

TArray<FFrameRange> ToCaptureExcludedFrames(const TArray<FMetaHumanFaceFrameRange>& InGaps)
{
    TArray<FFrameRange> Excluded;
    Excluded.Reserve(InGaps.Num());

    for (const FMetaHumanFaceFrameRange& Gap : InGaps)
    {
        FFrameRange Range;
        Range.Name = TEXT("Doodle Sequence Gap");
        Range.StartFrame = Gap.Start;
        Range.EndFrame = Gap.End;
        Excluded.Add(Range);
    }

    return Excluded;
}

FTimecode MakeNonZeroTimecode(double InSeconds, const FFrameRate& InFrameRate)
{
    const double BaseSeconds = FMath::Max(0.0, InSeconds);
    FTimecode Timecode(BaseSeconds, InFrameRate, false);

    if (Timecode == FTimecode())
    {
        Timecode = FTimecode(FMath::Max(1.0, BaseSeconds), InFrameRate, false);
    }

    return Timecode;
}
}

FMetaHumanFaceCaptureDataBuilder::FMetaHumanFaceCaptureDataBuilder() {
}

FMetaHumanFaceCaptureDataBuilder::~FMetaHumanFaceCaptureDataBuilder() {
}

TValueOrError<FMetaHumanFaceCaptureDataBuilder::FSequenceScan, FMetaHumanFaceScanError>
FMetaHumanFaceCaptureDataBuilder::MakeScanError(
    EMetaHumanFaceScanErrorCode InCode, FString InDetail,
    const TArray<FMetaHumanFaceFrameRange>& InDroppedFrameRanges
) {
    FMetaHumanFaceScanError Error;
    Error.Code = InCode;
    Error.Detail = MoveTemp(InDetail);
    Error.DroppedFrameRanges = InDroppedFrameRanges;
    return MakeError(MoveTemp(Error));
}

TValueOrError<FMetaHumanFaceCaptureDataBuilder::FSequenceScan, FMetaHumanFaceScanError>
FMetaHumanFaceCaptureDataBuilder::ScanSequence(
    const FMetaHumanFaceJobRequest& InRequest, bool bInInspectImageFiles
) {
    FSequenceScan Scan;

    if (InRequest.FramesDir.IsEmpty() || !FPaths::DirectoryExists(InRequest.FramesDir))
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesNotFound,
            FString::Printf(TEXT("frames directory does not exist: %s"), *InRequest.FramesDir), Scan.Gaps
        );
    }

    Scan.FileNames = ::GatherSequenceFiles(InRequest.FramesDir);
    if (Scan.FileNames.IsEmpty())
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesNotFound,
            FString::Printf(TEXT("frames directory is empty: %s"), *InRequest.FramesDir), Scan.Gaps
        );
    }

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

    TArray<FString> ImageFileNames;
    ImageFileNames.Reserve(Scan.FileNames.Num());
    for (const FString& FileName : Scan.FileNames)
    {
        if (ImageWrapperModule.GetImageFormatFromExtension(*FPaths::GetExtension(FileName)) != EImageFormat::Invalid)
        {
            ImageFileNames.Add(FileName);
        }
    }

    if (ImageFileNames.IsEmpty())
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesNotFound,
            FString::Printf(TEXT("no readable image file found in %s"), *InRequest.FramesDir), Scan.Gaps
        );
    }

    ImageFileNames.Sort();
    Scan.FileNames = ImageFileNames;

    const FRegexPattern FilenamePattern(SequenceFilePattern);

    TMap<int32, FString> FrameNumberToFileName;
    TArray<FString> FilenameErrors;
    int32 MinDigits = MAX_int32;
    int32 MaxDigits = 0;

    for (const FString& FileName : Scan.FileNames)
    {
        FRegexMatcher Matcher(FilenamePattern, FileName);
        if (!Matcher.FindNext())
        {
            FilenameErrors.Add(FString::Printf(
                TEXT("file name does not match the required image sequence form <prefix><number>.<ext>: %s"), *FileName
                ));
            continue;
        }

        const FString Digits = Matcher.GetCaptureGroup(3);
        const int32 FrameNumber = FCString::Atoi(*Digits);

        MinDigits = FMath::Min(MinDigits, Digits.Len());
        MaxDigits = FMath::Max(MaxDigits, Digits.Len());

        if (FrameNumberToFileName.Contains(FrameNumber))
        {
            FilenameErrors.Add(FString::Printf(
                TEXT("frame number %d appears twice: %s and %s"), FrameNumber,
                *FrameNumberToFileName[FrameNumber], *FileName
                ));
            continue;
        }

        FrameNumberToFileName.Add(FrameNumber, FileName);
    }

    if (!FilenameErrors.IsEmpty())
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesFormatMismatch, JoinErrors(FilenameErrors), Scan.Gaps
        );
    }

    if (MinDigits != MaxDigits)
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesFormatMismatch,
            FString::Printf(
                TEXT("frame number digit count is not consistent in %s (%d digits vs %d digits)"),
                *InRequest.FramesDir, MinDigits, MaxDigits
            ),
            Scan.Gaps
        );
    }

    Scan.DigitCount = MinDigits;

    TArray<int32> FrameNumbers;
    FrameNumberToFileName.GetKeys(FrameNumbers);
    FrameNumbers.Sort();

    Scan.StartFrame = FrameNumbers[0];
    Scan.NumFrames = FrameNumbers.Num();

    for (int32 Index = 0; Index < FrameNumbers.Num(); ++Index)
    {
        const int32 ExpectedFrameNumber = Scan.StartFrame + Index;
        if (FrameNumbers[Index] != ExpectedFrameNumber)
        {
            AppendRangeGap(Scan.Gaps, ExpectedFrameNumber, FrameNumbers[Index] - 1);
        }
    }

    if (!Scan.Gaps.IsEmpty())
    {
        TArray<FString> GapTexts;
        for (const FMetaHumanFaceFrameRange& Gap : Scan.Gaps)
        {
            GapTexts.Add(FString::Printf(TEXT("%d-%d"), Gap.Start, Gap.End));
        }

        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesNotFound,
            FString::Printf(
                TEXT("frame numbering is not contiguous, missing frames: %s"),
                *FString::Join(GapTexts, TEXT(", "))
            ),
            Scan.Gaps
        );
    }

    if (!bInInspectImageFiles)
    {
        return MakeValue(MoveTemp(Scan));
    }

    Scan.Extension = FPaths::GetExtension(Scan.FileNames[0]).ToLower();

    const FString SampleImagePath = FPaths::Combine(InRequest.FramesDir, Scan.FileNames[0]);
    TArray<uint8> RawFileData;
    if (!FFileHelper::LoadFileToArray(RawFileData, *SampleImagePath))
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesFormatMismatch,
            FString::Printf(TEXT("failed to read image file %s"), *SampleImagePath), Scan.Gaps
        );
    }

    Scan.Format = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());

    if (Scan.Format == EImageFormat::Invalid)
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesFormatMismatch,
            FString::Printf(TEXT("unrecognised image format in %s"), *SampleImagePath), Scan.Gaps
        );
    }

    if (Scan.Extension != TEXT("bmp") && Scan.Extension != TEXT("dds") && Scan.Extension != TEXT("exr") &&
        Scan.Extension != TEXT("jpg") && Scan.Extension != TEXT("jpeg") && Scan.Extension != TEXT("png"))
    {
        return MakeScanError(
            EMetaHumanFaceScanErrorCode::FramesFormatMismatch,
            FString::Printf(
                TEXT("image extension %s is not readable by ImgMedia, use bmp, dds, exr, jpg or png"),
                *Scan.Extension
            ),
            Scan.Gaps
        );
    }

    return MakeValue(MoveTemp(Scan));
}

FString FMetaHumanFaceCaptureDataBuilder::ResolveOuterPackageName(const FMetaHumanFaceJobRequest& InRequest) {
    if (!InRequest.CaptureDataOuter.IsEmpty())
    {
        return InRequest.CaptureDataOuter;
    }

    return FString::Printf(TEXT("%s/Package"), *InRequest.OutDir);
}

FTimecode FMetaHumanFaceCaptureDataBuilder::ResolveStartTimecode(
    const FMetaHumanFaceJobRequest& InRequest, const FSequenceScan& InScan
) {
    const FFrameRate FrameRate(static_cast<int32>(InRequest.FpsNum), static_cast<int32>(InRequest.FpsDen));

    const FString MetaPath = FPaths::Combine(InRequest.OutDir, TEXT("frames_meta.json"));
    FString MetaText;
    if (!MetaPath.IsEmpty() && FFileHelper::LoadFileToString(MetaText, *MetaPath))
    {
        TSharedPtr<FJsonObject> MetaObject;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MetaText);
        if (FJsonSerializer::Deserialize(Reader, MetaObject) && MetaObject.IsValid())
        {
            int32 FirstFrameOfFile = 0;
            if (MetaObject->TryGetNumberField(TEXT("first_frame"), FirstFrameOfFile))
            {
                return MakeNonZeroTimecode(static_cast<double>(FirstFrameOfFile) / FrameRate.AsDecimal(), FrameRate);
            }
        }
    }

    if (InScan.StartFrame != 0)
    {
        return MakeNonZeroTimecode(static_cast<double>(InScan.StartFrame) / FrameRate.AsDecimal(), FrameRate);
    }

    const double OffsetSeconds = static_cast<double>(FDateTime::Now().GetTicks());
    return MakeNonZeroTimecode(OffsetSeconds, FrameRate);
}

FMetaHumanFaceJobResult FMetaHumanFaceCaptureDataBuilder::Validate(const FMetaHumanFaceJobRequest& InRequest) const {
    TArray<FString> Errors;

    if (InRequest.FramesDir.IsEmpty())
    {
        Errors.Add(TEXT("frames directory is empty"));
    }
    else if (FPaths::IsRelative(InRequest.FramesDir))
    {
        Errors.Add(FString::Printf(TEXT("frames directory is not absolute: %s"), *InRequest.FramesDir));
    }

    if (InRequest.OutDir.IsEmpty())
    {
        Errors.Add(TEXT("output directory is empty"));
    }
    else if (FPaths::IsRelative(InRequest.OutDir))
    {
        Errors.Add(FString::Printf(TEXT("output directory is not absolute: %s"), *InRequest.OutDir));
    }

    if (InRequest.FpsNum == 0 || InRequest.FpsDen == 0)
    {
        Errors.Add(FString::Printf(TEXT("frame rate is invalid: %u/%u"), InRequest.FpsNum, InRequest.FpsDen));
    }

    if (!Errors.IsEmpty())
    {
        return MakeFailure(EMetaHumanFaceErrorCode::InvalidRequest, JoinErrors(Errors));
    }

    const TValueOrError<FSequenceScan, FMetaHumanFaceScanError> ScanResult = ScanSequence(InRequest, true);

    if (ScanResult.HasError())
    {
        const FMetaHumanFaceScanError& ScanError = ScanResult.GetError();
        const EMetaHumanFaceErrorCode Code = ScanError.Code == EMetaHumanFaceScanErrorCode::FramesNotFound
                                                 ? EMetaHumanFaceErrorCode::FramesNotFound
                                                 : EMetaHumanFaceErrorCode::FramesFormatMismatch;
        return MakeFailure(Code, ScanError.Detail);
    }

    const FSequenceScan& Scan = ScanResult.GetValue();

    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Building;
    Result.Code = EMetaHumanFaceErrorCode::None;
    Result.FrameCount = Scan.NumFrames;
    Result.DroppedFrameRanges = Scan.Gaps;
    Result.Detail = FString::Printf(
        TEXT("frames=%d start=%d digits=%d extension=%s"),
        Scan.NumFrames, Scan.StartFrame, Scan.DigitCount, *Scan.Extension
        );
    Result.Detail += FString::Printf(TEXT(" format=%d"), static_cast<int32>(Scan.Format));
    return Result;
}

TValueOrError<FMetaHumanFaceBuiltCaptureData, FMetaHumanFaceBuildError>
FMetaHumanFaceCaptureDataBuilder::BuildWithTimecode(
    const FMetaHumanFaceJobRequest& InRequest, const FSequenceScan& InScan, const FTimecode& InStartTimecode
) const {
    const FString OuterPackageName = ResolveOuterPackageName(InRequest);

    UPackage* OuterPackage = FindObject<UPackage>(nullptr, *OuterPackageName);
    if (OuterPackage == nullptr)
    {
        OuterPackage = CreatePackage(*OuterPackageName);
    }

    if (OuterPackage == nullptr)
    {
        return MakeBuildError(
            EMetaHumanFaceBuildErrorCode::FileSystemFailed,
            FString::Printf(TEXT("failed to create package %s"), *OuterPackageName), InScan.Gaps
        );
    }

    UImgMediaSource* ImageSource = NewObject<UImgMediaSource>(OuterPackage);
    ImageSource->SetTokenizedSequencePath(InRequest.FramesDir);
    ImageSource->FrameRateOverride = FFrameRate(static_cast<int32>(InRequest.FpsNum), static_cast<int32>(InRequest.FpsDen));
    ImageSource->StartTimecode = InStartTimecode;
    ImageSource->bFillGapsInSequence = false;

    UFootageCaptureData* CaptureData = NewObject<UFootageCaptureData>(OuterPackage);
    CaptureData->ImageSequences.Reset();
    CaptureData->DepthSequences.Reset();
    CaptureData->AudioTracks.Reset();
    CaptureData->CameraCalibrations.Reset();
    CaptureData->ImageSequences.Add(ImageSource);
    CaptureData->CaptureExcludedFrames = ToCaptureExcludedFrames(InScan.Gaps);
    CaptureData->Metadata.FrameRate = InRequest.FpsDen != 0
                                          ? static_cast<double>(InRequest.FpsNum) / static_cast<double>(InRequest.FpsDen)
                                          : 0.0;
    CaptureData->Metadata.DeviceClass = EFootageDeviceClass::Unspecified;
    CaptureData->Metadata.DeviceModelName = TEXT("Doodle Offline");

    TArray<FString> Errors;

    const UFootageCaptureData::FVerifyResult VerifyResult =
        CaptureData->VerifyData(UCaptureData::EInitializedCheck::ImageSequencesOnly);
    if (VerifyResult.HasError())
    {
        Errors.Add(FString::Printf(TEXT("capture data verification failed: %s"), *VerifyResult.GetError()));
    }

    FString TrackingFilePath;
    int32 TrackingFrameOffset = 0;
    int32 TrackingNumFrames = 0;
    if (!FTrackingPathUtils::GetTrackingFilePathAndInfo(
            ImageSource, TrackingFilePath, TrackingFrameOffset, TrackingNumFrames
        ))
    {
        Errors.Add(FString::Printf(
            TEXT("failed to resolve the image sequence pattern in %s"), *InRequest.FramesDir
            ));
    }
    else
    {
        if (TrackingFrameOffset != InScan.StartFrame)
        {
            Errors.Add(FString::Printf(
                TEXT("tracking frame offset %d differs from the sequence start frame %d"), TrackingFrameOffset,
                InScan.StartFrame
                ));
        }

        if (TrackingNumFrames != InScan.NumFrames)
        {
            Errors.Add(FString::Printf(
                TEXT("tracking frame count %d differs from the scanned frame count %d"), TrackingNumFrames,
                InScan.NumFrames
                ));
        }
    }

    if (!Errors.IsEmpty())
    {
        return MakeBuildError(
            EMetaHumanFaceBuildErrorCode::CaptureDataBuildFailed, JoinErrors(Errors), InScan.Gaps
        );
    }

    FMetaHumanFaceBuiltCaptureData BuiltCaptureData;
    BuiltCaptureData.CaptureData = CaptureData;
    BuiltCaptureData.OuterPackageName = OuterPackageName;
    BuiltCaptureData.StartFrame = InScan.StartFrame;
    BuiltCaptureData.NumFrames = InScan.NumFrames;
    BuiltCaptureData.DroppedFrameRanges = InScan.Gaps;
    BuiltCaptureData.StartTimecode = InStartTimecode;
    return MakeValue(MoveTemp(BuiltCaptureData));
}

TValueOrError<FMetaHumanFaceBuiltCaptureData, FMetaHumanFaceBuildError> FMetaHumanFaceCaptureDataBuilder::Build(
    const FMetaHumanFaceJobRequest& InRequest
) const {
    if (InRequest.FpsNum == 0 || InRequest.FpsDen == 0)
    {
        return MakeBuildError(
            EMetaHumanFaceBuildErrorCode::InvalidRequest,
            FString::Printf(TEXT("frame rate is invalid: %u/%u"), InRequest.FpsNum, InRequest.FpsDen)
        );
    }

    const TValueOrError<FSequenceScan, FMetaHumanFaceScanError> ScanResult = ScanSequence(InRequest, true);

    if (ScanResult.HasError())
    {
        const FMetaHumanFaceScanError& ScanError = ScanResult.GetError();
        const EMetaHumanFaceBuildErrorCode Code = ScanError.Code == EMetaHumanFaceScanErrorCode::FramesNotFound
                                                      ? EMetaHumanFaceBuildErrorCode::FramesNotFound
                                                      : EMetaHumanFaceBuildErrorCode::FramesFormatMismatch;
        return MakeBuildError(Code, ScanError.Detail, ScanError.DroppedFrameRanges);
    }

    const FSequenceScan& Scan = ScanResult.GetValue();

    const FTimecode StartTimecode = ResolveStartTimecode(InRequest, Scan);

    return BuildWithTimecode(InRequest, Scan, StartTimecode);
}

void FMetaHumanFaceCaptureDataBuilder::Release(const FMetaHumanFaceJobRequest& InRequest) const {
    const FString OuterPackageName = ResolveOuterPackageName(InRequest);

    if (UPackage* OuterPackage = FindObject<UPackage>(nullptr, *OuterPackageName))
    {
        OuterPackage->MarkAsGarbage();
    }
}
