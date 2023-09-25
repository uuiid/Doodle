
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
class UMovieSceneSequence;

// 闪电序列器

class FDoodleLightningPostSequencerEditor final : public FMovieSceneTrackEditor {
 public:
  FDoodleLightningPostSequencerEditor(TSharedRef<ISequencer> InSequencer);
  virtual ~FDoodleLightningPostSequencerEditor() override;

  static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> InSequencer);

 public:
  // ISequencerTrackEditor interface
  // 在obj绑定中添加菜单
  virtual void BuildObjectBindingTrackMenu(
      FMenuBuilder &MenuBuilder, const TArray<FGuid> &ObjectBindings, const UClass *ObjectClass
  ) override;
  // 制作特定的部分
  virtual TSharedRef<ISequencerSection> MakeSectionInterface(
      UMovieSceneSection &SectionObject, UMovieSceneTrack &Track, FGuid ObjectBinding
  ) override;
  // 返回是否支持序列
  virtual bool SupportsSequence(UMovieSceneSequence *InSequence) const override;
  // 返回是否支持轨道类
  virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
  // 构造编辑小部件
  virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(
      const FGuid &ObjectBinding, UMovieSceneTrack *Track, const FBuildEditWidgetParams &Params
  ) override;
  // virtual const FSlateBrush *GetIconBrush() const overriFde;
 private:
  void AddNewObjectBindingTrack(TArray<FGuid> InObjectBindings) const;
};