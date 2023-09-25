#include "Doodle/DoodleLightningPostSequencerSection.h"

#include "Doodle/DoodleClusterSectionRuntime.h"
#include "EditorStyleSet.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "SequencerSectionPainter.h"
FDoodleLightningPostSequencerSection::FDoodleLightningPostSequencerSection(
    UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer
) {}

FDoodleLightningPostSequencerSection::~FDoodleLightningPostSequencerSection() = default;

UMovieSceneSection* FDoodleLightningPostSequencerSection::GetSectionObject() { return Section; }

FText FDoodleLightningPostSequencerSection::GetSectionTitle() const {
  return FText::FromString(TEXT("Doodle Curve Section"));
}

float FDoodleLightningPostSequencerSection::GetSectionHeight() const { return 20.0f; }

int32 FDoodleLightningPostSequencerSection::OnPaintSection(FSequencerSectionPainter& InPainter) const {
  const int32 LayerId = InPainter.PaintSectionBackground();

  if (!Section->HasStartFrame() || !Section->HasEndFrame()) {
    return LayerId;
  }
  TSharedPtr<ISequencer> SequencerPtr = Sequencer.Pin();
  if (InPainter.bIsSelected && SequencerPtr.IsValid()) {
    const ESlateDrawEffect DrawEffects =
        InPainter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
    static const FSlateBrush* GenericDivider = FAppStyle::GetBrush("Sequencer.GenericDivider");
    const FLinearColor DrawColor             = FAppStyle::GetSlateColor("SelectionColor").GetColor(FWidgetStyle());

    FString FrameString                      = FString::FromInt(0);
    TArrayView<TWeakObjectPtr<UObject>> L_Objects =
        SequencerPtr->FindObjectsInCurrentSequence(Section->DoodleLockAtObject.GetGuid());
    for (auto&& i : L_Objects) {
      if (i.IsValid()) {
        FrameString = i.Get()->GetName();
        break;
      }
    }

    const FSlateFontInfo SmallLayoutFont             = FCoreStyle::GetDefaultFontStyle("Bold", 10);

    TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    FVector2D TextSize                               = FontMeasureService->Measure(FrameString, SmallLayoutFont);

    FSlateDrawElement::MakeText(
        InPainter.DrawElements, LayerId + 6,
        InPainter.SectionGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform{}), FrameString, SmallLayoutFont,
        DrawEffects, DrawColor
    );
  }

  return LayerId;
}

void FDoodleLightningPostSequencerSection::BeginResizeSection() { ISequencerSection::BeginResizeSection(); }

void FDoodleLightningPostSequencerSection::ResizeSection(
    ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime
) {
  ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}
