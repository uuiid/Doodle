#pragma once

#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#include "CoreMinimal.h"
#include "MovieSceneNameableTrack.h"
#include "UObject/ObjectMacros.h"
//
#include "DoodleLightningPostTrack.generated.h"

UCLASS(MinimalAPI)
class UDoodleLightningPostTrack : public UMovieSceneNameableTrack, public IMovieSceneTrackTemplateProducer {
  GENERATED_UCLASS_BODY()
 public:
  // UMovieSceneTrack interface
  virtual void RemoveAllAnimationData() override;
  virtual bool HasSection(const UMovieSceneSection& Section) const override;
  virtual void AddSection(UMovieSceneSection& Section) override;
  virtual void RemoveSection(UMovieSceneSection& Section) override;
  virtual void RemoveSectionAt(int32 SectionIndex) override;
  virtual bool IsEmpty() const override;
  virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
  virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
  virtual UMovieSceneSection* CreateNewSection() override;
  /// 是否支持多个
  virtual bool SupportsMultipleRows() const override;

  // ~IMovieSceneTrackTemplateProducer interface
  /// 创建评估自定义类
  virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;

#if WITH_EDITORONLY_DATA
  virtual FText GetDefaultDisplayName() const override;
#endif

 private:
  /** List of all animation sections */
  UPROPERTY()
  TArray<UMovieSceneSection*> AnimationSections;
};
