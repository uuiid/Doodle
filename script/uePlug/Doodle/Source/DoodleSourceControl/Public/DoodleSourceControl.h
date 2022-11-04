#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Doodle/SourceControlProvider.h"

// #include "DoodleSourceControl.generated.h"

/// 这个类是模块的开始, 注册我们自己的源码控制

class FDoodleSourceControlModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    FDoodleSourceControlProvider SourceControlProvider;
};
