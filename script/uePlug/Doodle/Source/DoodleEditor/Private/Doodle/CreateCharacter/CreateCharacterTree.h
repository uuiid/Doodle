#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"

class SCharacterEditorViewport;
struct FDoodleCreateCharacterConfigNode;
class UDoodleCreateCharacterConfig;

class ITableRow;
class STableViewBase;
class UCreateCharacterMianTreeItem;

DECLARE_DELEGATE_OneParam(FDoodleTreeEdit, TSharedPtr<UCreateCharacterMianTreeItem>);

class UCreateCharacterMianTreeItem {
 private:
  TObjectPtr<UDoodleCreateCharacterConfig> Config;
  int32 ConfigNode_Index{};

 public:
  UCreateCharacterMianTreeItem(UDoodleCreateCharacterConfig* In_Config) : Config(In_Config){};

  FDoodleCreateCharacterConfigUINode& Get();
  inline int32 Get_Index() { return ConfigNode_Index; };
  inline void Set(int32 In_Index) {
    ConfigNode_Index = In_Index;
  };

  operator bool() const;

  TRange<FFrameNumber> GetPlaybackRange();
  void SetPlaybackRange(const TRange<FFrameNumber>& In_Range);

  TArray<TSharedPtr<UCreateCharacterMianTreeItem>> Childs;
  TWeakPtr<UCreateCharacterMianTreeItem> Parent;

  FSimpleDelegate OnRenameRequested;
  FSimpleDelegate OnSetRange;

  static bool Sort_Child(
      const TSharedPtr<UCreateCharacterMianTreeItem>& In_L, const TSharedPtr<UCreateCharacterMianTreeItem>& In_R
  );
  bool Sort_Child();

 private:
};

class SCreateCharacterTree : public STreeView<TSharedPtr<UCreateCharacterMianTreeItem>> {
 private:
  using Super = STreeView<TSharedPtr<UCreateCharacterMianTreeItem>>;
  friend class SCreateCharacterConfigTreeItem;

 public:
  using TreeVirwWeightItemType = TSharedPtr<UCreateCharacterMianTreeItem>;
  using TreeVirwWeightType     = STreeView<TreeVirwWeightItemType>;
  using TreeVirwWeightDataType = TArray<TreeVirwWeightItemType>;

  SLATE_BEGIN_ARGS(SCreateCharacterTree) : _CreateCharacterConfig(nullptr), _OnEditItem(), _OnModifyWeights() {}

  SLATE_ATTRIBUTE(UDoodleCreateCharacterConfig*, CreateCharacterConfig)

  SLATE_EVENT(FDoodleTreeEdit, OnEditItem)

  SLATE_EVENT(FDoodleTreeEdit, OnModifyWeights)

  SLATE_END_ARGS()

  // 这里是内容创建函数
  void Construct(const FArguments& Arg);

  TreeVirwWeightItemType GetSelectItem() const {
    return CurrentSelect;
  };

  const static FName Name;

  static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

 private:
  TSharedRef<class ITableRow> CreateCharacterConfigTreeData_Row(
      TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table
  );
  void CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List);
  TSharedPtr<SWidget> Create_ContextMenuOpening();
  void On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo);
  void On_MouseButtonDoubleClick(TreeVirwWeightItemType TreeItem);

  void AddBoneTreeMenu(FMenuBuilder& In_Builder);

  void Add_TreeNode(const FName& In_Bone_Name);

  void Sort_Root();
  void My_OnSortModeChanged(
      EColumnSortPriority::Type In_Sort_Priority, const FName& In_Name, EColumnSortMode::Type In_Mode
  );

  void CreateUITree();

  void AddBone();
  void Delete_UiTreeNode();
  // 数据
  TreeVirwWeightDataType CreateCharacterConfigTreeData{};

  // 配置引用
  TAttribute<UDoodleCreateCharacterConfig*> Config{};

  // 当前选择
  TreeVirwWeightItemType CurrentSelect;
  FDoodleTreeEdit OnEditItem;
  FDoodleTreeEdit OnModifyWeights;

  // 上下文ui元素
  TSharedPtr<FUICommandList> UICommandList;
  TSharedPtr<FExtender> Extender;

  const static FName G_Name;
  const static FName G_Value;

  //// DragBegin
  /////  当拖动进入一个小部件时在拖放过程中调用
  // void OnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override;
  ///// 当拖动离开小部件时在拖放过程中调用
  // void OnDragLeave(const FDragDropEvent& InDragDropEvent) override;
  ///// 当鼠标被拖动到小部件上时，在拖放过程中调用
  // FReply OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override;
  ///// 当用户把东西放到小部件上时被调用 终止拖放
  // FReply OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override;
  //// DragEnd
};