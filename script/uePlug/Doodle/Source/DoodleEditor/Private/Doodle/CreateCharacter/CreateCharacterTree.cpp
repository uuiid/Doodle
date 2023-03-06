#include "CreateCharacterTree.h"

#include "CharacterEditorViewport.h"
#include "Engine/SkeletalMeshSocket.h"     // 骨骼 Socket
#include "Widgets/Input/SSlider.h"         // 滑动条
#include "SScrubControlPanel.h"            // 时间控制
#include "Framework/Docking/TabManager.h"  // 选项卡布局
#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"

class SCreateCharacterConfigTreeItem : public SMultiColumnTableRow<SCreateCharacterTree::TreeVirwWeightItemType> {
 public:
  using Super = SMultiColumnTableRow<SCreateCharacterTree::TreeVirwWeightItemType>;
  SLATE_BEGIN_ARGS(SCreateCharacterConfigTreeItem)
      : _ItemData() {}

  SLATE_ARGUMENT(SCreateCharacterTree::TreeVirwWeightItemType, ItemData)

  SLATE_END_ARGS()

  // 这里是内容创建函数
  void Construct(const FArguments& Arg, const TSharedRef<STableViewBase>& OwnerTableView) {
    ItemData = Arg._ItemData;
    Super::Construct(Super::FArguments{}, OwnerTableView);
  }

  TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override {
    if (InColumnName == "Name") {
      return SNew(STextBlock).Text(FText::FromString(ItemData->Name));
    } else if (InColumnName == "Value") {
      if (ItemData->Item)
        return SNew(SSlider).MaxValue(ItemData->Item->MaxValue).MinValue(ItemData->Item->MinValue);
      else
        return SNew(STextBlock);
    } else {
      return SNew(STextBlock);
    }
  }

 private:
  SCreateCharacterTree::TreeVirwWeightItemType ItemData;
};

#define LOCTEXT_NAMESPACE "SCreateCharacterTree"

const FName SCreateCharacterTree::Name{"Doodle_SCreateCharacterTree"};

void SCreateCharacterTree::Construct(const FArguments& Arg) {
  Config = Arg._CreateCharacterConfig;
  CreateUITree();

  Super::Construct(Super::FArguments{}
                       .TreeItemsSource(&CreateCharacterConfigTreeData)
                       .OnGenerateRow(TreeVirwWeightType::FOnGenerateRow::CreateSP(this, &SCreateCharacterTree::CreateCharacterConfigTreeData_Row))
                       .OnGetChildren(TreeVirwWeightType::FOnGetChildren::CreateSP(this, &SCreateCharacterTree::CreateCharacterConfigTreeData_GetChildren))
                       .HeaderRow(
                           // clang-format off
                           SNew(SHeaderRow) 
                         + SHeaderRow::Column(FName{TEXT("Name")})
                         .DefaultLabel(LOCTEXT("Construct", "Name")) 
                         + SHeaderRow::Column(FName{TEXT("Value")})
                         .DefaultLabel(LOCTEXT("Construct", "Value"))
                           // clang-format on
                       )
                       .OnContextMenuOpening(FOnContextMenuOpening::CreateSP(this, &SCreateCharacterTree::Create_ContextMenuOpening))
                       .OnSelectionChanged(TreeVirwWeightType::FOnSelectionChanged::CreateSP(this, &SCreateCharacterTree::On_SelectionChanged))
  );
}

TSharedRef<SDockTab> SCreateCharacterTree::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterTree)];  //
}

TSharedRef<class ITableRow> SCreateCharacterTree::CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table) {
  return SNew(SCreateCharacterConfigTreeItem, In_Table).ItemData(In_Value);
}

void SCreateCharacterTree::CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List) {
  In_List = In_Value->Childs;
}

TSharedPtr<SWidget> SCreateCharacterTree::Create_ContextMenuOpening() {



  return TSharedPtr<SWidget>();
}

void SCreateCharacterTree::On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo) {
  CurrentSelect = TreeItem;
}

namespace {

struct CreateUIAssist {
  SCreateCharacterTree::TreeVirwWeightItemType Node;
  TMap<FString, TSharedPtr<CreateUIAssist>> Child;
};

bool AddNode(
    FString& In_Path, TMap<FString, TSharedPtr<CreateUIAssist>>& In_Node, FDoodleCreateCharacterConfigNode* In_DataNode
) {
  FString L_Left, L_Right{};
  const bool L_B_Split = In_Path.Split(TEXT("/"), &L_Left, &L_Right);
  if (!L_B_Split)
    return L_B_Split;

  if (!In_Node.Contains(L_Left)) {
    In_Node.Emplace(L_Left, MakeShared<CreateUIAssist>());
    In_Node[L_Left]->Node       = MakeShared<SCreateCharacterTree::TreeVirwWeightItemType::ElementType>();
    In_Node[L_Left]->Node->Name = L_Left;
  }
  if (!AddNode(L_Right, In_Node[L_Left]->Child, In_DataNode)) {
    In_Node[L_Left]->Node->Item = In_DataNode;
  }
  return L_B_Split;
}

}  // namespace

void SCreateCharacterTree::CreateUITree() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config)
    return;
  TMap<FString, TSharedPtr<CreateUIAssist>> L_List;
  for (auto&& i : L_Config->ListConfigNode) {
    FPaths::NormalizeDirectoryName(i.ShowUIName);
    AddNode(i.ShowUIName, L_List, &i);
  }

  CreateCharacterConfigTreeData.Empty(L_Config->ListConfigNode.Num());
  for (auto&& i : L_List) {
    CreateCharacterConfigTreeData.Emplace(i.Value->Node);
  }
}

#undef LOCTEXT_NAMESPACE