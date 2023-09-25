#include "Doodle/DoodleLightningPostSequencerEditor.h"

#include "Doodle/DoodleLightningPost.h"
#include "Doodle/DoodleLightningPostSequencerSection.h"
#include "LevelSequence.h"
#include "ScopedTransaction.h"
#define LOCTEXT_NAMESPACE "DoodleLightningPostSequencerEditor"

FDoodleLightningPostSequencerEditor::FDoodleLightningPostSequencerEditor(TSharedRef<ISequencer> InSequencer)
    : FMovieSceneTrackEditor(InSequencer) {}

FDoodleLightningPostSequencerEditor::~FDoodleLightningPostSequencerEditor() = default;

TSharedRef<ISequencerTrackEditor> FDoodleLightningPostSequencerEditor::CreateTrackEditor(
    TSharedRef<ISequencer> InSequencer
) {
  return MakeShareable(new FDoodleLightningPostSequencerEditor(InSequencer));
}

void FDoodleLightningPostSequencerEditor::BuildObjectBindingTrackMenu(
    FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass
) {
  FMovieSceneTrackEditor::BuildObjectBindingTrackMenu(MenuBuilder, ObjectBindings, ObjectClass);

  if (ObjectClass->IsChildOf(ADoodleLightingPost::StaticClass())) {
    MenuBuilder.AddMenuEntry(
        LOCTEXT("Add_Doodle_Object", "Add Curve"), LOCTEXT("Add_Doodle_Object", "Add Curve"),
        FSlateIcon("Subtitle", "EventIcon"),
        FUIAction{FExecuteAction::CreateLambda([=]() { this->AddNewObjectBindingTrack(ObjectBindings); })}
    );
  }
}

TSharedRef<ISequencerSection> FDoodleLightningPostSequencerEditor::MakeSectionInterface(
    UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding
) {
  check(SupportsType(SectionObject.GetOuter()->GetClass()));
  return MakeShareable(new FDoodleLightningPostSequencerSection{SectionObject, GetSequencer()});
}

bool FDoodleLightningPostSequencerEditor::SupportsSequence(UMovieSceneSequence* InSequence) const {
  return InSequence && InSequence->IsA(ULevelSequence::StaticClass());
}

bool FDoodleLightningPostSequencerEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const {
  // return Type == UMovieSceneDoodleClusterTrack::StaticClass();
  return {};
}

TSharedPtr<SWidget> FDoodleLightningPostSequencerEditor::BuildOutlinerEditWidget(
    const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params
) {
  return FMovieSceneTrackEditor::BuildOutlinerEditWidget(ObjectBinding, Track, Params);
  // return {};
}
void FDoodleLightningPostSequencerEditor::AddNewObjectBindingTrack(TArray<FGuid> InObjectBindings) const {
  TSharedPtr<ISequencer> SequencerPin = GetSequencer();

  TSet<ADoodleLightingPost*> L_Sks;

  for (auto&& I_ObjBind : InObjectBindings) {
    TArrayView<TWeakObjectPtr<UObject>> L_UObjs = SequencerPin->FindObjectsInCurrentSequence(I_ObjBind);

    for (auto&& I : L_UObjs) {
      if (I.IsValid() && I.Get()->IsA<ADoodleLightingPost>()) {
        L_Sks.Add(CastChecked<ADoodleLightingPost>(I.Get()));
      }
    }
  }

  for (auto&& I_SkeletalComponent : L_Sks) {
    // UAnimInstance* AnimInstance = I_SkeletalComponent->GetAnimInstance();
    // SequencerPin->GetHandleToObject(AnimInstance ? AnimInstance : NewObject<UAnimInstance>(I_SkeletalComponent));
  }
}
#undef LOCTEXT_NAMESPACE