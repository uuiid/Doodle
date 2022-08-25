#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneNameableTrack.h"
#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#include "MovieSceneDoodleClusterTrack.generated.h"

/**
 * Handles animation of geometry cache actors
 */
class UGeometryCacheComponent;

UCLASS(MinimalAPI)
class UMovieSceneDoodleClusterTrack
    : public UMovieSceneNameableTrack,
      public IMovieSceneTrackTemplateProducer {
  GENERATED_UCLASS_BODY()
 public:
 public:
  // UMovieSceneTrack interface
  // virtual void RemoveAllAnimationData() override;
  // virtual bool HasSection(const UMovieSceneSection &Section) const override;
  // virtual void AddSection(UMovieSceneSection &Section) override;
  // virtual void RemoveSection(UMovieSceneSection &Section) override;
  // virtual void RemoveSectionAt(int32 SectionIndex) override;
  // virtual bool IsEmpty() const override;
  // virtual const TArray<UMovieSceneSection *> &GetAllSections() const override;
  virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
  // virtual UMovieSceneSection *CreateNewSection() override;

  // ~IMovieSceneTrackTemplateProducer interface
  virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection &InSection) const override;

#if WITH_EDITORONLY_DATA
  virtual FText GetDefaultDisplayName() const override;
#endif

 private:
  /** List of all animation sections */
  UPROPERTY()
  TArray<UMovieSceneSection *> AnimationSections;
};
