#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Misc/Paths.h"
#include "Widgets/SUserWidget.h"
#include "MapGenerator/DataType.h"

class SLoadBPAnimationUI : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SLoadBPAnimationUI) {
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

  TSharedRef<ITableRow> GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase> &OwnerTable);
  TSharedRef<ITableRow> GenerateMapInfoList(TSharedPtr<struct FMapInfo> Item, const TSharedRef<STableViewBase> &OwnerTable);
  void ShowSequence(TSharedPtr<FMapInfo> Item, ESelectInfo::Type Direct);
  void SetMapLoad(ECheckBoxState State);

  bool IsSaveInMap = true;
  void ItemsUpdateContent();

  FReply OpenProjcetDir();
  FReply ChooseAsset();

  void SaveInMap(ECheckBoxState State);
  void SaveInShot(ECheckBoxState State);

  FReply LoadBPAnim();
};
