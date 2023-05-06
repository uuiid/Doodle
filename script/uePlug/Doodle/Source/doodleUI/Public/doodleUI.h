#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// #include "DoodleUI.generated.h"
struct FTemplateMapInfo;
class FdoodleUIModule : public IModuleInterface {
  TArray<FTemplateMapInfo> Map_Lists;
 public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};
