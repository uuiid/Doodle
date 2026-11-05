#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "MetaHumanFaceTypes.h"

#include "MetaHumanFaceCommandlet.generated.h"

class FMetaHumanFaceSolver;

UCLASS()
class UMetaHumanFaceCommandlet : public UCommandlet
{
    GENERATED_BODY()

  public:
    UMetaHumanFaceCommandlet();

    virtual int32 Main(const FString& Params) override;

  private:
    FMetaHumanFaceJobResult Run(const FMetaHumanFaceJobRequest& InRequest);
    void RememberElapsed(FMetaHumanFaceJobResult& InResult) const;
    void WriteLog(FMetaHumanFaceJobResult& InResult) const;
    static int32 ToExitCode(const FMetaHumanFaceJobResult& InResult);
    static void Report(const FMetaHumanFaceJobResult& InResult);

    double StartSeconds = 0.0;
    FString RunOutDir;
    TUniquePtr<FMetaHumanFaceSolver> Solver;
};
