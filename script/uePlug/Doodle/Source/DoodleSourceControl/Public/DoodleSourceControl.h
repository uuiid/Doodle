#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Doodle/SourceControlProvider.h"

// #include "DoodleSourceControl.generated.h"

class FDoodleSourceControlModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    FDoodleSourceControlProvider SourceControlProvider;
};
