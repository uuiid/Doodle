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
    // 只是题头部件
    TSharedPtr<SHorizontalBox> L_Box = SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth()[SNew(SExpanderArrow, SharedThis(this)).ShouldDrawWires(true)];
    if (!ItemData->Item) {
      if (InColumnName == SCreateCharacterTree::G_Name) {
        L_Box->AddSlot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(ItemData->Name))];
      }
    } else {
      if (InColumnName == SCreateCharacterTree::G_Name) {
        L_Box->AddSlot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(ItemData->Name))];
      } else if (InColumnName == SCreateCharacterTree::G_Value) {
        L_Box->AddSlot().AutoWidth(
        )[SNew(SSlider).MaxValue(ItemData->Item->MaxValue).MinValue(ItemData->Item->MinValue)];
      }
    }
    return L_Box.ToSharedRef();
  }

  virtual int32 DoesItemHaveChildren() const override {
    if (!ItemData->Childs.IsEmpty())
      return 1;

    return Super::DoesItemHaveChildren();
  };

  virtual bool IsItemExpanded() const override {
    return Super::IsItemExpanded() || !ItemData->Childs.IsEmpty();
  };

  virtual void ToggleExpansion() override {
    Super::ToggleExpansion(); };

 private:
  SCreateCharacterTree::TreeVirwWeightItemType ItemData;
};

#define LOCTEXT_NAMESPACE "SCreateCharacterTree"

const FName SCreateCharacterTree::Name{"Doodle_SCreateCharacterTree"};

const FName SCreateCharacterTree::G_Name{"Name"};
const FName SCreateCharacterTree::G_Value{"Value"};

void SCreateCharacterTree::Construct(const FArguments& Arg) {
  Config = Arg._CreateCharacterConfig;
  CreateUITree();

  Super::Construct(
      Super::FArguments{}
          .TreeItemsSource(&CreateCharacterConfigTreeData)
          .OnGenerateRow(TreeVirwWeightType::FOnGenerateRow::CreateSP(
              this, &SCreateCharacterTree::CreateCharacterConfigTreeData_Row
          ))
          .OnGetChildren(TreeVirwWeightType::FOnGetChildren::CreateSP(
              this, &SCreateCharacterTree::CreateCharacterConfigTreeData_GetChildren
          ))
          .HeaderRow(
              // clang-format off
                           SNew(SHeaderRow) 
                         + SHeaderRow::Column(G_Name)
                         .DefaultLabel(LOCTEXT("Construct", "Name")) 
                         + SHeaderRow::Column(G_Value)
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
  FMenuBuilder L_Builder{true, UICommandList, Extender};
  {
    L_Builder.BeginSection("Create_ContextMenuOpening_Add_Bone", LOCTEXT("Create_ContextMenuOpening_Add_Bone1", "Add"));

    // 添加
    L_Builder.AddMenuEntry(
        LOCTEXT("Create_ContextMenuOpening_Add_Bone2", "Add"),
        LOCTEXT("Create_ContextMenuOpening_Add_Bone2_Tip", "Add Bone"), FSlateIcon{"Subtitle", "EventIcon"},
        FUIAction{FExecuteAction::CreateLambda([this]() { AddBone(); })}
    );
    // 修改
    L_Builder.AddMenuEntry(
        LOCTEXT("Create_ContextMenuOpening_Add_Bone3", "Edit"),
        LOCTEXT("Create_ContextMenuOpening_Add_Bone3_Tip", "Edit Bone"), FSlateIcon{"Subtitle", "EventIcon"},
        FUIAction{FExecuteAction::CreateLambda([this]() {
          if (CurrentSelect->Item) this->OnEditItem.ExecuteIfBound(CurrentSelect->Item);
        })}

    );

    L_Builder.EndSection();
  }

  return L_Builder.MakeWidget();
}

void SCreateCharacterTree::On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo) {
  CurrentSelect = TreeItem;
}

namespace {

struct CreateUIAssist {
  SCreateCharacterTree::TreeVirwWeightItemType Node;
  TMap<FString, TSharedPtr<CreateUIAssist>> Child;
};

void AddNode(
    FString& In_Path, TMap<FString, TSharedPtr<CreateUIAssist>>& In_Node, FDoodleCreateCharacterConfigNode* In_DataNode, const SCreateCharacterTree::TreeVirwWeightItemType& In_Parent = nullptr
) {
  FString L_Left, L_Right{};
  const bool L_B_Split = In_Path.Split(TEXT("/"), &L_Left, &L_Right);
  if (!L_B_Split) {
    if (!In_Node.Contains(In_Path)) {
      In_Node.Emplace(In_Path, MakeShared<CreateUIAssist>());
      In_Node[In_Path]->Node       = MakeShared<SCreateCharacterTree::TreeVirwWeightItemType::ElementType>();
      In_Node[In_Path]->Node->Name = In_Path;
    }
    if (In_Parent)
      In_Parent->Childs.Add(In_Node[In_Path]->Node);
    In_Node[In_Path]->Node->Item = In_DataNode;
    return;
  }

  if (!In_Node.Contains(L_Left)) {
    In_Node.Emplace(L_Left, MakeShared<CreateUIAssist>());
    In_Node[L_Left]->Node       = MakeShared<SCreateCharacterTree::TreeVirwWeightItemType::ElementType>();
    In_Node[L_Left]->Node->Name = L_Left;
    if (In_Parent)
      In_Parent->Childs.Add(In_Node[L_Left]->Node);
  }
  AddNode(L_Right, In_Node[L_Left]->Child, In_DataNode, In_Node[L_Left]->Node);
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

void SCreateCharacterTree::AddBone() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config)
    return;

  FDoodleCreateCharacterConfigNode& L_Node = L_Config->ListConfigNode.Emplace_GetRef();
  L_Node.BoneName                          = "None";
  L_Node.ShowUIName                        = FString::Format(TEXT("{0}{1}"), FStringFormatOrderedArguments{TEXT("Root/Add_Bone"), L_Config->ListConfigNode.Num()});

  L_Config->GetPackage()->MarkPackageDirty();

  CreateUITree();
  this->RebuildList();
}

#undef LOCTEXT_NAMESPACE