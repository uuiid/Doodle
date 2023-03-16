#include "CreateCharacterTree.h"

#include "CharacterEditorViewport.h"
#include "Engine/SkeletalMeshSocket.h"     // 骨骼 Socket
#include "Widgets/Input/SSlider.h"         // 滑动条
#include "SScrubControlPanel.h"            // 时间控制
#include "Framework/Docking/TabManager.h"  // 选项卡布局
#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"
#include "BoneSelectionWidget.h"  // 骨骼树

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
    if (ItemData->ItemKeys.IsEmpty()) {
      if (InColumnName == SCreateCharacterTree::G_Name) {
        L_Box->AddSlot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(ItemData->ShowName.ToString()))];
      }
    } else {
      if (InColumnName == SCreateCharacterTree::G_Name) {
        L_Box->AddSlot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(ItemData->ShowName.ToString()))];
      } else if (InColumnName == SCreateCharacterTree::G_Value) {
        L_Box->AddSlot().FillWidth(1.0f)[

            SNew(SSlider)
                .Value(Slider_Value)
                .MaxValue(ItemData->MaxValue)
                .MinValue(ItemData->MinValue)
                .OnValueChanged(FOnFloatValueChanged::CreateSP(this, &SCreateCharacterConfigTreeItem::On_FloatValueChanged))

        ];
      }
    }
    return L_Box.ToSharedRef();
  }

  virtual int32 DoesItemHaveChildren() const override {
    if (!ItemData->Childs.IsEmpty()) return 1;

    return Super::DoesItemHaveChildren();
  };

  void On_FloatValueChanged(float In_Value) {
    Slider_Value = In_Value;
    UE_LOG(LogTemp, Warning, TEXT("FileManipulation: %f"), In_Value);
  }

  virtual bool IsItemExpanded() const override { return Super::IsItemExpanded() || !ItemData->Childs.IsEmpty(); };

  virtual void ToggleExpansion() override { Super::ToggleExpansion(); };

 private:
  SCreateCharacterTree::TreeVirwWeightItemType ItemData;
  float Slider_Value;
};

#define LOCTEXT_NAMESPACE "SCreateCharacterTree"

const FName SCreateCharacterTree::Name{"Doodle_SCreateCharacterTree"};

const FName SCreateCharacterTree::G_Name{"Name"};
const FName SCreateCharacterTree::G_Value{"Value"};

void SCreateCharacterTree::Construct(const FArguments& Arg) {
  Config     = Arg._CreateCharacterConfig;
  OnEditItem = Arg._OnEditItem;

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
          .OnSelectionChanged(
              TreeVirwWeightType::FOnSelectionChanged::CreateSP(this, &SCreateCharacterTree::On_SelectionChanged)
          )
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
        LOCTEXT("Create_ContextMenuOpening_Add_Bone2", "Add Classify"),
        LOCTEXT("Create_ContextMenuOpening_Add_Bone2_Tip", "Add Classify"), FSlateIcon{"Subtitle", "EventIcon"},
        FUIAction{FExecuteAction::CreateLambda([this]() { AddBone(); })}
    );
    if (CurrentSelect) {
      // 修改
      L_Builder.AddMenuEntry(
          LOCTEXT("Create_ContextMenuOpening_Add_Bone3", "Edit"),
          LOCTEXT("Create_ContextMenuOpening_Add_Bone3_Tip", "Edit Bone"), FSlateIcon{"Subtitle", "EventIcon"},
          FUIAction{FExecuteAction::CreateLambda([this]() {
            if (CurrentSelect) this->OnEditItem.ExecuteIfBound(CurrentSelect);
          })}
      );

      L_Builder.AddSubMenu(
          LOCTEXT("Create_ContextMenuOpening_Add_Bone4", "Binding"),
          LOCTEXT("Create_ContextMenuOpening_Add_Bone4_Tip", "Binding Bone"),
          FNewMenuDelegate::CreateLambda([this](FMenuBuilder& In_Builder) {
            this->AddBoneTreeMenu(In_Builder);
          })
      );
    }

    L_Builder.EndSection();
  }

  return L_Builder.MakeWidget();
}

void SCreateCharacterTree::On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo) {
  CurrentSelect = TreeItem;
}

void SCreateCharacterTree::AddBoneTreeMenu(FMenuBuilder& In_Builder) {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();
  if (!L_Config)
    return;
  const bool bShowVirtualBones          = false;
  TSharedRef<SBoneTreeMenu> MenuContent = SNew(SBoneTreeMenu)
                                              .bShowVirtualBones(bShowVirtualBones)
                                              .Title(LOCTEXT("TargetBonePickerTitle", "Pick Target Bone..."))
                                              .OnBoneSelectionChanged_Lambda([this](FName In_BoneName) {
                                                this->Add_TreeNode(In_BoneName);
                                              })
                                              .OnGetReferenceSkeleton_Lambda([L_Config]() -> const FReferenceSkeleton& { return L_Config->GetSkeletalMesh()->GetRefSkeleton(); });

  In_Builder.AddWidget(MenuContent, FText::GetEmpty(), true);

  MenuContent->RegisterActiveTimer(
      0.0f,
      FWidgetActiveTimerDelegate::CreateLambda([FilterTextBox = MenuContent->GetFilterTextWidget()](double, float) {
        FSlateApplication::Get().SetKeyboardFocus(FilterTextBox);
        return EActiveTimerReturnType::Stop;
      })
  );
}

void SCreateCharacterTree::Add_TreeNode(const FName& In_Bone_Name) {
  if (!CurrentSelect)
    return;
  if (!CurrentSelect->Childs.IsEmpty())
    return;
  UDoodleCreateCharacterConfig* L_Config = Config.Get();
  if (!L_Config) return;

  TOptional<FString> L_Key = L_Config->Add_ConfigNode(In_Bone_Name, (CurrentSelect && CurrentSelect->ConfigNode) ? L_Config->ListTrees.Find(*CurrentSelect->ConfigNode) : INDEX_NONE);

  if (L_Key && !CurrentSelect->ItemKeys.Contains(*L_Key)) {
    CurrentSelect->ItemKeys.Add(*L_Key);
  }
  this->RebuildList();
}

namespace {

struct CreateUIAssist {
  SCreateCharacterTree::TreeVirwWeightItemType Node;
  TMap<FString, TSharedPtr<CreateUIAssist>> Child;
};

void AddNode(
    const SCreateCharacterTree::TreeVirwWeightItemType& InParent,
    UDoodleCreateCharacterConfig* InConfig,
    const TArray<int32>& InChildIndex
) {
  for (auto i : InChildIndex) {
    auto& L_Nodes = InConfig->ListTrees[i];
    // if (L_Nodes.Childs.IsEmpty()) continue;

    SCreateCharacterTree::TreeVirwWeightItemType L_Ptr =
        InParent->Childs.Add_GetRef(MakeShared<SCreateCharacterTree::TreeVirwWeightItemType::ElementType>());
    L_Ptr->ShowName   = L_Nodes.ShowUIName;
    // 添加子项
    L_Ptr->ItemKeys   = L_Nodes.Keys;
    L_Ptr->MaxValue   = L_Nodes.MaxValue;
    L_Ptr->MinValue   = L_Nodes.MinValue;
    L_Ptr->ConfigNode = &L_Nodes;
    AddNode(L_Ptr, InConfig, L_Nodes.Childs);
  }
}

}  // namespace

void SCreateCharacterTree::CreateUITree() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config) return;
  TMap<FString, TSharedPtr<CreateUIAssist>> L_List;
  CreateCharacterConfigTreeData.Empty(L_Config->ListConfigNode.Num());
  for (auto&& i : L_Config->ListTrees) {
    if (i.Parent != INDEX_NONE) {
      continue;
    }
    TreeVirwWeightItemType L_Ptr =
        CreateCharacterConfigTreeData.Add_GetRef(MakeShared<TreeVirwWeightItemType::ElementType>());
    L_Ptr->MaxValue   = i.MaxValue;
    L_Ptr->MinValue   = i.MinValue;
    L_Ptr->ConfigNode = &i;
    AddNode(L_Ptr, L_Config, i.Childs);
  }
}

void SCreateCharacterTree::AddBone() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config)
    return;

  TreeVirwWeightItemType L_Ptr{
      CurrentSelect ? CurrentSelect->Childs.Add_GetRef(MakeShared<TreeVirwWeightItemType::ElementType>())
                    : MakeShared<TreeVirwWeightItemType::ElementType>()};

  L_Ptr->ShowName                               = FName{"Add_Bone"};
  FDoodleCreateCharacterConfigUINode* L_UI_Node = L_Config->Add_TreeNode(
      (CurrentSelect && CurrentSelect->ConfigNode) ? L_Config->ListTrees.Find(*CurrentSelect->ConfigNode) : INDEX_NONE
  );
  L_UI_Node->ShowUIName = L_Ptr->ShowName;
  CreateUITree();
  this->RebuildList();
}

#undef LOCTEXT_NAMESPACE