#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Misc/Paths.h"
#include "Widgets/SUserWidget.h"
#include "MapGenerator/DataType.h"

class SLoadSkeletonAnimationUI : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SLoadSkeletonAnimationUI) {
  }
  SLATE_END_ARGS()

  void Construct(const FArguments &InArgs);

  FString DefaultOpenFileDir = FPaths::ProjectContentDir();
  FString DefaultOpenFbxDir  = "C:/";

  TArray<TSharedPtr<FString>> ItemsMap, ItemsMapPackage, ItemsSequence, ItemsShot, ItemsEmpty;
  TArray<TSharedPtr<FMapInfo>> ItemsMapInfo;

  TArray<TArray<TSharedPtr<FString>>> ItemsAllSequence, ItemsAllSequencePackage;
  TSharedPtr<class SEditableTextBox> TextPorject, TextAsset, TextSceneMap, TextStartFrame;
  TSharedPtr<class SCheckBox> CheckBoxShot, CheckBoxMap;

  TSharedPtr<class SListView<TSharedPtr<FString>>> ListViewSequence, ListViewMap;
  TSharedPtr<class SListView<TSharedPtr<FMapInfo>>> ListViewMapInfo;
  /// 生成列表
  TSharedRef<ITableRow> GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase> &OwnerTable);
  /// 生成地图信息列表
  TSharedRef<ITableRow> GenerateMapInfoList(TSharedPtr<struct FMapInfo> Item, const TSharedRef<STableViewBase> &OwnerTable);
  /// 显示定序器
  void ShowSequence(TSharedPtr<FMapInfo> Item, ESelectInfo::Type Direct);
  /// 设置地图加载
  void SetMapLoad(ECheckBoxState State);

  bool IsSaveInMap = true;
  ///  列表项目更新
  void ItemsUpdateContent();

  /// 打开项目目录
  FReply OpenProjcetDir();
  /// 选择资产
  FReply ChooseAsset();

  /// 保存在地图
  void SaveInMap(ECheckBoxState State);
  /// 保存在镜头中
  void SaveInShot(ECheckBoxState State);

  FReply LoadSkeletonAnim();
};
