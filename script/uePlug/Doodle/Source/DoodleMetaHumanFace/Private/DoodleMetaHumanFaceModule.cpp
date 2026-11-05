#include "DoodleMetaHumanFaceModule.h"

void FDoodleMetaHumanFaceModule::StartupModule() {
}

void FDoodleMetaHumanFaceModule::ShutdownModule() {
  Endpoint.Reset();
}

IMPLEMENT_MODULE(FDoodleMetaHumanFaceModule, DoodleMetaHumanFace)
