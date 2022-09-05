#include "Doodle/DoodleClusterSection.h"

FDoodleClusterSection::FDoodleClusterSection(
    UMovieSceneSection &InSection, TWeakPtr<ISequencer> InSequencer)
    : ISequencerSection()
{
}
FDoodleClusterSection::~FDoodleClusterSection()=default;

UMovieSceneSection *FDoodleClusterSection::GetSectionObject()
{
    return {};
}

FText FDoodleClusterSection::GetSectionTitle() const
{
    return FText::FromString(TEXT("Doodle Test"));
}

float FDoodleClusterSection::GetSectionHeight() const
{
    return {

    };
}

int32 FDoodleClusterSection::OnPaintSection(FSequencerSectionPainter &Painter) const
{
    return {

    };
}

void FDoodleClusterSection::BeginResizeSection()
{
}

void FDoodleClusterSection::ResizeSection(
    ESequencerSectionResizeMode ResizeMode,
    FFrameNumber ResizeTime)
{
}

void FDoodleClusterSection::BeginSlipSection()
{
}

void FDoodleClusterSection::SlipSection(FFrameNumber SlipTime)
{
}

void FDoodleClusterSection::BeginDilateSection()
{
}

void FDoodleClusterSection::DilateSection(const TRange<FFrameNumber> &NewRange, float DilationFactor)
{
}
