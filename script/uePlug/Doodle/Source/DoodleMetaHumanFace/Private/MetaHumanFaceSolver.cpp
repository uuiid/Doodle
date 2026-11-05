#include "MetaHumanFaceSolver.h"

FMetaHumanFaceSolver::FMetaHumanFaceSolver() {
}

FMetaHumanFaceSolver::~FMetaHumanFaceSolver() {
}

FMetaHumanFaceJobResult FMetaHumanFaceSolver::Preflight() const {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceSolver::CreatePerformance(const FMetaHumanFaceJobRequest& InRequest) {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceSolver::Process(UFootageCaptureData* InCaptureData) {
  return {};
}

FMetaHumanFaceJobResult FMetaHumanFaceSolver::CollectCurveSet(CurveSetResult& OutCurveSet) const {
  return {};
}

bool FMetaHumanFaceSolver::IsProcessing() const {
  return false;
}

void FMetaHumanFaceSolver::Cancel() {
}

TArray<FString> FMetaHumanFaceSolver::ExpectedHeadCurves() {
  return {};
}
