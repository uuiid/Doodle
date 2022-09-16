#include "DoodleClusterSequencerEditor.h"
#include "LevelSequence.h"
#include "DoodleAnimInstance.h"
#include "Doodle/MovieSceneDoodleClusterTrack.h"
#include "ScopedTransaction.h"

#include "Doodle/DoodleClusterSection.h"

#include "Doodle/MovieSceneDoodleClusterTrack.h"
#include "Doodle/DoodleClusterSectionRuntime.h"

#define LOCTEXT_NAMESPACE "DoodleClusterSequencer"

FDoodleClusterTrackEditor::FDoodleClusterTrackEditor(
    TSharedRef<ISequencer> InSequencer
)
    : FMovieSceneTrackEditor(InSequencer) {
}

FDoodleClusterTrackEditor::~FDoodleClusterTrackEditor() = default;

TSharedRef<ISequencerTrackEditor>
FDoodleClusterTrackEditor::CreateTrackEditor(
    TSharedRef<ISequencer> OwningSequencer
) {
  return MakeShareable(new FDoodleClusterTrackEditor(OwningSequencer));
}

void FDoodleClusterTrackEditor::AddNewObjectBindingTrack(
    TArray<FGuid> InObjectBindings
) const {
  UMovieScene *MovieScene = GetFocusedMovieScene();
  if (MovieScene == nullptr || MovieScene->IsReadOnly()) {
    return;
  }

  UClass *ClassToAdd = UMovieSceneDoodleClusterTrack::StaticClass();  // LoadClassFromAssetData(AssetData);

  MovieScene->Modify();

  for (const FGuid &ObjectBindingID : InObjectBindings) {
    UMovieSceneDoodleClusterTrack *CustomTrack = CastChecked<UMovieSceneDoodleClusterTrack>(
        MovieScene->AddTrack(ClassToAdd, ObjectBindingID)
    );
    {
      TSharedPtr<ISequencer> SequencerPin = GetSequencer();
      UClass *Class                       = UDoodleClusterSection::StaticClass();

      if (Class && SequencerPin) {
        FScopedTransaction L_Transaction(FText::Format(LOCTEXT("AddCustomSection_Transaction", "Add New Section From Class %s"), FText::FromName(Class->GetFName())));
        // UMovieScene3DTransformSection* NewSection = NewObject<UMovieScene3DTransformSection>(Track, UMovieScene3DTransformSection::StaticClass(), NAME_None, RF_Transactional);
        UDoodleClusterSection *NewAttachSection = NewObject<UDoodleClusterSection>(CustomTrack, Class, NAME_None, RF_Transactional);

        const FQualifiedFrameTime CurrentTime   = SequencerPin->GetLocalTime();

        const FFrameNumber Duration             = (5.f * CurrentTime.Rate).FrameNumber;
        NewAttachSection->SetRange(TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, CurrentTime.Time.FrameNumber + Duration));
        NewAttachSection->InitialPlacement(CustomTrack->GetAllSections(), CurrentTime.Time.FrameNumber, Duration.Value, CustomTrack->SupportsMultipleRows());

        CustomTrack->AddSection(*NewAttachSection);
        // CustomTrack->AddSection(*NewSection);

        SequencerPin->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
      }
    }

    if (GetSequencer().IsValid()) {
      GetSequencer()->OnAddTrack(CustomTrack, FGuid());
    }
  }
}

void FDoodleClusterTrackEditor::BuildObjectBindingTrackMenu(
    FMenuBuilder &MenuBuilder,
    const TArray<FGuid> &ObjectBindings,
    const UClass *ObjectClass
) {
  FMovieSceneTrackEditor::BuildObjectBindingTrackMenu(
      MenuBuilder,
      ObjectBindings,
      ObjectClass
  );

  if (ObjectClass->IsChildOf(UDoodleAnimInstance::StaticClass())) {
    MenuBuilder.AddMenuEntry(
        LOCTEXT("Add_Doodle_LookAtObject", "Add Foodle LookAtObject"),
        LOCTEXT("Add_Doodle_LookAtObject", "Add Foodle LookAtObject"),
        FSlateIcon("Subtitle", "EventIcon"),
        FUIAction{
            FExecuteAction::CreateLambda(
                [=]() {
                  this->AddNewObjectBindingTrack(ObjectBindings);
                }
            )}
    );
  }

  if (ObjectClass->IsChildOf(USkeletalMeshComponent::StaticClass())) {
    MenuBuilder.AddMenuEntry(
        LOCTEXT("Add_Anim", "Add Anim"),
        LOCTEXT("Add_Anim", "Add Anim"),
        FSlateIcon("Subtitle", "EventIcon"),
        FUIAction{
            FExecuteAction::CreateLambda(
                [=]() {
                  this->AddAllBindAnimationInstance(ObjectBindings);
                }
            )}
    );
  }
}
TSharedPtr<SWidget> FDoodleClusterTrackEditor::BuildOutlinerEditWidget(
    const FGuid &ObjectBinding,
    UMovieSceneTrack *Track,
    const FBuildEditWidgetParams &Params
) {
  return FMovieSceneTrackEditor::BuildOutlinerEditWidget(
      ObjectBinding,
      Track,
      Params
  );
  // return {};
}

TSharedRef<ISequencerSection> FDoodleClusterTrackEditor::MakeSectionInterface(
    UMovieSceneSection &SectionObject, UMovieSceneTrack &Track, FGuid ObjectBinding
) {
  check(SupportsType(SectionObject.GetOuter()->GetClass()));

  return MakeShareable(new FDoodleClusterSection{SectionObject, GetSequencer()});
}
bool FDoodleClusterTrackEditor::SupportsSequence(UMovieSceneSequence *InSequence) const {
  return InSequence && InSequence->IsA(ULevelSequence::StaticClass());
}
bool FDoodleClusterTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const {
  return Type == UMovieSceneDoodleClusterTrack::StaticClass();
}

void FDoodleClusterTrackEditor::AddAllBindAnimationInstance(const TArray<FGuid> &ObjectBindings) const {
  TSharedPtr<ISequencer> SequencerPin = GetSequencer();

  TArray<USkeletalMeshComponent *> L_Sks;

  for (auto &&I_ObjBind : ObjectBindings) {
    TArrayView<TWeakObjectPtr<UObject>> L_UObjs =
        SequencerPin->FindObjectsInCurrentSequence(I_ObjBind);

    for (auto &&I : L_UObjs) {
      if (I.IsValid()) {
        if (USkeletalMeshComponent *L_Sk = Cast<USkeletalMeshComponent>(I.Get())) {
          if (!L_Sks.Contains(L_Sk))
            L_Sks.Add(L_Sk);
        }
      }
    }
  }

  for (auto &&I_SkeletalComponent : L_Sks) {
    UAnimInstance *AnimInstance = I_SkeletalComponent->GetAnimInstance();
    SequencerPin->GetHandleToObject(AnimInstance ? AnimInstance : NewObject<UAnimInstance>(I_SkeletalComponent));
  }
}

#undef LOCTEXT_NAMESPACE
