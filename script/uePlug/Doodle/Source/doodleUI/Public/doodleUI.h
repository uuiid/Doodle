#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// #include "DoodleUI.generated.h"

class FdoodleUIModule : public IModuleInterface {
 public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};
