#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMetaHumanFaceEndpoint;

class DOODLEMETAHUMANFACE_API FDoodleMetaHumanFaceModule : public IModuleInterface
{
  public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

  private:
    TUniquePtr<FMetaHumanFaceEndpoint> Endpoint;
};
