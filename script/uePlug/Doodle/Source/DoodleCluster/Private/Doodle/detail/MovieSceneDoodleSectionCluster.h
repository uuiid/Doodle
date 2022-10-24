#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "MovieSceneDoodleSectionCluster.generated.h"

class UDoodleClusterSection;

USTRUCT()
struct FMovieSceneDoodleSectionClusterTemplate : public FMovieSceneEvalTemplate {
  GENERATED_BODY()
  FMovieSceneDoodleSectionClusterTemplate();
  FMovieSceneDoodleSectionClusterTemplate(const UDoodleClusterSection &Section);

  virtual UScriptStruct &GetScriptStructImpl() const override { return *StaticStruct(); }
  virtual void Evaluate(
      const FMovieSceneEvaluationOperand &Operand, const FMovieSceneContext &Context,
      const FPersistentEvaluationData &PersistentData, FMovieSceneExecutionTokens &ExecutionTokens
  ) const override;

  void Setup(FPersistentEvaluationData &InPersistentData, IMovieScenePlayer &InPlayer) const override;

  UPROPERTY()
  const UDoodleClusterSection *Params;
};