#include "DoodleSourceControl.h"

#include "Doodle/DoodleSourceControlOperations.h"

#define LOCTEXT_NAMESPACE "DoodleSourceControl"

template <typename Type>
static TSharedRef<IDoodleSourceControlWorker, ESPMode::ThreadSafe> CreateWorker() {
  return MakeShareable(new Type());
}

void FDoodleSourceControlModule::StartupModule() {
  // 在我们这里添加自定义放置类
  SourceControlProvider.WorkersMap.Emplace(
      "Connect",
      FGetDoodleSourceControlWorker::CreateStatic(&CreateWorker<FDoodleConnectWorker>)
  );

  IModularFeatures::Get().RegisterModularFeature("SourceControl", &SourceControlProvider);
}

void FDoodleSourceControlModule::ShutdownModule() {
  SourceControlProvider.Close();
  // 我们的卸载函数
  IModularFeatures::Get().UnregisterModularFeature("SourceControl", &SourceControlProvider);
}

FDoodleSourceControlProvider& FDoodleSourceControlModule::GetProvider() {
  return SourceControlProvider;
}

IMPLEMENT_MODULE(FDoodleSourceControlModule, DoodleSourceControl)

#undef LOCTEXT_NAMESPACE