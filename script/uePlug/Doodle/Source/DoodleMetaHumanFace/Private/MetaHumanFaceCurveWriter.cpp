#include "MetaHumanFaceCurveWriter.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/MemoryWriter.h"

namespace
{
const TCHAR* const CurvesMagic = TEXT("DHFC0001");
constexpr uint32 CurvesVersion = 1;
constexpr uint32 CurvesFlags = 2;
constexpr int32 CurveNameLength = 64;
constexpr int32 CurvesHeaderSize = 48;
constexpr int32 RootPoseStride = 28;

FMetaHumanFaceJobResult MakeWriteFailure(EMetaHumanFaceErrorCode InCode, const FString& InDetail)
{
    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Failed;
    Result.Code = InCode;
    Result.Detail = InDetail;
    return Result;
}

FString DescribeCurveNames(const TArray<FString>& InCurveNames, int32 InMaxCount)
{
    TArray<FString> Head;
    for (int32 Index = 0; Index < InCurveNames.Num() && Index < InMaxCount; ++Index)
    {
        Head.Add(InCurveNames[Index]);
    }

    return FString::Join(Head, TEXT(", "));
}

TValueOrError<void, FString> ValidateResult(const FMetaHumanFaceSolverResult& InResult)
{
    if (InResult.FpsNum == 0 || InResult.FpsDen == 0)
    {
        return MakeError(FString::Printf(TEXT("frame rate is invalid: %u/%u"), InResult.FpsNum, InResult.FpsDen));
    }

    if (InResult.FrameCount <= 0)
    {
        return MakeError(FString::Printf(TEXT("frame count must be positive, got %d"), InResult.FrameCount));
    }

    if (InResult.CurveNames.IsEmpty())
    {
        return MakeError(TEXT("curve names are empty"));
    }

    TArray<FString> InvalidNames;
    TSet<FString> UniqueNames;
    for (const FString& CurveName : InResult.CurveNames)
    {
        if (CurveName.IsEmpty() || CurveName.Len() >= CurveNameLength)
        {
            InvalidNames.Add(CurveName);
            continue;
        }

        if (UniqueNames.Contains(CurveName))
        {
            InvalidNames.Add(CurveName);
            continue;
        }

        UniqueNames.Add(CurveName);

        for (const TCHAR Character : CurveName)
        {
            if (Character < 0x20 || Character > 0x7E)
            {
                InvalidNames.Add(CurveName);
                break;
            }
        }
    }

    if (!InvalidNames.IsEmpty())
    {
        return MakeError(FString::Printf(
            TEXT("invalid curve names (empty, duplicated, longer than %d bytes or not printable ascii): %s"),
            CurveNameLength - 1, *DescribeCurveNames(InvalidNames, 3)
        ));
    }

    const int64 ExpectedValues = static_cast<int64>(InResult.FrameCount) * InResult.CurveNames.Num();
    if (InResult.Values.Num() != ExpectedValues)
    {
        return MakeError(FString::Printf(
            TEXT("curve values do not match the frame layout: expected %lld, got %d"), ExpectedValues,
            InResult.Values.Num()
        ));
    }

    if (InResult.RootPoses.Num() != InResult.FrameCount)
    {
        return MakeError(FString::Printf(
            TEXT("root poses do not match the frame count: expected %d, got %d"), InResult.FrameCount,
            InResult.RootPoses.Num()
        ));
    }

    return MakeValue();
}

void AppendUInt32(TArray<uint8>& InOutBytes, uint32 InValue) {
    InOutBytes.AddUninitialized(sizeof(uint32));
    FMemory::Memcpy(InOutBytes.GetData() + InOutBytes.Num() - sizeof(uint32), &InValue, sizeof(uint32));
}

void AppendFloat(TArray<uint8>& InOutBytes, float InValue) {
    InOutBytes.AddUninitialized(sizeof(float));
    FMemory::Memcpy(InOutBytes.GetData() + InOutBytes.Num() - sizeof(float), &InValue, sizeof(float));
}

void AppendMagic(TArray<uint8>& InOutBytes) {
    uint8 Magic[8] = {0};
    FTCHARToUTF8 Converted(CurvesMagic);
    FMemory::Memcpy(Magic, Converted.Get(), FMath::Min<int32>(Converted.Length(), 8));
    InOutBytes.Append(Magic, 8);
}

void AppendHeader(TArray<uint8>& InOutBytes, const FMetaHumanFaceSolverResult& InResult) {
    AppendMagic(InOutBytes);
    AppendUInt32(InOutBytes, CurvesVersion);
    AppendUInt32(InOutBytes, CurvesFlags);
    AppendUInt32(InOutBytes, static_cast<uint32>(InResult.FrameCount));
    AppendUInt32(InOutBytes, static_cast<uint32>(InResult.CurveNames.Num()));
    AppendUInt32(InOutBytes, InResult.FpsNum);
    AppendUInt32(InOutBytes, InResult.FpsDen);
    AppendFloat(
        InOutBytes, static_cast<float>(
                        static_cast<double>(InResult.FirstFrame) * static_cast<double>(InResult.FpsDen) /
                        static_cast<double>(InResult.FpsNum)
                    )
    );
    AppendUInt32(InOutBytes, 0);
    AppendUInt32(InOutBytes, 0);
    AppendUInt32(InOutBytes, 0);
}

void AppendCurveNames(TArray<uint8>& InOutBytes, const TArray<FString>& InCurveNames) {
    for (const FString& CurveName : InCurveNames)
    {
        const int32 Offset = InOutBytes.Num();
        InOutBytes.AddZeroed(CurveNameLength);
        const FTCHARToUTF8 Converted(*CurveName);
        FMemory::Memcpy(
            InOutBytes.GetData() + Offset, Converted.Get(), FMath::Min<int32>(Converted.Length(), CurveNameLength - 1)
        );
    }
}

void AppendValues(TArray<uint8>& InOutBytes, const TArray<float>& InValues) {
    for (const float Value : InValues)
    {
        AppendFloat(InOutBytes, Value);
    }
}

void AppendRootPoses(TArray<uint8>& InOutBytes, const TArray<FTransform>& InRootPoses) {
    for (const FTransform& RootPose : InRootPoses)
    {
        const FVector Translation = RootPose.GetTranslation();
        const FQuat Rotation = RootPose.GetRotation();

        AppendFloat(InOutBytes, static_cast<float>(Translation.X));
        AppendFloat(InOutBytes, static_cast<float>(Translation.Y));
        AppendFloat(InOutBytes, static_cast<float>(Translation.Z));
        AppendFloat(InOutBytes, static_cast<float>(Rotation.X));
        AppendFloat(InOutBytes, static_cast<float>(Rotation.Y));
        AppendFloat(InOutBytes, static_cast<float>(Rotation.Z));
        AppendFloat(InOutBytes, static_cast<float>(Rotation.W));
    }
}

bool ReadHeaderField(const TArray<uint8>& InBytes, int32 InOffset, uint32& OutValue) {
    if (InBytes.Num() < InOffset + static_cast<int32>(sizeof(uint32)))
    {
        return false;
    }

    FMemory::Memcpy(&OutValue, InBytes.GetData() + InOffset, sizeof(uint32));
    return true;
}

TValueOrError<void, FString> VerifyWrittenHeader(
    const TArray<uint8>& InBytes, const FMetaHumanFaceSolverResult& InResult, int64 InExpectedSize
) {
    if (InBytes.Num() != InExpectedSize)
    {
        return MakeError(FString::Printf(
            TEXT("written size %d does not match the expected size %lld"), InBytes.Num(), InExpectedSize
        ));
    }

    const uint8* ExpectedMagic = reinterpret_cast<const uint8*>("DHFC0001");
    if (FMemory::Memcmp(InBytes.GetData(), ExpectedMagic, 8) != 0)
    {
        return MakeError(TEXT("written magic does not match"));
    }

    const TArray<TPair<int32, uint32>> Expectations = {
        {8, CurvesVersion},
        {12, CurvesFlags},
        {16, static_cast<uint32>(InResult.FrameCount)},
        {20, static_cast<uint32>(InResult.CurveNames.Num())},
        {24, InResult.FpsNum},
        {28, InResult.FpsDen},
    };

    for (const TPair<int32, uint32>& Expectation : Expectations)
    {
        uint32 Actual = 0;
        if (!ReadHeaderField(InBytes, Expectation.Key, Actual))
        {
            return MakeError(FString::Printf(TEXT("written header is truncated at offset %d"), Expectation.Key));
        }

        if (Actual != Expectation.Value)
        {
            return MakeError(FString::Printf(
                TEXT("written header field at offset %d is %u, expected %u"), Expectation.Key, Actual,
                Expectation.Value
            ));
        }
    }

    return MakeValue();
}

FString MakeSummaryJson(const FMetaHumanFaceSolverResult& InResult) {
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes);

    const bool bSucceeded = [&Writer, &InResult]() {
        const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriter<>::Create(&Writer);
        JsonWriter->WriteObjectStart();
        JsonWriter->WriteValue(TEXT("magic"), CurvesMagic);
        JsonWriter->WriteValue(TEXT("version"), static_cast<int32>(CurvesVersion));
        JsonWriter->WriteValue(TEXT("flags"), static_cast<int32>(CurvesFlags));
        JsonWriter->WriteValue(TEXT("frame_count"), InResult.FrameCount);
        JsonWriter->WriteValue(TEXT("curve_count"), InResult.CurveNames.Num());
        JsonWriter->WriteValue(
            TEXT("fps"),
            FString::Printf(TEXT("%u/%u"), InResult.FpsNum, InResult.FpsDen)
        );
        JsonWriter->WriteValue(TEXT("start_frame"), InResult.FirstFrame);
        JsonWriter->WriteValue(
            TEXT("start_time_sec"),
            static_cast<double>(InResult.FirstFrame) * static_cast<double>(InResult.FpsDen) /
                static_cast<double>(InResult.FpsNum)
        );
        JsonWriter->WriteValue(
            TEXT("duration_sec"),
            static_cast<double>(InResult.FrameCount) * static_cast<double>(InResult.FpsDen) /
                static_cast<double>(InResult.FpsNum)
        );

        JsonWriter->WriteArrayStart(TEXT("dropped_frame_ranges"));
        for (const FMetaHumanFaceFrameRange& Range : InResult.DroppedFrameRanges)
        {
            JsonWriter->WriteObjectStart();
            JsonWriter->WriteValue(TEXT("start"), Range.Start);
            JsonWriter->WriteValue(TEXT("end"), Range.End);
            JsonWriter->WriteObjectEnd();
        }
        JsonWriter->WriteArrayEnd();

        JsonWriter->WriteArrayStart(TEXT("curves"));
        for (int32 CurveIndex = 0; CurveIndex < InResult.CurveNames.Num(); ++CurveIndex)
        {
            float Minimum = 0.0f;
            float Maximum = 0.0f;
            float First = 0.0f;
            float Last = 0.0f;

            for (int32 FrameIndex = 0; FrameIndex < InResult.FrameCount; ++FrameIndex)
            {
                const float Value = InResult.Values[FrameIndex * InResult.CurveNames.Num() + CurveIndex];

                if (FrameIndex == 0)
                {
                    Minimum = Value;
                    Maximum = Value;
                    First = Value;
                }
                else
                {
                    Minimum = FMath::Min(Minimum, Value);
                    Maximum = FMath::Max(Maximum, Value);
                }

                Last = Value;
            }

            JsonWriter->WriteObjectStart();
            JsonWriter->WriteValue(TEXT("name"), InResult.CurveNames[CurveIndex]);
            JsonWriter->WriteValue(TEXT("min"), Minimum);
            JsonWriter->WriteValue(TEXT("max"), Maximum);
            JsonWriter->WriteValue(TEXT("first"), First);
            JsonWriter->WriteValue(TEXT("last"), Last);
            JsonWriter->WriteObjectEnd();
        }
        JsonWriter->WriteArrayEnd();

        JsonWriter->WriteObjectEnd();
        return JsonWriter->Close();
    }();

    if (!bSucceeded)
    {
        return FString();
    }

    Bytes.Add(0);
    return FString(UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(Bytes.GetData())));
}
}

FMetaHumanFaceCurveWriter::FMetaHumanFaceCurveWriter() {
}

FMetaHumanFaceCurveWriter::~FMetaHumanFaceCurveWriter() {
}

FMetaHumanFaceJobResult FMetaHumanFaceCurveWriter::Write(
    const FMetaHumanFaceSolverResult& InResult, const FString& InCurvesPath, const FString& InSummaryPath
) const {
    const TValueOrError<void, FString> Validation = ValidateResult(InResult);
    if (Validation.HasError())
    {
        return MakeWriteFailure(EMetaHumanFaceErrorCode::InvalidRequest, Validation.GetError());
    }

    if (InCurvesPath.IsEmpty() || InSummaryPath.IsEmpty())
    {
        return MakeWriteFailure(EMetaHumanFaceErrorCode::InvalidRequest, TEXT("output paths are empty"));
    }

    const FString CurvesDirectory = FPaths::GetPath(InCurvesPath);
    if (!CurvesDirectory.IsEmpty() &&
        !IFileManager::Get().MakeDirectory(*CurvesDirectory, true))
    {
        return MakeWriteFailure(
            EMetaHumanFaceErrorCode::FileSystemFailed,
            FString::Printf(TEXT("failed to create directory %s"), *CurvesDirectory)
        );
    }

    const FString SummaryDirectory = FPaths::GetPath(InSummaryPath);
    if (!SummaryDirectory.IsEmpty() &&
        !IFileManager::Get().MakeDirectory(*SummaryDirectory, true))
    {
        return MakeWriteFailure(
            EMetaHumanFaceErrorCode::FileSystemFailed,
            FString::Printf(TEXT("failed to create directory %s"), *SummaryDirectory)
        );
    }

    TArray<uint8> Bytes;
    const int64 ExpectedSize = CurvesHeaderSize +
                               static_cast<int64>(CurveNameLength) * InResult.CurveNames.Num() +
                               static_cast<int64>(sizeof(float)) * InResult.Values.Num() +
                               static_cast<int64>(RootPoseStride) * InResult.RootPoses.Num();
    Bytes.Reserve(ExpectedSize);

    AppendHeader(Bytes, InResult);
    AppendCurveNames(Bytes, InResult.CurveNames);
    AppendValues(Bytes, InResult.Values);
    AppendRootPoses(Bytes, InResult.RootPoses);

    if (!FFileHelper::SaveArrayToFile(Bytes, *InCurvesPath))
    {
        return MakeWriteFailure(
            EMetaHumanFaceErrorCode::FileSystemFailed, FString::Printf(TEXT("failed to write %s"), *InCurvesPath)
        );
    }

    TArray<uint8> WrittenBytes;
    if (!FFileHelper::LoadFileToArray(WrittenBytes, *InCurvesPath))
    {
        IFileManager::Get().Delete(*InCurvesPath, false, true, true);
        return MakeWriteFailure(
            EMetaHumanFaceErrorCode::FileSystemFailed,
            FString::Printf(TEXT("failed to read back %s"), *InCurvesPath)
        );
    }

    const TValueOrError<void, FString> Verification = VerifyWrittenHeader(WrittenBytes, InResult, ExpectedSize);
    if (Verification.HasError())
    {
        IFileManager::Get().Delete(*InCurvesPath, false, true, true);
        return MakeWriteFailure(EMetaHumanFaceErrorCode::FileSystemFailed, Verification.GetError());
    }

    const FString SummaryJson = MakeSummaryJson(InResult);
    if (SummaryJson.IsEmpty())
    {
        IFileManager::Get().Delete(*InCurvesPath, false, true, true);
        return MakeWriteFailure(EMetaHumanFaceErrorCode::FileSystemFailed, TEXT("failed to serialize the summary"));
    }

    if (!FFileHelper::SaveStringToFile(SummaryJson, *InSummaryPath))
    {
        IFileManager::Get().Delete(*InCurvesPath, false, true, true);
        return MakeWriteFailure(
            EMetaHumanFaceErrorCode::FileSystemFailed, FString::Printf(TEXT("failed to write %s"), *InSummaryPath)
        );
    }

    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Done;
    Result.Code = EMetaHumanFaceErrorCode::None;
    Result.CurvesPath = InCurvesPath;
    Result.SummaryPath = InSummaryPath;
    Result.FrameCount = InResult.FrameCount;
    Result.CurveCount = InResult.CurveNames.Num();
    Result.DroppedFrameRanges = InResult.DroppedFrameRanges;
    return Result;
}
