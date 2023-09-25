#include "Doodle/DoodleLightningPostSequencerEditor.h"

#include "Doodle/DoodleLightningPost.h"
#include "Doodle/DoodleLightningPostSection.h"
#include "Doodle/DoodleLightningPostSequencerSection.h"
#include "Doodle/DoodleLightningPostTrack.h"
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
  UMovieScene* MovieScene = GetFocusedMovieScene();
  if (MovieScene == nullptr || MovieScene->IsReadOnly()) {
    return;
  }

  UClass* ClassToAdd = ADoodleLightingPost::StaticClass();  // LoadClassFromAssetData(AssetData);

  MovieScene->Modify();

  for (const FGuid& ObjectBindingID : InObjectBindings) {
    UDoodleLightningPostTrack* CustomTrack =
        CastChecked<UDoodleLightningPostTrack>(MovieScene->AddTrack(ClassToAdd, ObjectBindingID));
    TSharedPtr<ISequencer> SequencerPin = GetSequencer();
    UClass* Class                       = UDoodleLightningPostSection::StaticClass();

    if (Class && SequencerPin) {
      FScopedTransaction L_Transaction(FText::Format(
          LOCTEXT("AddCustomSection_Transaction", "Add New Section From Class %s"), FText::FromName(Class->GetFName())
      ));

      const FQualifiedFrameTime CurrentTime         = SequencerPin->GetLocalTime();

      const FFrameNumber Duration           = (5.f * CurrentTime.Rate).FrameNumber;

      UDoodleLightningPostSection* NewAttachSection =
          CastChecked<UDoodleLightningPostSection>(CustomTrack->CreateNewSection());
      NewAttachSection->SetRange(
          TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, CurrentTime.Time.FrameNumber + Duration)
      );
      NewAttachSection->InitialPlacement(
          CustomTrack->GetAllSections(), CurrentTime.Time.FrameNumber, Duration.Value,
          CustomTrack->SupportsMultipleRows()
      );
      CustomTrack->AddSection(*NewAttachSection);

      SequencerPin->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
      SequencerPin->OnAddTrack(CustomTrack, FGuid());
    }
  }
}
#undef LOCTEXT_NAMESPACE
