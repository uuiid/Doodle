// Copyright Epic Games, Inc. All Rights Reserved.

#include "Doodle/MovieSceneDoodleClusterTrack.h"
// #include "GeometryCacheComponent.h"
// #include "MovieSceneGeometryCacheSection.h"
// #include "Compilation/MovieSceneCompilerRules.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "detail/MovieSceneDoodleSectionCluster.h"
#include "Doodle/DoodleClusterSectionRuntime.h"
// #include "MovieSceneGeometryCacheTemplate.h"
// #include "Compilation/IMovieSceneTemplateGenerator.h"
// #include "MovieSceneGeometryCacheTemplate.h"
// #include "MovieScene.h"

#define LOCTEXT_NAMESPACE "MovieSceneDoodleClusterTrack"

/* UMovieSceneDoodleClusterTrack structors
 *****************************************************************************/

UMovieSceneDoodleClusterTrack::UMovieSceneDoodleClusterTrack(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
  TrackTint = FColor(124, 15, 124, 65);
#endif

  SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);

  EvalOptions.bCanEvaluateNearestSection = true;
  EvalOptions.bEvaluateInPreroll = true;
}

void UMovieSceneDoodleClusterTrack::RemoveAllAnimationData()
{
  AnimationSections.Empty();
}

bool UMovieSceneDoodleClusterTrack::HasSection(const UMovieSceneSection &Section) const
{
  return AnimationSections.Contains(&Section);
}

void UMovieSceneDoodleClusterTrack::AddSection(UMovieSceneSection &Section)
{
  AnimationSections.Add(&Section);
}

void UMovieSceneDoodleClusterTrack::RemoveSection(UMovieSceneSection &Section)
{
  AnimationSections.Remove(&Section);
}

void UMovieSceneDoodleClusterTrack::RemoveSectionAt(int32 SectionIndex)
{
  AnimationSections.RemoveAt(SectionIndex);
}

bool UMovieSceneDoodleClusterTrack::IsEmpty() const
{
  return AnimationSections.Num() == 0;
}

const TArray<UMovieSceneSection *> &UMovieSceneDoodleClusterTrack::GetAllSections() const
{
  return AnimationSections;
}

bool UMovieSceneDoodleClusterTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
  return SectionClass == UDoodleClusterSection::StaticClass();
}

UMovieSceneSection *UMovieSceneDoodleClusterTrack::CreateNewSection()
{
  return NewObject<UMovieSceneSection>(this,
                                       UDoodleClusterSection::StaticClass(),
                                       NAME_None,
                                       RF_Transactional);
}

bool UMovieSceneDoodleClusterTrack::SupportsMultipleRows() const
{
  return false;
}

FMovieSceneEvalTemplatePtr UMovieSceneDoodleClusterTrack::CreateTemplateForSection(const UMovieSceneSection &InSection) const
{
  return FMovieSceneDoodleSectionClusterTemplate{
      *CastChecked<const UDoodleClusterSection>(&InSection)};
}

#if WITH_EDITORONLY_DATA

FText UMovieSceneDoodleClusterTrack::GetDefaultDisplayName() const
{
  return LOCTEXT("TrackName", "Doodle Cluster");
}

#endif

#undef LOCTEXT_NAMESPACE
