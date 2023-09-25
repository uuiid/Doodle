#include "Doodle/DoodleLightningPostTrack.h"

#include "Doodle/DoodleLightningPostSection.h"
// #include "GeometryCacheComponent.h"
// #include "MovieSceneGeometryCacheSection.h"
// #include "Compilation/MovieSceneCompilerRules.h"
#include "Detail/DoodleLightningPostSectionTemplate.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
// #include "MovieSceneGeometryCacheTemplate.h"
// #include "Compilation/IMovieSceneTemplateGenerator.h"
// #include "MovieSceneGeometryCacheTemplate.h"
// #include "MovieScene.h"

UDoodleLightningPostTrack::UDoodleLightningPostTrack(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
#if WITH_EDITORONLY_DATA
  TrackTint = FColor(124, 15, 124, 65);
#endif

  SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);

  EvalOptions.bCanEvaluateNearestSection = true;
  EvalOptions.bEvaluateInPreroll         = true;
}

void UDoodleLightningPostTrack::RemoveAllAnimationData() { AnimationSections.Emplace(); }

bool UDoodleLightningPostTrack::HasSection(const UMovieSceneSection& Section) const {
  return AnimationSections.Contains(&Section);
}

void UDoodleLightningPostTrack::AddSection(UMovieSceneSection& Section) { AnimationSections.Add(&Section); }

void UDoodleLightningPostTrack::RemoveSection(UMovieSceneSection& Section) { AnimationSections.Remove(&Section); }

void UDoodleLightningPostTrack::RemoveSectionAt(int32 SectionIndex) { AnimationSections.RemoveAt(SectionIndex); }

bool UDoodleLightningPostTrack::IsEmpty() const { return AnimationSections.IsEmpty(); }

const TArray<UMovieSceneSection*>& UDoodleLightningPostTrack::GetAllSections() const { return AnimationSections; }

bool UDoodleLightningPostTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const {
  return SectionClass == UDoodleLightningPostTrack::StaticClass();
}

UMovieSceneSection* UDoodleLightningPostTrack::CreateNewSection() {
  return NewObject<UDoodleLightningPostSection>(
      this, UDoodleLightningPostSection::StaticClass(), NAME_None, RF_Transactional
  );
}

bool UDoodleLightningPostTrack::SupportsMultipleRows() const { return false; }

FMovieSceneEvalTemplatePtr UDoodleLightningPostTrack::CreateTemplateForSection(const UMovieSceneSection& InSection
) const {
  return FDoodleLightningPostSectionTemplate{CastChecked<const UDoodleLightningPostSection>(&InSection)};
}

FText UDoodleLightningPostTrack::GetDefaultDisplayName() const {
  return FText::FromString("Doodle Lightning Post Track");
}
