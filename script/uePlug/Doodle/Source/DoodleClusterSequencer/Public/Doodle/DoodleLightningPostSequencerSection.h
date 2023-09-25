
#pragma once

#include "CoreMinimal.h"
#include "ISequencer.h"
#include "ISequencerSection.h"
#include "ISequencerTrackEditor.h"
#include "Misc/Guid.h"
#include "MovieSceneTrack.h"
#include "MovieSceneTrackEditor.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SWidget.h"

struct FAssetData;
class FMenuBuilder;
class FSequencerSectionPainter;
class UMovieSceneGeometryCacheSection;
class UMovieSceneSequence;
class UMovieSceneSection;
class UDoodleClusterSection;

class FDoodleLightningPostSequencerSection : public ISequencerSection,
                                             public TSharedFromThis<FDoodleLightningPostSequencerSection> {
 public:
  /** Constructor. */
  FDoodleLightningPostSequencerSection(UMovieSceneSection &InSection, TWeakPtr<ISequencer> InSequencer);

  /** Virtual destructor. */
  virtual ~FDoodleLightningPostSequencerSection();

 public:
  // 开始 ISequencerSection interface

  virtual UMovieSceneSection *GetSectionObject() override;
  virtual FText GetSectionTitle() const override;
  virtual float GetSectionHeight() const override;
  virtual int32 OnPaintSection(FSequencerSectionPainter &Painter) const override;
  virtual void BeginResizeSection() override;
  virtual void ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime) override;
  // virtual void BeginSlipSection() override;
  // virtual void SlipSection(FFrameNumber SlipTime) override;
  // virtual void BeginDilateSection() override;
  // virtual void DilateSection(const TRange<FFrameNumber> &NewRange, float DilationFactor) override;

 private:
  UDoodleClusterSection *Section;
  TWeakPtr<ISequencer> Sequencer;
};
