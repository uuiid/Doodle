#include "DoodleLightningPostSectionTemplate.h"

#include "Components/DirectionalLightComponent.h"
#include "Doodle/DoodleLightningPost.h"
#include "Doodle/DoodleLightningPostSection.h"

struct FLightingPostEventTrackExecutionToken : IMovieSceneExecutionToken {
  FLightingPostEventTrackExecutionToken(const FDoodleLightningPostSectionTemplate* InSectionTemplate)
      : SectionTemplate(InSectionTemplate) {}

  virtual void Execute(
      const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand,
      FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player
  ) override {
    MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_EventTrack_TokenExecute_Doodle)

    /// @brief 解析上下文
    TArray<UObject*> EventContexts = Player.GetEventContexts();

    for (auto i : Player.FindBoundObjects(Operand)) {
      if (UObject* L_Object = i.Get();
          SectionTemplate->Params->IntensityCurve && L_Object && L_Object->IsA<ADoodleLightingPost>()) {
        ;
        ADoodleLightingPost* L_LightingPost = CastChecked<ADoodleLightingPost>(L_Object);
        const float L_Current_Time          = Context.GetTime().GetFrame().Value;
        const float L_Time_Range_Size       = SectionTemplate->Params->GetRange().Size<FFrameNumber>().Value;
        const auto L_Time                   = L_Current_Time / L_Time_Range_Size;
        const auto L_Curve_Value            = SectionTemplate->Params->IntensityCurve->GetFloatValue(L_Time);
        // UE_LOG(LogTemp, Warning, TEXT("L_Curve_Value: %f"), L_Curve_Value);

        L_LightingPost->GetLightComponent()->SetIntensity(L_Curve_Value * L_LightingPost->IntensityMultiplier);
        // UE_LOG(LogTemp, Warning, TEXT("Intensity Value: %f"), L_Curve_Value * L_LightingPost->IntensityMultiplier);
        L_LightingPost->Settings.ColorSaturation[3] = L_Curve_Value * L_LightingPost->SaturationMultiplier;
        // UE_LOG(LogTemp, Warning, TEXT("Saturation Value: %f"), L_Curve_Value * L_LightingPost->SaturationMultiplier);
        L_LightingPost->Settings.ColorContrast[3]   = L_Curve_Value * L_LightingPost->ContrastMultiplier;
        // UE_LOG(LogTemp, Warning, TEXT("Contrast Value: %f"), L_Curve_Value * L_LightingPost->ContrastMultiplier);
      }
    }
  }

  const FDoodleLightningPostSectionTemplate* SectionTemplate;
};

FDoodleLightningPostSectionTemplate::FDoodleLightningPostSectionTemplate() : Params(nullptr) {}

FDoodleLightningPostSectionTemplate::FDoodleLightningPostSectionTemplate(const UDoodleLightningPostSection* Section)
    : Params(Section) {}

void FDoodleLightningPostSectionTemplate::Evaluate(
    const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context,
    const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens
) const {
  // 模式暂停或者禁音(禁用)状态下不进行评估
  if (Context.GetStatus() == EMovieScenePlayerStatus::Stopped || Context.IsSilent()) {
    return;
  }
  // 评估范围是否向后
  const bool bBackwards = Context.GetDirection() == EPlayDirection::Backwards;
  if (Params) {
    TRange<FFrameNumber> L_Range = Params->GetRange();
    if (L_Range.Contains(Context.GetTime().GetFrame())) {
      ExecutionTokens.Add(FLightingPostEventTrackExecutionToken{this});
    }
  }
}
