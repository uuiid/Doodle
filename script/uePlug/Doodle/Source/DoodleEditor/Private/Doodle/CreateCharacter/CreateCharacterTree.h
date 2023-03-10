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
  TArray<FString> ItemKeys;
  FName ShowName;
  // 调整最大值
  float MaxValue{2.0f};
  // 调整最小值
  float MinValue{-2.0f};
  TArray<TSharedPtr<UCreateCharacterMianTreeItem>> Childs;
};

class SCreateCharacterTree : public STreeView<TSharedPtr<UCreateCharacterMianTreeItem>> {
 private:
  using Super = STreeView<TSharedPtr<UCreateCharacterMianTreeItem>>;
  friend class SCreateCharacterConfigTreeItem;

 public:
  using TreeVirwWeightItemType = TSharedPtr<UCreateCharacterMianTreeItem>;
  using TreeVirwWeightType     = STreeView<TreeVirwWeightItemType>;
  using TreeVirwWeightDataType = TArray<TreeVirwWeightItemType>;

  DECLARE_DELEGATE_OneParam(FDoodleTreeEdit, const TSharedPtr<UCreateCharacterMianTreeItem>&);

  SLATE_BEGIN_ARGS(SCreateCharacterTree) : _CreateCharacterConfig(nullptr), _OnEditItem() {}

  SLATE_ATTRIBUTE(UDoodleCreateCharacterConfig*, CreateCharacterConfig)

  SLATE_EVENT(FDoodleTreeEdit, OnEditItem)

  SLATE_END_ARGS()

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

  void AddBone();
  // 数据
  TreeVirwWeightDataType CreateCharacterConfigTreeData{};

  // 配置引用
  TAttribute<UDoodleCreateCharacterConfig*> Config{};

  // 当前选择
  TreeVirwWeightItemType CurrentSelect;
  FDoodleTreeEdit OnEditItem;

  // 上下文ui元素
  TSharedPtr<FUICommandList> UICommandList;
  TSharedPtr<FExtender> Extender;

  const static FName G_Name;
  const static FName G_Value;
};