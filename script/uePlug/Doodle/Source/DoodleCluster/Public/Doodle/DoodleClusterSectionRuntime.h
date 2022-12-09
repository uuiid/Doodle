#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneObjectBindingID.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "DoodleClusterSectionRuntime.generated.h"

class UMovieSceneSignedObject;

class UMovieSceneEntitySystemLinker;
class FEntityImportParams;
class FImportedEntity;

UCLASS()
class DOODLECLUSTER_API UDoodleClusterSection : public UMovieSceneSection, public IMovieSceneEntityProvider {
  GENERATED_BODY()

 public:
  /// 开始 IMovieSceneEntityProvider 接口
  virtual void ImportEntityImpl(
      UMovieSceneEntitySystemLinker *EntityLinker, const FEntityImportParams &Params, FImportedEntity *OutImportedEntity
  ) override;

 protected:
  // 开始 UMovieSceneSection 接口
  // virtual TOptional<TRange<FFrameNumber>> GetAutoSizeRange() const override;
  // virtual void TrimSection(FQualifiedFrameTime TrimTime, bool bTrimLeft, bool bDeleteKeys) override;
  // virtual UMovieSceneSection *SplitSection(FQualifiedFrameTime SplitTime, bool bDeleteKeys) override;
  // virtual void GetSnapTimes(TArray<FFrameNumber> &OutSnapTimes, bool bGetSectionBorders) const override;
  // virtual TOptional<FFrameTime> GetOffsetTime() const override;

 private:
 public:
  /** 这个是看向或者走向的物体 */
  UPROPERTY(EditAnywhere, Category = "Section")
  FMovieSceneObjectBindingID DoodleLockAtObject;
};