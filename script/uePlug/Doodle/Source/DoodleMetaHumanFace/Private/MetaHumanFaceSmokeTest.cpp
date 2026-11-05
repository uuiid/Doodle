#include "MetaHumanFaceSmokeTest.h"

bool UMetaHumanFaceSmokeTest::RunPreflight() {
  return false;
}

bool UMetaHumanFaceSmokeTest::RunMonocular(const FString& InFramesDir, const FString& InOutDir) {
  return false;
}

bool UMetaHumanFaceSmokeTest::CaptureDataRoundTrip(const FString& InFramesDir) {
  return false;
}

void UMetaHumanFaceSmokeTest::DumpCurveNames(const FString& InOutDir) {
}

UFootageCaptureData* UMetaHumanFaceSmokeTest::LoadOrBuildCaptureData(const FString& InFramesDir) {
  return nullptr;
}
