#pragma once

#include "MetaHumanFaceTypes.h"

class FMetaHumanFaceCommandLine
{
  public:
    FMetaHumanFaceCommandLine();
    ~FMetaHumanFaceCommandLine();

    FMetaHumanFaceCommandLine(const FMetaHumanFaceCommandLine&) = delete;
    FMetaHumanFaceCommandLine(FMetaHumanFaceCommandLine&&) = delete;
    FMetaHumanFaceCommandLine& operator=(const FMetaHumanFaceCommandLine&) = delete;
    FMetaHumanFaceCommandLine& operator=(FMetaHumanFaceCommandLine&&) = delete;

    FMetaHumanFaceJobResult Parse(const TArray<FString>& InTokens) const;

    static FMetaHumanFaceJobResult Validate(const FMetaHumanFaceJobRequest& InRequest);
    static FString Usage();

  private:
    static FMetaHumanFaceJobResult ParseFrameRate(const FString& InValue, FMetaHumanFaceJobRequest& InOut);
    static FMetaHumanFaceJobResult ParseFrameRange(const FString& InValue, FMetaHumanFaceJobRequest& InOut);
};
