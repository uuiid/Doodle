
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
class UMovieSceneSequence;

/**
 * ue4人群tack条目
 */
class FDoodleClusterTrackEditor final : public FMovieSceneTrackEditor {
 public:
  FDoodleClusterTrackEditor(TSharedRef<ISequencer> InSequencer);

  virtual ~FDoodleClusterTrackEditor() override;

  /**
   * 创建tack 的静态方法, 可以用来注册使用
   *
   * @param OwningSequencer 定序器实例
   * @return 新的创建的类
   */
  static TSharedRef<ISequencerTrackEditor>
  CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

 public:
  // ISequencerTrackEditor interface
  // 在obj绑定中添加菜单
  virtual void BuildObjectBindingTrackMenu(
      FMenuBuilder &MenuBuilder,
      const TArray<FGuid> &ObjectBindings,
      const UClass *ObjectClass
  ) override;
  // 制作特定的部分
  virtual TSharedRef<ISequencerSection> MakeSectionInterface(
      UMovieSceneSection &SectionObject,
      UMovieSceneTrack &Track, FGuid ObjectBinding
  ) override;
  // 返回是否支持序列
  virtual bool SupportsSequence(UMovieSceneSequence *InSequence) const override;
  // 返回是否支持轨道类
  virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
  // 构造编辑小部件
  virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid &ObjectBinding, UMovieSceneTrack *Track, const FBuildEditWidgetParams &Params) override;
  // virtual const FSlateBrush *GetIconBrush() const overriFde;

 private:
  
  void AddNewObjectBindingTrack(TArray<FGuid> InObjectBindings) const;

  void AddAllBindAnimationInstance(    const TArray<FGuid> &ObjectBindings)const;
};
