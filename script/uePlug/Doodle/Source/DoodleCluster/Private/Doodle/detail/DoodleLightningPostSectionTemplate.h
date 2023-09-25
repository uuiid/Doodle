#pragma once

#include "CoreMinimal.h"
#include "DoodleLightningPostSectionTemplate.generated.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "UObject/ObjectMacros.h"

class UDoodleLightningPostSection;

USTRUCT()
struct FDoodleLightningPostSectionTemplate : public FMovieSceneEvalTemplate {
  GENERATED_BODY()
  FDoodleLightningPostSectionTemplate();
  FDoodleLightningPostSectionTemplate(const UDoodleLightningPostSection* Section);

  virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
  virtual void Evaluate(
      const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context,
      const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens
  ) const override;

  //   void Setup(FPersistentEvaluationData &InPersistentData, IMovieScenePlayer &InPlayer) const override;

  UPROPERTY()
  const UDoodleLightningPostSection* Params;
};
