#include "Doodle/DoodleClusterSection.h"

#include "SequencerSectionPainter.h"

#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

#include "Doodle/DoodleClusterSectionRuntime.h"
#include "EditorStyleSet.h"
FDoodleClusterSection::FDoodleClusterSection(UMovieSceneSection &InSection, TWeakPtr<ISequencer> InSequencer)
    : ISequencerSection(), Section(CastChecked<UDoodleClusterSection>(&InSection)), Sequencer(InSequencer) {}
FDoodleClusterSection::~FDoodleClusterSection() = default;

UMovieSceneSection *FDoodleClusterSection::GetSectionObject() { return Section; }

FText FDoodleClusterSection::GetSectionTitle() const { return FText::FromString(TEXT("Doodle Section")); }

float FDoodleClusterSection::GetSectionHeight() const { return 20.0f; }

int32 FDoodleClusterSection::OnPaintSection(FSequencerSectionPainter &InPainter) const {
  const int32 LayerId = InPainter.PaintSectionBackground();

  if (!Section->HasStartFrame() || !Section->HasEndFrame()) {
    return LayerId;
  }

  TSharedPtr<ISequencer> SequencerPtr = Sequencer.Pin();
  if (InPainter.bIsSelected && SequencerPtr.IsValid()) {
    const ESlateDrawEffect DrawEffects =
        InPainter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1)
    static const FSlateBrush *GenericDivider = FAppStyle::GetBrush("Sequencer.GenericDivider");
    const FLinearColor DrawColor             = FAppStyle::GetSlateColor("SelectionColor").GetColor(FWidgetStyle());
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0) || \
    (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27)
    static const FSlateBrush *GenericDivider = FEditorStyle::GetBrush("Sequencer.GenericDivider");
    const FLinearColor DrawColor             = FEditorStyle::GetSlateColor("SelectionColor").GetColor(FWidgetStyle());
#endif
    FString FrameString = FString::FromInt(0);
    TArrayView<TWeakObjectPtr<UObject>> L_Objects =
        SequencerPtr->FindObjectsInCurrentSequence(Section->DoodleLockAtObject.GetGuid());
    for (auto &&i : L_Objects) {
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

void FDoodleClusterSection::BeginResizeSection() {}

void FDoodleClusterSection::ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime) {
  if (ResizeMode == SSRM_LeadingEdge) {
  }
  ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}

// void FDoodleClusterSection::BeginSlipSection()
// {
// }

// void FDoodleClusterSection::SlipSection(FFrameNumber SlipTime)
// {
// }

// void FDoodleClusterSection::BeginDilateSection()
// {
// }

// void FDoodleClusterSection::DilateSection(const TRange<FFrameNumber> &NewRange, float DilationFactor)
// {
// }
