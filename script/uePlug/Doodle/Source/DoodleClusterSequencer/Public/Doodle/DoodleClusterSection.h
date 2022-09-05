
#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SWidget.h"
#include "ISequencer.h"
#include "MovieSceneTrack.h"
#include "ISequencerSection.h"
#include "ISequencerTrackEditor.h"
#include "MovieSceneTrackEditor.h"

struct FAssetData;
class FMenuBuilder;
class FSequencerSectionPainter;
class UMovieSceneGeometryCacheSection;
class UMovieSceneSequence;

class FDoodleClusterSection
    : public ISequencerSection,
      public TSharedFromThis<FDoodleClusterSection>
{
public:
    /** Constructor. */
    FDoodleClusterSection(UMovieSceneSection &InSection, TWeakPtr<ISequencer> InSequencer);

    /** Virtual destructor. */
    virtual ~FDoodleClusterSection();

public:
    // ISequencerSection interface

    virtual UMovieSceneSection *GetSectionObject() override;
    virtual FText GetSectionTitle() const override;
    virtual float GetSectionHeight() const override;
    virtual int32 OnPaintSection(FSequencerSectionPainter &Painter) const override;
    virtual void BeginResizeSection() override;
    virtual void ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime) override;
    virtual void BeginSlipSection() override;
    virtual void SlipSection(FFrameNumber SlipTime) override;
    virtual void BeginDilateSection() override;
    virtual void DilateSection(const TRange<FFrameNumber> &NewRange, float DilationFactor) override;
};
