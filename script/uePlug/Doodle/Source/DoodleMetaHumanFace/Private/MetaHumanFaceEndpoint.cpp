#include "MetaHumanFaceEndpoint.h"

FMetaHumanFaceEndpoint::FMetaHumanFaceEndpoint() {
}

FMetaHumanFaceEndpoint::~FMetaHumanFaceEndpoint() {
  Stop();
}

bool FMetaHumanFaceEndpoint::Start(uint32 InPort) {
  return false;
}

void FMetaHumanFaceEndpoint::Stop() {
}

bool FMetaHumanFaceEndpoint::IsRunning() const {
  return false;
}

FString FMetaHumanFaceEndpoint::HandleSubmit(const FJsonObject& InBody) {
  return {};
}

FString FMetaHumanFaceEndpoint::HandleQuery(const FString& InJobId) {
  return {};
}

FString FMetaHumanFaceEndpoint::HandleCancel(const FString& InJobId) {
  return {};
}

bool MetaHumanFaceJobRequestFromJson(const FJsonObject& InJson, FMetaHumanFaceJobRequest& OutRequest, FString& OutError) {
  return false;
}

TSharedRef<FJsonObject> MetaHumanFaceJobResultToJson(const FMetaHumanFaceJobResult& InResult) {
  return MakeShared<FJsonObject>();
}
