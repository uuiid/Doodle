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

    FMetaHumanFaceJobResult Parse(const FString& InCommandLine);
    const FMetaHumanFaceJobRequest& GetRequest() const;

    static FMetaHumanFaceJobResult Validate(const FMetaHumanFaceJobRequest& InRequest);
    static FString Usage();

  private:
    FMetaHumanFaceJobRequest Request;

    bool ParseToken(const FString& InToken, TArray<FString>& InOutErrors);
    static FMetaHumanFaceJobResult ParseFrameRate(const FString& InValue, FMetaHumanFaceJobRequest& InOut);
    static FMetaHumanFaceJobResult ParseFrameRange(const FString& InValue, FMetaHumanFaceJobRequest& InOut);
};
