#include "MetaHumanFaceCommandLine.h"

FMetaHumanFaceCommandLine::FMetaHumanFaceCommandLine() {
}

FMetaHumanFaceCommandLine::~FMetaHumanFaceCommandLine() {
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::Parse(const TArray<FString>& InTokens) const {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::Validate(const FMetaHumanFaceJobRequest& InRequest) {
  return {};
}

FString FMetaHumanFaceCommandLine::Usage() {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::ParseFrameRate(
    const FString& InValue, FMetaHumanFaceJobRequest& InOut
) {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceCommandLine::ParseFrameRange(
    const FString& InValue, FMetaHumanFaceJobRequest& InOut
) {
  return {};
}
