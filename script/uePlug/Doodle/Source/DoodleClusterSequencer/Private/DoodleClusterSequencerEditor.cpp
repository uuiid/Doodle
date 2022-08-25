#include "DoodleClusterSequencerEditor.h"
#include "LevelSequence.h"
#include "MovieSceneDoodleClusterTrack.h"

FDoodleClusterTrackEditor::FDoodleClusterTrackEditor(
    TSharedRef<ISequencer> InSequencer
)
    : FMovieSceneTrackEditor(InSequencer) {
}

FDoodleClusterTrackEditor::~FDoodleClusterTrackEditor() {}

TSharedRef<ISequencerTrackEditor>
FDoodleClusterTrackEditor::CreateTrackEditor(
    TSharedRef<ISequencer> OwningSequencer
) {
  return MakeShareable(new FDoodleClusterTrackEditor(OwningSequencer));
}

void FDoodleClusterTrackEditor::BuildObjectBindingTrackMenu(
    FMenuBuilder &MenuBuilder,
    const TArray<FGuid> &ObjectBindings,
    const UClass *ObjectClass
) {
}
TSharedPtr<SWidget> FDoodleClusterTrackEditor::BuildOutlinerEditWidget(
    const FGuid &ObjectBinding,
    UMovieSceneTrack *Track,
    const FBuildEditWidgetParams &Params
) {
  return {};
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

FDoodleClusterSection::FDoodleClusterSection(UMovieSceneSection &InSection, TWeakPtr<ISequencer> InSequencer)
    : ISequencerSection() {
}
