// Copyright Epic Games, Inc. All Rights Reserved.

#include "MovieSceneDoodleClusterTrack.h"
// #include "GeometryCacheComponent.h"
// #include "MovieSceneGeometryCacheSection.h"
// #include "Compilation/MovieSceneCompilerRules.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
// #include "MovieSceneGeometryCacheTemplate.h"
// #include "Compilation/IMovieSceneTemplateGenerator.h"
// #include "MovieSceneGeometryCacheTemplate.h"
// #include "MovieScene.h"

#define LOCTEXT_NAMESPACE "MovieSceneDoodleClusterTrack"

/* UMovieSceneDoodleClusterTrack structors
 *****************************************************************************/

UMovieSceneDoodleClusterTrack::UMovieSceneDoodleClusterTrack(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
#if WITH_EDITORONLY_DATA
  // TrackTint = FColor(124, 15, 124, 65);
#endif

  // SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);

  // EvalOptions.bCanEvaluateNearestSection = true;
  // EvalOptions.bEvaluateInPreroll = true;
}

bool UMovieSceneDoodleClusterTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const {
  return true;
}

FMovieSceneEvalTemplatePtr UMovieSceneDoodleClusterTrack::CreateTemplateForSection(const UMovieSceneSection &InSection) const {
  // return FMovieSceneGeometryCacheSectionTemplate(*CastChecked<UMovieSceneGeometryCacheSection>(&InSection));
  return {};
}

#if WITH_EDITORONLY_DATA

FText UMovieSceneDoodleClusterTrack::GetDefaultDisplayName() const {
  return LOCTEXT("TrackName", "Doodle Cluster");
}

#endif

#undef LOCTEXT_NAMESPACE
