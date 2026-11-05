#pragma once

#include "MetaHumanFaceTypes.h"
#include "UObject/Object.h"

#include "MetaHumanFaceJobSubsystem.generated.h"

UCLASS()
class DOODLEMETAHUMANFACE_API UMetaHumanFaceJobSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

  public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    FString Submit(const FMetaHumanFaceJobRequest& InRequest);
    FMetaHumanFaceJobResult Query(const FString& InJobId) const;
    FMetaHumanFaceJobResult CancelJob(const FString& InJobId);
    bool IsBusy() const;

  private:
    void PumpQueue();
    void ExecuteNext();
};
