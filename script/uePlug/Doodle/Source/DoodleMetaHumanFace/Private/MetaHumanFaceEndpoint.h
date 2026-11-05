#pragma once

#include "MetaHumanFaceTypes.h"
#include "Templates/Function.h"
#include "Templates/UniquePtr.h"

class FJsonObject;

class FMetaHumanFaceEndpoint
{
  public:
    FMetaHumanFaceEndpoint();
    ~FMetaHumanFaceEndpoint();

    FMetaHumanFaceEndpoint(const FMetaHumanFaceEndpoint&) = delete;
    FMetaHumanFaceEndpoint(FMetaHumanFaceEndpoint&&) = delete;
    FMetaHumanFaceEndpoint& operator=(const FMetaHumanFaceEndpoint&) = delete;
    FMetaHumanFaceEndpoint& operator=(FMetaHumanFaceEndpoint&&) = delete;

    bool Start(uint32 InPort);
    void Stop();
    bool IsRunning() const;

  private:
    TFunction<FMetaHumanFaceJobResult(const FMetaHumanFaceJobRequest&)> OnSubmit;
    TFunction<FMetaHumanFaceJobResult(const FString&)> OnQuery;
    TFunction<FMetaHumanFaceJobResult(const FString&)> OnCancel;

    FString HandleSubmit(const FJsonObject& InBody);
    FString HandleQuery(const FString& InJobId);
    FString HandleCancel(const FString& InJobId);
};

bool MetaHumanFaceJobRequestFromJson(const FJsonObject& InJson, FMetaHumanFaceJobRequest& OutRequest, FString& OutError);
TSharedRef<FJsonObject> MetaHumanFaceJobResultToJson(const FMetaHumanFaceJobResult& InResult);
