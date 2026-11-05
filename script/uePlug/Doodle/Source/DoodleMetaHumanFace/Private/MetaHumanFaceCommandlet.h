#pragma once

#include "Commandlets/Commandlet.h"
#include "MetaHumanFaceTypes.h"

#include "MetaHumanFaceCommandlet.generated.h"

UCLASS()
class UMetaHumanFaceCommandlet : public UCommandlet
{
    GENERATED_BODY()

  public:
    UMetaHumanFaceCommandlet();

    virtual int32 Main(const FString& Params) override;

  private:
    FMetaHumanFaceJobResult Run(const FMetaHumanFaceJobRequest& InRequest);
};
