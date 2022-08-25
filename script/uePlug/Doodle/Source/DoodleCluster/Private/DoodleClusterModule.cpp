

#include "DoodleClusterModule.h"

IMPLEMENT_MODULE(FDoodleClusterModule, DoodleCluster)

void FDoodleClusterModule::StartupModule() {
#if WITH_EDITOR
  // FGeometryCacheSequencerModule& Module = FModuleManager::LoadModuleChecked<FGeometryCacheSequencerModule>(TEXT("GeometryCacheSequencer"));
#endif
}

void FDoodleClusterModule::ShutdownModule() {
}
