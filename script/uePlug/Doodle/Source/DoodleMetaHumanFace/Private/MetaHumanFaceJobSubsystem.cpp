#include "MetaHumanFaceJobSubsystem.h"

void UMetaHumanFaceJobSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
  Super::Initialize(Collection);
}

void UMetaHumanFaceJobSubsystem::Deinitialize() {
  Super::Deinitialize();
}

FString UMetaHumanFaceJobSubsystem::Submit(const FMetaHumanFaceJobRequest& InRequest) {
  return {};
}

FMetaHumanFaceJobResult UMetaHumanFaceJobSubsystem::Query(const FString& InJobId) const {
  return {};
}

FMetaHumanFaceJobResult UMetaHumanFaceJobSubsystem::CancelJob(const FString& InJobId) {
  return {};
}

bool UMetaHumanFaceJobSubsystem::IsBusy() const {
  return false;
}

void UMetaHumanFaceJobSubsystem::PumpQueue() {
}

void UMetaHumanFaceJobSubsystem::ExecuteNext() {
}
