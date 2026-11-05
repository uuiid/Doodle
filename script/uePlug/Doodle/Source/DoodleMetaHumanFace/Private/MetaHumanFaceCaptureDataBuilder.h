#pragma once

#include "CoreMinimal.h"
#include "IImageWrapper.h"
#include "MetaHumanFaceTypes.h"

class UFootageCaptureData;

class FMetaHumanFaceCaptureDataBuilder
{
  public:
    FMetaHumanFaceCaptureDataBuilder();
    ~FMetaHumanFaceCaptureDataBuilder();

    FMetaHumanFaceCaptureDataBuilder(const FMetaHumanFaceCaptureDataBuilder&) = delete;
    FMetaHumanFaceCaptureDataBuilder(FMetaHumanFaceCaptureDataBuilder&&) = delete;
    FMetaHumanFaceCaptureDataBuilder& operator=(const FMetaHumanFaceCaptureDataBuilder&) = delete;
    FMetaHumanFaceCaptureDataBuilder& operator=(FMetaHumanFaceCaptureDataBuilder&&) = delete;

    FMetaHumanFaceJobResult Validate(const FMetaHumanFaceJobRequest& InRequest) const;
    TValueOrError<FMetaHumanFaceBuiltCaptureData, FMetaHumanFaceBuildError> Build(
        const FMetaHumanFaceJobRequest& InRequest
    ) const;
    void Release(const FMetaHumanFaceJobRequest& InRequest) const;

  private:
    struct FSequenceScan
    {
        TArray<FString> FileNames;
        EImageFormat Format = EImageFormat::Invalid;
        FString Extension;
        int32 StartFrame = 0;
        int32 NumFrames = 0;
        int32 DigitCount = 0;
        TArray<FMetaHumanFaceFrameRange> Gaps;
    };

    static TValueOrError<FSequenceScan, FMetaHumanFaceScanError> ScanSequence(
        const FMetaHumanFaceJobRequest& InRequest, bool bInInspectImageFiles
    );
    static TValueOrError<FSequenceScan, FMetaHumanFaceScanError> MakeScanError(
        EMetaHumanFaceScanErrorCode InCode, FString InDetail,
        const TArray<FMetaHumanFaceFrameRange>& InDroppedFrameRanges
    );
    static FString ResolveOuterPackageName(const FMetaHumanFaceJobRequest& InRequest);
    static FTimecode ResolveStartTimecode(
        const FMetaHumanFaceJobRequest& InRequest, const FSequenceScan& InScan
    );
    TValueOrError<FMetaHumanFaceBuiltCaptureData, FMetaHumanFaceBuildError> BuildWithTimecode(
        const FMetaHumanFaceJobRequest& InRequest, const FSequenceScan& InScan, const FTimecode& InStartTimecode
    ) const;
};
