// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "MovieSceneSection.h"
//
#include "DoodleLightningPostSection.generated.h"
class UMovieSceneSignedObject;

class UMovieSceneEntitySystemLinker;
class FEntityImportParams;
class FImportedEntity;
class UCurveFloat;

/**
 *
 */
UCLASS()
class DOODLECLUSTER_API UDoodleLightningPostSection : public UMovieSceneSection, public IMovieSceneEntityProvider {
  GENERATED_BODY()

 public:
 public:
  /// 开始 IMovieSceneEntityProvider 接口
  virtual void ImportEntityImpl(
      UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity
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
  /** 强度曲线 */
  UPROPERTY(EditAnywhere, Category = "Section")
  TObjectPtr<UCurveFloat> IntensityCurve;
};
