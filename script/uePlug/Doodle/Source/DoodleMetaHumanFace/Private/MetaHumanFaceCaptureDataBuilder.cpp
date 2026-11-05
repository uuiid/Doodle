#include "MetaHumanFaceCaptureDataBuilder.h"

FMetaHumanFaceCaptureDataBuilder::FMetaHumanFaceCaptureDataBuilder() {
}

FMetaHumanFaceCaptureDataBuilder::~FMetaHumanFaceCaptureDataBuilder() {
}

FMetaHumanFaceJobResult FMetaHumanFaceCaptureDataBuilder::Validate(const FMetaHumanFaceJobRequest& InRequest) const {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceCaptureDataBuilder::Build(
    const FMetaHumanFaceJobRequest& InRequest, OutCaptureDataResult& OutCaptureData
) const {
  return {};
}

void FMetaHumanFaceCaptureDataBuilder::Release(const FMetaHumanFaceJobRequest& InRequest) const {
}
