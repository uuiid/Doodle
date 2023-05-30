#include "MovieSceneDoodleSectionCluster.h"

#include "Doodle/DoodleClusterSectionRuntime.h"
#include "DoodleAiCrowd.h"
#include "DoodleAnimInstance.h"
#include "Evaluation/MovieSceneEvaluationTemplateInstance.h"
#include "MovieSceneExecutionToken.h"
#include "MovieSceneSequence.h"
#include "MovieSceneTrack.h"
#include "Components/SkeletalMeshComponent.h"

DECLARE_CYCLE_STAT(
    TEXT("Doodle Event Track Token Execute"), MovieSceneEval_EventTrack_TokenExecute_Doodle, STATGROUP_MovieSceneEval
);

struct FEventTrackExecutionTokenDOodle : IMovieSceneExecutionToken {
  FEventTrackExecutionTokenDOodle(const FMovieSceneDoodleSectionClusterTemplate *InSectionTemplate)
      : SectionTemplate(InSectionTemplate) {}

  virtual void Execute(
      const FMovieSceneContext &Context, const FMovieSceneEvaluationOperand &Operand,
      FPersistentEvaluationData &PersistentData,
      IMovieScenePlayer &Player
  ) override {
    MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_EventTrack_TokenExecute_Doodle)

    /// @brief 解析上下文
    TArray<UObject *> EventContexts = Player.GetEventContexts();

    for (auto i : Player.FindBoundObjects(Operand)) {
      if (UObject *L_Object = i.Get()) {
        // if (APawn *L_Pawn = Cast<APawn>(L_Object))
        // {
        //     TArray<USkeletalMeshComponent *> L_Component{};
        //     L_Pawn->GetComponents<USkeletalMeshComponent>(L_Component);
        //     for (auto j : L_Component)
        //     {
        //         // j->Get
        //     }
        // }
        AActor *L_Lock_Object{};
        for (auto j : Player.FindBoundObjects(
                 SectionTemplate->Params->DoodleLockAtObject.GetGuid(),
                 SectionTemplate->Params->DoodleLockAtObject.GetRelativeSequenceID()
             )) {
          if (UObject *L_Object2 = j.Get()) {
            if (AActor *L_Actor = Cast<AActor>(L_Object2)) L_Lock_Object = L_Actor;
            break;
          }
        }
        if (ADoodleAiCrowd *L_Ai = Cast<ADoodleAiCrowd>(L_Object)) {
          USkeletalMeshComponent *L_SK = L_Ai->GetMesh();
          if (L_SK) {
            UDoodleAnimInstance *L_Anim = Cast<UDoodleAnimInstance>(L_SK->GetAnimInstance());
            /// 计算方向速度
            if (L_Anim && L_Lock_Object) {
              L_Anim->DoodleLookAtObject(L_Lock_Object);
              break;
            }
          }
        }
      }
    }
  }
  const FMovieSceneDoodleSectionClusterTemplate *SectionTemplate;
};

FMovieSceneDoodleSectionClusterTemplate::FMovieSceneDoodleSectionClusterTemplate() : Params() {
  // EnableOverrides(RequiresSetupFlag);
}

FMovieSceneDoodleSectionClusterTemplate::FMovieSceneDoodleSectionClusterTemplate(const UDoodleClusterSection &Section)
    : Params(&Section) {
  // EnableOverrides(RequiresSetupFlag);
}

void FMovieSceneDoodleSectionClusterTemplate::Evaluate(
    const FMovieSceneEvaluationOperand &Operand, const FMovieSceneContext &Context,
    const FPersistentEvaluationData &PersistentData,
    FMovieSceneExecutionTokens &ExecutionTokens
) const {
  // 模式暂停或者禁音(禁用)状态下不进行评估
  if (Context.GetStatus() == EMovieScenePlayerStatus::Stopped ||
      Context.IsSilent()) {
    return;
  }
  // 评估范围是否向后
  const bool bBackwards = Context.GetDirection() == EPlayDirection::Backwards;

  if (Params) {
    TRange<FFrameNumber> L_Range = Params->GetRange();
    if (L_Range.Contains(Context.GetTime().GetFrame())) {
      ExecutionTokens.Add(FEventTrackExecutionTokenDOodle{this});
    }

    // for (auto i = L_Range.GetLowerBoundValue();
    //      i < L_Range.GetUpperBoundValue();
    //      ++i)
    // {
    // }
  }
}

// void FMovieSceneDoodleSectionClusterTemplate::Setup(
//     FPersistentEvaluationData &InPersistentData, IMovieScenePlayer &InPlayer
// ) const {
// UMovieSceneTrack *L_Movie = Cast<UMovieSceneTrack>(Params->GetOuter());
// for (auto &&i : InPlayer.FindBoundObjects(L_Movie->FindObjectBindingGuid(), MovieSceneSequenceID::Root)) {
//   if (i.IsValid()) {
//     auto L_Obj    = i.Get();
//     APawn *L_Anim = Cast<APawn>(L_Obj->GetOuter()->GetOuter());
//     if (L_Anim && L_Movie && Params->MoveTo) {
//       L_Anim->SpawnDefaultController();
//       L_Anim->UpdateNavAgent();
//       // L_Anim->PostInitializeComponents();
//       // UE_LOG(LogTemp, Log, TEXT("ok %s"), *L_Anim->GetName());
//     }
//   }
// };
// }