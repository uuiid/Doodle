#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

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

  DECLARE_DELEGATE(FOnRenameRequested);
  FOnRenameRequested OnRenameRequested;
  TArray<TSharedPtr<UCreateCharacterMianTreeItem>> Childs;

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
  TSharedRef<class ITableRow> CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table);
  void CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List);
  TSharedPtr<SWidget> Create_ContextMenuOpening();
  void On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo);
  void On_MouseButtonDoubleClick(TreeVirwWeightItemType TreeItem);

  void AddBoneTreeMenu(FMenuBuilder& In_Builder);

  void Add_TreeNode(const FName& In_Bone_Name);

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
};