#include "MetaHumanFaceCommandLine.h"

#include "Commandlets/Commandlet.h"
#include "Misc/Paths.h"

namespace
{
FMetaHumanFaceJobResult MakeParseFailure(const FString& InDetail) {
    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Failed;
    Result.Code = EMetaHumanFaceErrorCode::InvalidRequest;
    Result.Detail = InDetail;
    return Result;
}

FMetaHumanFaceJobResult MakeParseSuccess() {
    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Queued;
    Result.Code = EMetaHumanFaceErrorCode::None;
    return Result;
}

bool ParsePositiveInteger(const FString& InValue, uint32& OutValue) {
    if (InValue.IsEmpty() || InValue.Len() > 10)
    {
        return false;
    }

    uint64 Accumulated = 0;
    for (const TCHAR Character : InValue)
    {
        if (Character < TEXT('0') || Character > TEXT('9'))
        {
            return false;
        }

        Accumulated = Accumulated * 10 + static_cast<uint64>(Character - TEXT('0'));
        if (Accumulated > MAX_uint32)
        {
            return false;
        }
    }

    OutValue = static_cast<uint32>(Accumulated);
    return true;
}

bool ParseNonNegativeInteger(const FString& InValue, int32& OutValue) {
    uint32 Parsed = 0;
    if (!ParsePositiveInteger(InValue, Parsed))
    {
        return false;
    }

    if (Parsed > static_cast<uint32>(MAX_int32))
    {
        return false;
    }

    OutValue = static_cast<int32>(Parsed);
    return true;
}

bool IsHelpSwitch(const FString& InSwitch) {
    return InSwitch.Equals(TEXT("H"), ESearchCase::IgnoreCase) ||
           InSwitch.Equals(TEXT("Help"), ESearchCase::IgnoreCase) ||
           InSwitch.Equals(TEXT("?"), ESearchCase::IgnoreCase);
}

bool IsPackagePath(const FString& InValue) {
    if (InValue.IsEmpty() || !InValue.StartsWith(TEXT("/")))
    {
        return false;
    }

    const int32 LastSlash = InValue.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    if (LastSlash <= 0)
    {
        return false;
    }

    return !InValue.Mid(LastSlash + 1).Contains(TEXT("."));
}
}

FMetaHumanFaceCommandLine::FMetaHumanFaceCommandLine() {
}

FMetaHumanFaceCommandLine::~FMetaHumanFaceCommandLine() {
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::Parse(const FString& InCommandLine) {
    Request = FMetaHumanFaceJobRequest();

    TArray<FString> Tokens;
    TArray<FString> Switches;
    TMap<FString, FString> Params;
    UCommandlet::ParseCommandLine(*InCommandLine, Tokens, Switches, Params);

    for (const FString& Switch : Switches)
    {
        if (IsHelpSwitch(Switch))
        {
            return MakeParseFailure(Usage());
        }
    }

    TArray<FString> Errors;

    for (const FString& Switch : Switches)
    {
        if (Switch.Equals(TEXT("NoHeadStabilize"), ESearchCase::IgnoreCase))
        {
            Request.bHeadStabilize = false;
        }
        else if (Switch.Equals(TEXT("KeepTongueSolve"), ESearchCase::IgnoreCase))
        {
            Request.bSkipTongueSolve = false;
        }
        else if (!IsHelpSwitch(Switch))
        {
            Errors.Add(FString::Printf(TEXT("unknown argument \"-%s\""), *Switch));
        }
    }

    if (!Tokens.IsEmpty())
    {
        Errors.Add(FString::Printf(
            TEXT("unexpected argument \"%s\": arguments must use the -Key=Value or -Switch form"), *Tokens[0]
        ));
    }

    for (const TPair<FString, FString>& Param : Params)
    {
        const FString Token = FString::Printf(TEXT("%s=%s"), *Param.Key, *Param.Value);
        ParseToken(Token, Errors);
    }

    if (!Errors.IsEmpty())
    {
        Errors.Add(Usage());
        return MakeParseFailure(FString::Join(Errors, TEXT("\n")));
    }

    const FMetaHumanFaceJobResult Validation = Validate(Request);
    if (Validation.Code != EMetaHumanFaceErrorCode::None)
    {
        return Validation;
    }

    return MakeParseSuccess();
}

const FMetaHumanFaceJobRequest& FMetaHumanFaceCommandLine::GetRequest() const {
    return Request;
}

bool FMetaHumanFaceCommandLine::ParseToken(const FString& InToken, TArray<FString>& InOutErrors) {
    const int32 AssignmentIndex = InToken.Find(TEXT("="), ESearchCase::CaseSensitive, ESearchDir::FromStart);

    if (AssignmentIndex <= 0)
    {
        InOutErrors.Add(FString::Printf(TEXT("invalid argument \"%s\""), *InToken));
        return false;
    }

    const FString Key = InToken.Left(AssignmentIndex);
    const FString Value = InToken.Mid(AssignmentIndex + 1);

    if (Key.Equals(TEXT("NoHeadStabilize"), ESearchCase::IgnoreCase) ||
        Key.Equals(TEXT("KeepTongueSolve"), ESearchCase::IgnoreCase))
    {
        InOutErrors.Add(FString::Printf(TEXT("argument \"-%s\" does not take a value"), *Key));
        return false;
    }

    if (Key.Equals(TEXT("FramesDir"), ESearchCase::IgnoreCase))
    {
        Request.FramesDir = Value;
        return true;
    }

    if (Key.Equals(TEXT("OutDir"), ESearchCase::IgnoreCase))
    {
        Request.OutDir = Value;
        return true;
    }

    if (Key.Equals(TEXT("SequencePattern"), ESearchCase::IgnoreCase))
    {
        Request.SequencePattern = Value;
        return true;
    }

    if (Key.Equals(TEXT("PackagePath"), ESearchCase::IgnoreCase))
    {
        Request.AssetPackagePath = Value;
        return true;
    }

    if (Key.Equals(TEXT("FPS"), ESearchCase::IgnoreCase))
    {
        const FMetaHumanFaceJobResult FrameRateResult = ParseFrameRate(Value, Request);
        if (FrameRateResult.Code != EMetaHumanFaceErrorCode::None)
        {
            InOutErrors.Add(FrameRateResult.Detail);
        }

        return FrameRateResult.Code == EMetaHumanFaceErrorCode::None;
    }

    if (Key.Equals(TEXT("Range"), ESearchCase::IgnoreCase))
    {
        const FMetaHumanFaceJobResult FrameRangeResult = ParseFrameRange(Value, Request);
        if (FrameRangeResult.Code != EMetaHumanFaceErrorCode::None)
        {
            InOutErrors.Add(FrameRangeResult.Detail);
        }

        return FrameRangeResult.Code == EMetaHumanFaceErrorCode::None;
    }

    InOutErrors.Add(FString::Printf(TEXT("unknown argument \"-%s\""), *Key));
    return false;
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::Validate(const FMetaHumanFaceJobRequest& InRequest) {
    TArray<FString> Errors;

    if (InRequest.FramesDir.IsEmpty())
    {
        Errors.Add(TEXT("-FramesDir is required"));
    }
    else if (FPaths::IsRelative(InRequest.FramesDir))
    {
        Errors.Add(FString::Printf(TEXT("-FramesDir must be an absolute path: %s"), *InRequest.FramesDir));
    }

    if (InRequest.OutDir.IsEmpty())
    {
        Errors.Add(TEXT("-OutDir is required"));
    }
    else if (FPaths::IsRelative(InRequest.OutDir))
    {
        Errors.Add(FString::Printf(TEXT("-OutDir must be an absolute path: %s"), *InRequest.OutDir));
    }

    if (!InRequest.AssetPackagePath.IsEmpty() && !IsPackagePath(InRequest.AssetPackagePath))
    {
        Errors.Add(FString::Printf(
            TEXT("-PackagePath must be a package path such as /Game/MetaHumanFace/Shot010: %s"),
            *InRequest.AssetPackagePath
        ));
    }

    if (!InRequest.SequencePattern.IsEmpty() && !InRequest.SequencePattern.Contains(TEXT("{frame}")))
    {
        Errors.Add(FString::Printf(
            TEXT("-SequencePattern must contain the {frame} placeholder: %s"), *InRequest.SequencePattern
        ));
    }

    if (InRequest.FpsNum == 0 || InRequest.FpsDen == 0)
    {
        Errors.Add(FString::Printf(TEXT("-FPS is invalid: %u/%u"), InRequest.FpsNum, InRequest.FpsDen));
    }

    if (InRequest.FirstFrame < 0)
    {
        Errors.Add(FString::Printf(TEXT("-Range first frame is negative: %d"), InRequest.FirstFrame));
    }

    if (InRequest.FrameCount < 0)
    {
        Errors.Add(FString::Printf(TEXT("-Range frame count is negative: %d"), InRequest.FrameCount));
    }

    if (Errors.IsEmpty())
    {
        return MakeParseSuccess();
    }

    if (InRequest.FramesDir.IsEmpty() || InRequest.OutDir.IsEmpty())
    {
        Errors.Add(Usage());
    }

    return MakeParseFailure(FString::Join(Errors, TEXT("\n")));
}

FString FMetaHumanFaceCommandLine::Usage() {
    return FString::Join(
        TArray<FString>{
            TEXT("Usage: UnrealEditor-Cmd.exe <Project>.uproject -run=MetaHumanFace -FramesDir=<dir> "
                 "-OutDir=<dir> [options]"),
            TEXT(""),
            TEXT("  -FramesDir=<absolute directory>   required, directory holding the frame sequence"),
            TEXT("  -OutDir=<absolute directory>      required, directory receiving the curve output"),
            TEXT("  -FPS=<numerator>/<denominator>    optional, default 24000/1"),
            TEXT("  -Range=<first>:<count>            optional, default covers the whole sequence"),
            TEXT("  -SequencePattern=<template>       optional, must contain {frame}"),
            TEXT("  -PackagePath=<package path>       optional, where the intermediate assets are created"),
            TEXT("  -NoHeadStabilize                  optional, disables head stabilization"),
            TEXT("  -KeepTongueSolve                  optional, enables tongue solving"),
            TEXT(""),
            TEXT("Examples:"),
            TEXT("  \"-FramesDir=E:\\DoodleFaceTmp\\Shot_010\\frames\" \"-OutDir=E:\\DoodleFaceTmp\\Shot_010\\curves\""),
            TEXT("  -FPS=30000/1001 -Range=0:120")
        },
        TEXT("\n")
    );
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::ParseFrameRate(
    const FString& InValue, FMetaHumanFaceJobRequest& InOut
) {
    const int32 SlashIndex = InValue.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart);

    if (SlashIndex < 0 || InValue.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd) != SlashIndex)
    {
        return MakeParseFailure(FString::Printf(
            TEXT("invalid -FPS value \"%s\": expected <numerator>/<denominator>, for example 24000/1 or 30000/1001"),
            *InValue
        ));
    }

    uint32 Numerator = 0;
    uint32 Denominator = 0;

    if (!ParsePositiveInteger(InValue.Left(SlashIndex), Numerator) ||
        !ParsePositiveInteger(InValue.Mid(SlashIndex + 1), Denominator))
    {
        return MakeParseFailure(FString::Printf(
            TEXT("invalid -FPS value \"%s\": both parts must be positive integers, for example 24000/1 or 30000/1001"),
            *InValue
        ));
    }

    if (Numerator == 0 || Denominator == 0)
    {
        return MakeParseFailure(
            FString::Printf(TEXT("invalid -FPS value \"%s\": neither part can be zero"), *InValue)
        );
    }

    InOut.FpsNum = Numerator;
    InOut.FpsDen = Denominator;
    return MakeParseSuccess();
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::ParseFrameRange(
    const FString& InValue, FMetaHumanFaceJobRequest& InOut
) {
    const int32 ColonIndex = InValue.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart);

    if (ColonIndex < 0 || InValue.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromEnd) != ColonIndex)
    {
        return MakeParseFailure(FString::Printf(
            TEXT("invalid -Range value \"%s\": expected <first>:<count>, for example 0:120"), *InValue
        ));
    }

    int32 FirstFrame = 0;
    int32 FrameCount = 0;

    if (!ParseNonNegativeInteger(InValue.Left(ColonIndex), FirstFrame) ||
        !ParseNonNegativeInteger(InValue.Mid(ColonIndex + 1), FrameCount))
    {
        return MakeParseFailure(FString::Printf(
            TEXT("invalid -Range value \"%s\": both parts must be non negative integers, for example 0:120"), *InValue
        ));
    }

    if (FrameCount == 0)
    {
        return MakeParseFailure(FString::Printf(
            TEXT("invalid -Range value \"%s\": the frame count must be greater than zero, for example 0:120"), *InValue
        ));
    }

    InOut.FirstFrame = FirstFrame;
    InOut.FrameCount = FrameCount;
    return MakeParseSuccess();
}
