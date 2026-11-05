#pragma once

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
    FMetaHumanFaceJobResult Build(
        const FMetaHumanFaceJobRequest& InRequest, OutCaptureDataResult& OutCaptureData
    ) const;
    void Release(const FMetaHumanFaceJobRequest& InRequest) const;
};
