#include "DoodleSourceControl.h"

#define LOCTEXT_NAMESPACE "FDoodleSourceControlModule"

void FDoodleSourceControlModule::StartupModule()
{
    // 在我们这里添加自定义放置类

    IModularFeatures::Get().RegisterModularFeature("SourceControl", &SourceControlProvider);
}

void FDoodleSourceControlModule::ShutdownModule()
{
    SourceControlProvider.Close();
    // 我们的卸载函数
    IModularFeatures::Get().UnregisterModularFeature("SourceControl", &SourceControlProvider);
}

IMPLEMENT_MODULE(FDoodleSourceControlModule, DoodleSourceControl)

#undef LOCTEXT_NAMESPACE