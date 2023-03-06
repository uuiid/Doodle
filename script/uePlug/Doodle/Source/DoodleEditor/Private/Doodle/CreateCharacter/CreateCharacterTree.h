#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SCharacterEditorViewport;
struct FDoodleCreateCharacterConfigNode;
class UDoodleCreateCharacterConfig;

class ITableRow;
class STableViewBase;

class UCreateCharacterMianTreeItem {
 public:
  FString Name;

  FDoodleCreateCharacterConfigNode* Item;

  TArray<TSharedPtr<UCreateCharacterMianTreeItem>> Childs;
};

class SCreateCharacterTree : public STreeView<TSharedPtr<UCreateCharacterMianTreeItem>> {
 private:
  using Super = STreeView<TSharedPtr<UCreateCharacterMianTreeItem>>;

 public:
  SLATE_BEGIN_ARGS(SCreateCharacterTree)
      : _CreateCharacterConfig(nullptr) {}
  SLATE_ATTRIBUTE(UDoodleCreateCharacterConfig*, CreateCharacterConfig)
  SLATE_END_ARGS()

  using TreeVirwWeightItemType = TSharedPtr<UCreateCharacterMianTreeItem>;
  using TreeVirwWeightType     = STreeView<TreeVirwWeightItemType>;
  using TreeVirwWeightDataType = TArray<TreeVirwWeightItemType>;

  // 这里是内容创建函数
  void Construct(const FArguments& Arg);

  const static FName Name;

  static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

 private:
  TSharedRef<class ITableRow> CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table);
  void CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List);
  TSharedPtr<SWidget> Create_ContextMenuOpening();
  void On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo);

  void CreateUITree();
  // 数据
  TreeVirwWeightDataType CreateCharacterConfigTreeData{};

  // 配置引用
  TAttribute<UDoodleCreateCharacterConfig*> Config{};
  
  // 当前选择
  TreeVirwWeightItemType CurrentSelect;
};