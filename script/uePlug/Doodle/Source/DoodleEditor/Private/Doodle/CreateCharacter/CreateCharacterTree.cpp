#include "CreateCharacterTree.h"

#include "BoneSelectionWidget.h"  // 骨骼树
#include "CharacterEditorViewport.h"
#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"
#include "Engine/SkeletalMeshSocket.h"              // 骨骼 Socket
#include "Framework/Docking/TabManager.h"           // 选项卡布局
#include "Input/DragAndDrop.h"                      // 拖拽数据源
#include "SScrubControlPanel.h"                     // 时间控制
#include "Widgets/Input/SSlider.h"                  // 滑动条
#include "Widgets/Text/SInlineEditableTextBlock.h"  // 内联编辑小部件

// #include "SortHelper.h"

#define LOCTEXT_NAMESPACE "SCreateCharacterConfigTreeItem"

FDoodleCreateCharacterConfigUINode& UCreateCharacterMianTreeItem::Get() { return Config->ListTrees[ConfigNode_Index]; }

UCreateCharacterMianTreeItem::operator bool() const {
  return Config && ConfigNode_Index != INDEX_NONE && ConfigNode_Index < Config->ListTrees.Num();
}

TRange<FFrameNumber> UCreateCharacterMianTreeItem::GetPlaybackRange() {
  if (*this) {
    return TRange<FFrameNumber>{
        (Get().MinValue * FFrameRate{}).FloorToFrame(),
        (Get().MaxValue * FFrameRate{}).FloorToFrame()};
  } else
    return TRange<FFrameNumber>();
}

void UCreateCharacterMianTreeItem::SetPlaybackRange(const TRange<FFrameNumber>& In_Range) {
  if (*this) {
    Get().MinValue = In_Range.GetLowerBoundValue() / FFrameRate{};
    Get().MaxValue = In_Range.GetUpperBoundValue() / FFrameRate{};
    OnSetRange.ExecuteIfBound();
  }
}

bool UCreateCharacterMianTreeItem::Sort_Child(
    const TSharedPtr<UCreateCharacterMianTreeItem>& In_L, const TSharedPtr<UCreateCharacterMianTreeItem>& In_R
) {
  return In_L->Get().Sort_Index < In_R->Get().Sort_Index;
  // if (*In_L && *In_R) {
  // } else {
  //   return *In_L ? *In_L : *In_R;
  // }
}

bool UCreateCharacterMianTreeItem::Sort_Child() {
  // using Sort_type = FSceneOutlinerSortHelper<std::int32_t>;

  // Sort_type{}.Primary();

  Childs.Sort(
      static_cast<
          bool (*)(const TSharedPtr<UCreateCharacterMianTreeItem>&, const TSharedPtr<UCreateCharacterMianTreeItem>&)>(
          &UCreateCharacterMianTreeItem::Sort_Child
      )
  );
  // Childs.HeapSort(static_cast<bool (*)(const TSharedPtr<UCreateCharacterMianTreeItem>&, const
  // TSharedPtr<UCreateCharacterMianTreeItem>&)>(&UCreateCharacterMianTreeItem::Sort_Child)); std::sort(Childs.begin(),
  // Childs.end(), static_cast<bool (*)(const TSharedPtr<UCreateCharacterMianTreeItem>&, const
  // TSharedPtr<UCreateCharacterMianTreeItem>&)>(&UCreateCharacterMianTreeItem::Sort_Child));
  // Childs.StableSort(static_cast<bool (*)(const TSharedPtr<UCreateCharacterMianTreeItem>&, const
  // TSharedPtr<UCreateCharacterMianTreeItem>&)>(&UCreateCharacterMianTreeItem::Sort_Child));
  return false;
}

class FSCreateCharacterBoneDragDropOp : public FDragDropOperation {
 public:
  DRAG_DROP_OPERATOR_TYPE(FSCreateCharacterBoneDragDropOp, FDragDropOperation)
  FString BoneName;
  int32 CurrentIndex{};
  int32 TargetIndex{};

  SCreateCharacterTree::TreeVirwWeightItemType ItemData;
  virtual TSharedPtr<SWidget> GetDefaultDecorator() const override {
    // clang-format off
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[		
				SNew(SHorizontalBox)
				//+SHorizontalBox::Slot()
				//.AutoWidth()
				//[
				//	SNew(SImage)
				//	.Image(this, &FSCreateCharacterBoneDragDropOp::GetIcon)
				//]

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock) 
					.Text( this, &FSCreateCharacterBoneDragDropOp::GetHoverText )
				]
			];

    // clang-format on
  }

  FText GetHoverText() const {
                return FText::Format(
                    NSLOCTEXT("BoneDragDropOp", "BoneHoverTextFmt", "Bone {0} {1} {2}"), FText::FromString(BoneName),
                    CurrentIndex, TargetIndex
                );
  }

  static TSharedRef<FSCreateCharacterBoneDragDropOp> New(
      SCreateCharacterTree::TreeVirwWeightItemType In_ItemData, const FString& InBoneName
  ) {
    TSharedPtr<FSCreateCharacterBoneDragDropOp> L_Drop = MakeShared<FSCreateCharacterBoneDragDropOp>();
    L_Drop->BoneName                                   = InBoneName;
    L_Drop->ItemData                                   = In_ItemData;
    L_Drop->CurrentIndex                               = In_ItemData->Get().Sort_Index;
    L_Drop->Construct();
    return L_Drop.ToSharedRef();
  }
};

class SCreateCharacterConfigTreeItem : public SMultiColumnTableRow<SCreateCharacterTree::TreeVirwWeightItemType> {
 public:
  using Super = SMultiColumnTableRow<SCreateCharacterTree::TreeVirwWeightItemType>;

  SLATE_BEGIN_ARGS(SCreateCharacterConfigTreeItem)
      : _ItemData(), _InConfig(), _OnEditItem(), _OnModifyWeights(), _OnRootSort() {}

  SLATE_ARGUMENT(SCreateCharacterTree::TreeVirwWeightItemType, ItemData)
  SLATE_ARGUMENT(UDoodleCreateCharacterConfig*, InConfig)
  SLATE_EVENT(FDoodleTreeEdit, OnEditItem)
  SLATE_EVENT(FDoodleTreeEdit, OnModifyWeights)
  SLATE_EVENT(FSimpleDelegate, OnRootSort)
  SLATE_END_ARGS()

  // 这里是内容创建函数
  void Construct(const FArguments& Arg, const TSharedRef<STableViewBase>& In_OwnerTableView) {
    ItemData        = Arg._ItemData;
    Config_Data     = Arg._InConfig;
    OnEditItem      = Arg._OnEditItem;
    OnModifyWeights = Arg._OnModifyWeights;
    OnRootSort      = Arg._OnRootSort;
    OwnerTableView  = In_OwnerTableView;
    Super::FArguments L_Arg{};
    L_Arg.OnDragDetected(this, &SCreateCharacterConfigTreeItem::My_OnDragDetected);

    Super::Construct(L_Arg, In_OwnerTableView);
  }

  TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override {
    if (InColumnName == SCreateCharacterTree::G_Name) {
      // 只是题头部件
      TSharedPtr<SHorizontalBox> L_Box = SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth()[SNew(SExpanderArrow, SharedThis(this)).ShouldDrawWires(true)];

      TSharedPtr<SInlineEditableTextBlock> L_InlineEditableTextBlock_Ptr =
          SNew(SInlineEditableTextBlock)
              //.ColorAndOpacity(this, &FSkeletonTreeVirtualBoneItem::GetBoneTextColor, InIsSelected)
              .Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SCreateCharacterConfigTreeItem::Get_ItemName)))
              //.HighlightText(FilterText)
              .Font(this, &SCreateCharacterConfigTreeItem::GetTextFont)
              //.ToolTipText(ToolTip)
              .OnEnterEditingMode(this, &SCreateCharacterConfigTreeItem::OnItemNameEditing)
              .OnVerifyTextChanged(this, &SCreateCharacterConfigTreeItem::OnVerifyItemNameChanged)
              .OnTextCommitted(this, &SCreateCharacterConfigTreeItem::OnCommitItemBoneName)
          //.IsSelected(InIsSelected)
          ;
      if (ItemData && !ItemData->OnRenameRequested.IsBound()) {
        ItemData->OnRenameRequested.BindSP(L_InlineEditableTextBlock_Ptr.Get(), &SInlineEditableTextBlock::EnterEditingMode);
      }
      L_Box->AddSlot().AutoWidth()[L_InlineEditableTextBlock_Ptr.ToSharedRef()];

      return L_Box.ToSharedRef();
    }

    if (InColumnName == SCreateCharacterTree::G_Value && ItemData && *ItemData && !ItemData->Get().Keys.IsEmpty()) {
      if (!ItemData->OnSetRange.IsBound())
        ItemData->OnSetRange.BindLambda([this]() { Slider->SetMinAndMaxValues(ItemData->Get().MinValue, ItemData->Get().MaxValue); });

      return SNew(SHorizontalBox)
             // clang-format off
               + SHorizontalBox::Slot().FillWidth(1.0f)
               [
                 SAssignNew(Slider, SSlider)
                 .Value_Lambda([this](){ return ItemData->Get().Value; })
                 .MaxValue(ItemData->Get().MaxValue)
                 .MinValue(ItemData->Get().MinValue)
                 .OnValueChanged(FOnFloatValueChanged::CreateSP(this, &SCreateCharacterConfigTreeItem::On_FloatValueChanged))
               ] 
               + SHorizontalBox::Slot()
               .AutoWidth()
               [
                 SNew(SButton)
                 .OnClicked_Lambda([this]() {
                   if (!ItemData)
                     return FReply::Unhandled();
                   OnEditItem.ExecuteIfBound(ItemData);
                   return FReply::Handled();
                 })
                   [
                     SNew(STextBlock)
                     .Text(LOCTEXT("SCreateCharacterConfigTreeItem_Edit", "Edit"))
                   ]
               ]
               + SHorizontalBox::Slot()
               .AutoWidth()
               [
                 SNew(SButton)
                 .OnClicked_Lambda([this]() {
                   if (!ItemData)
                     return FReply::Unhandled();
                   On_FloatValueChanged(0.0f);
                   return FReply::Handled();
                 })
                   [
                     SNew(STextBlock)
                     .Text(LOCTEXT("SCreateCharacterConfigTreeItem_Edit", "Rest"))
                   ]
               ]
          // clang-format on
          ;
    }

    return SNew(SHorizontalBox);
  }

  virtual int32 DoesItemHaveChildren() const override {
    if (!ItemData->Childs.IsEmpty()) return 1;

    return Super::DoesItemHaveChildren();
  };

  void On_FloatValueChanged(float In_Value) {
    ItemData->Get().Value = In_Value;
    if (ItemData)
      OnModifyWeights.ExecuteIfBound(ItemData);
    // UE_LOG(LogTemp, Warning, TEXT("FileManipulation: %f"), In_Value);
  }

  virtual bool IsItemExpanded() const override { return Super::IsItemExpanded() || !ItemData->Childs.IsEmpty(); };

  virtual void ToggleExpansion() override { Super::ToggleExpansion(); };

 private:
  void OnItemNameEditing() {
  }
  FText Get_ItemName() {
    return FText::FromString((ItemData && *ItemData && !ItemData->Get().ShowUIName.IsEmpty()) ? ItemData->Get().ShowUIName : FString{TEXT("None")});
  }

  bool OnVerifyItemNameChanged(const FText& InText, FText& OutErrorMessage) {
    bool bVerifyName      = true;

    FString InTextTrimmed = FText::TrimPrecedingAndTrailing(InText).ToString();

    // FString NewName       = VirtualBoneNameHelpers::AddVirtualBonePrefix(InTextTrimmed);

    if (InTextTrimmed.IsEmpty() || !ItemData) {
      OutErrorMessage = LOCTEXT("EmptyVirtualBoneName_Error", "Virtual bones must have a name!");
      bVerifyName     = false;
    } else {
      if (InTextTrimmed != ItemData->Get().ShowUIName) {
        // 判断是否存在
        bVerifyName = !(ItemData && Config_Data.IsValid()) ? Config_Data.Get()->Has_UI_ShowName(ItemData->Get_Index(), InTextTrimmed) : true;

        // Needs to be checked on verify.
        if (!bVerifyName) {
          // Tell the user that the name is a duplicate
          OutErrorMessage = LOCTEXT("DuplicateVirtualBone_Error", "Name in use!");
          bVerifyName     = false;
        }
      }
    }

    return bVerifyName;
  }

  void OnCommitItemBoneName(const FText& InText, ETextCommit::Type CommitInfo) {
    FString NewNameString = FText::TrimPrecedingAndTrailing(InText).ToString();

    // 通知所有到更改

    // if (!ItemData || !ItemData->ConfigNode || !Config_Data.IsValid())
    //   return;
    if (ItemData && *ItemData && Config_Data.IsValid())
      Config_Data.Get()->Rename_UI_ShowName(ItemData->Get_Index(), NewNameString);
  }
  FSlateFontInfo GetTextFont() const {
    return FAppStyle::GetWidgetStyle<FTextBlockStyle>("SkeletonTree.NormalFont").Font;
  }

  // DragBegin
  /////  当拖动进入一个小部件时在拖放过程中调用
  // void OnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override{};
  ///// 当拖动离开小部件时在拖放过程中调用
  // void OnDragLeave(const FDragDropEvent& InDragDropEvent) override{};
  ///// 当鼠标被拖动到小部件上时，在拖放过程中调用
  FReply OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override {
    if (auto L_Op = InDragDropEvent.GetOperationAs<FSCreateCharacterBoneDragDropOp>()) {
      if (L_Op->ItemData && *L_Op->ItemData) {
        L_Op->TargetIndex = ItemData->Get().Sort_Index;
        return FReply::Handled();
      }
    }

    return FReply::Unhandled();
  };
  /// 当用户把东西放到小部件上时被调用 终止拖放
  FReply OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) override {
    if (auto L_Op = InDragDropEvent.GetOperationAs<FSCreateCharacterBoneDragDropOp>()) {
      if (L_Op->ItemData && *L_Op->ItemData) {
        Swap(L_Op->ItemData->Get().Sort_Index, ItemData->Get().Sort_Index);
        Config_Data->MarkPackageDirty();
        if (!L_Op->ItemData->Parent.IsValid()) {
          OnRootSort.ExecuteIfBound();
        }
        OwnerTableView->RequestListRefresh();
        return FReply::Handled();
      }
    }

    return FReply::Unhandled();
  };
  FReply My_OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
    return FReply::Handled().BeginDragDrop(FSCreateCharacterBoneDragDropOp::New(ItemData, ItemData->Get().ShowUIName));
  }
  // DragEnd

  SCreateCharacterTree::TreeVirwWeightItemType ItemData;
  TWeakObjectPtr<UDoodleCreateCharacterConfig> Config_Data;
  FDoodleTreeEdit OnModifyWeights;
  TSharedPtr<STableViewBase> OwnerTableView;

  FDoodleTreeEdit OnEditItem;
  FSimpleDelegate OnRootSort;
  TSharedPtr<SSlider> Slider;
};

#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "SCreateCharacterTree"

const FName SCreateCharacterTree::Name{"Doodle_SCreateCharacterTree"};

const FName SCreateCharacterTree::G_Name{"Name"};
const FName SCreateCharacterTree::G_Value{"Value"};

void SCreateCharacterTree::Construct(const FArguments& Arg) {
  Config          = Arg._CreateCharacterConfig;
  OnEditItem      = Arg._OnEditItem;
  OnModifyWeights = Arg._OnModifyWeights;

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
                         //.OnSort(FOnSortModeChanged::CreateSP(this,&SCreateCharacterTree::My_OnSortModeChanged))
                         //.SortMode(EColumnSortMode::Ascending)
                         //.SortPriority(EColumnSortPriority::Max)
                         + SHeaderRow::Column(G_Value)
                         .DefaultLabel(LOCTEXT("Construct", "Value"))
              // clang-format on
          )
          .OnContextMenuOpening(FOnContextMenuOpening::CreateSP(this, &SCreateCharacterTree::Create_ContextMenuOpening))
          .OnSelectionChanged(
              TreeVirwWeightType::FOnSelectionChanged::CreateSP(this, &SCreateCharacterTree::On_SelectionChanged)
          )
          .OnMouseButtonDoubleClick(
              FOnMouseButtonDoubleClick::CreateSP(this, &SCreateCharacterTree::On_MouseButtonDoubleClick)
          )
  );
}

TSharedRef<SDockTab> SCreateCharacterTree::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterTree)];  //
}

TSharedRef<class ITableRow> SCreateCharacterTree::CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table) {
  return SNew(SCreateCharacterConfigTreeItem, In_Table)
      .ItemData(In_Value)
      .OnEditItem(OnEditItem)
      .OnModifyWeights(OnModifyWeights)
      .InConfig(Config.Get())
      .OnRootSort_Lambda([this]() { Sort_Root(); });
}

void SCreateCharacterTree::CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List) {
  In_Value->Sort_Child();

  // TArray<int32> L_Print_Value{};
  // for (auto&& i : In_Value->Childs) {
  //   L_Print_Value.Add(i->Get().Sort_Index);
  // }
  // auto TArrayConvertFString = [](TArray<int32> IntArray) {
  //   TArray<FString> StrArray;
  //   for (int32 i : IntArray) {
  //     StrArray.Push(FString::FromInt(i));
  //   };
  //   return StrArray;
  // };
  // auto L_String = FString::Join(TArrayConvertFString(L_Print_Value), TEXT(" "));
  // UE_LOG(LogTemp, Warning, TEXT("Sort: %s "), *L_String);
  In_List = In_Value->Childs;
}

TSharedPtr<SWidget> SCreateCharacterTree::Create_ContextMenuOpening() {
  FMenuBuilder L_Builder{true, UICommandList, Extender};

  {
    L_Builder.BeginSection("Create_ContextMenuOpening_Add_Bone", LOCTEXT("Create_ContextMenuOpening_Add_Bone1", "Add"));

    // 添加
    if (!CurrentSelect || (CurrentSelect && *CurrentSelect && CurrentSelect->Get().Keys.IsEmpty()))
      L_Builder.AddMenuEntry(
          LOCTEXT("Create_ContextMenuOpening_Add_Bone2", "Add Classify"),
          LOCTEXT("Create_ContextMenuOpening_Add_Bone2_Tip", "Add Classify"), FSlateIcon{"Subtitle", "EventIcon"},
          FUIAction{FExecuteAction::CreateLambda([this]() { AddBone(); })}
      );
    // 绑定骨骼
    if (CurrentSelect && *CurrentSelect && CurrentSelect->Get().Parent != INDEX_NONE && CurrentSelect->Get().Childs.IsEmpty()) {
      L_Builder.AddSubMenu(
          LOCTEXT("Create_ContextMenuOpening_Add_Bone4", "Binding"),
          LOCTEXT("Create_ContextMenuOpening_Add_Bone4_Tip", "Binding Bone"),
          FNewMenuDelegate::CreateSP(this, &SCreateCharacterTree::AddBoneTreeMenu)
      );
    }

    // 删除
    if (CurrentSelect && *CurrentSelect)
      L_Builder.AddMenuEntry(
          LOCTEXT("Create_ContextMenuOpening_Add_Bone3", "Remove"),
          LOCTEXT("Create_ContextMenuOpening_Add_Bone3_Tip", "Remove"), FSlateIcon{"Subtitle", "EventIcon"},
          FUIAction{FExecuteAction::CreateLambda([this]() { Delete_UiTreeNode(); })}
      );
    L_Builder.EndSection();
  }

  return L_Builder.MakeWidget();
}

void SCreateCharacterTree::On_SelectionChanged(TreeVirwWeightItemType TreeItem, ESelectInfo::Type SelectInfo) {
  CurrentSelect = TreeItem;
}

void SCreateCharacterTree::On_MouseButtonDoubleClick(TreeVirwWeightItemType TreeItem) {
  // UE_LOG(LogTemp, Warning, TEXT("Edit: %s"), *TreeItem->ShowName.ToString());
  TreeItem->OnRenameRequested.ExecuteIfBound();
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
                                                FSlateApplication::Get().DismissAllMenus();
                                                this->Add_TreeNode(In_BoneName);
                                              })
                                              .OnGetReferenceSkeleton_Lambda([L_Config]() -> const FReferenceSkeleton& {
                                                return L_Config->GetSkeletalMesh()->GetRefSkeleton();
                                              });

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
  // if (!CurrentSelect->Childs.IsEmpty())
  //   return;

  UDoodleCreateCharacterConfig* L_Config = Config.Get();
  if (!L_Config) return;

  TOptional<FGuid> L_Key = L_Config->Add_ConfigNode(
      In_Bone_Name, (CurrentSelect && *CurrentSelect) ? CurrentSelect->Get_Index() : INDEX_NONE
  );

  if (L_Key) {
    this->RebuildList();
    OnEditItem.ExecuteIfBound(CurrentSelect);
  }
}

void SCreateCharacterTree::Sort_Root() {
  CreateCharacterConfigTreeData.Sort(
      static_cast<
          bool (*)(const TSharedPtr<UCreateCharacterMianTreeItem>&, const TSharedPtr<UCreateCharacterMianTreeItem>&)>(
          &TreeVirwWeightItemType::ElementType::Sort_Child
      )
  );
}

void SCreateCharacterTree::My_OnSortModeChanged(
    EColumnSortPriority::Type In_Sort_Priority, const FName& In_Name, EColumnSortMode::Type In_Mode
) {
  if (In_Name == G_Name) {
    Sort_Root();
    for (auto&& i : CreateCharacterConfigTreeData) {
      i->Sort_Child();
    }
  }
  RequestListRefresh();
  // RebuildList();
}

void SCreateCharacterTree::CreateUITree() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config) return;

  TFunction<void(
      const SCreateCharacterTree::TreeVirwWeightItemType& InParent, UDoodleCreateCharacterConfig* InConfig,
      const TArray<int32>& InChildIndex
  )>
      L_Fun{};
  L_Fun = [&](const SCreateCharacterTree::TreeVirwWeightItemType& InParent, UDoodleCreateCharacterConfig* InConfig,
              const TArray<int32>& InChildIndex
          ) {
    for (auto i : InChildIndex) {
      auto& L_Nodes = InConfig->ListTrees[i];
      // if (L_Nodes.Childs.IsEmpty()) continue;

      SCreateCharacterTree::TreeVirwWeightItemType L_Ptr =
          InParent->Childs.Add_GetRef(MakeShared<TreeVirwWeightItemType::ElementType>(L_Config));
      // 添加子项
      L_Ptr->Set(i);
      L_Ptr->Parent = InParent;
      if (L_Nodes.Sort_Index == 0) {
        L_Nodes.Sort_Index = i;
      }
      L_Fun(L_Ptr, InConfig, L_Nodes.Childs);
    }
    InParent->Sort_Child();
  };

  CreateCharacterConfigTreeData.Empty(L_Config->ListConfigNode.Num());
  for (auto i = 0; i < L_Config->ListTrees.Num(); ++i) {
    auto&& L_Node = L_Config->ListTrees[i];
    if (L_Node.Parent != INDEX_NONE) {
      continue;
    }
    TreeVirwWeightItemType L_Ptr =
        CreateCharacterConfigTreeData.Add_GetRef(MakeShared<TreeVirwWeightItemType::ElementType>(L_Config));
    L_Ptr->Set(i);
    L_Fun(L_Ptr, L_Config, L_Node.Childs);
  }
  Sort_Root();
}

void SCreateCharacterTree::AddBone() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config)
    return;

  int32 L_UI_Node = L_Config->Add_TreeNode(
      (CurrentSelect && *CurrentSelect) ? CurrentSelect->Get_Index() : INDEX_NONE
  );

  if (L_UI_Node == INDEX_NONE)
    return;
  const bool L_Has_Parent{CurrentSelect};

  TreeVirwWeightItemType L_Ptr{
      L_Has_Parent ? CurrentSelect->Childs.Add_GetRef(MakeShared<TreeVirwWeightItemType::ElementType>(L_Config))
                   : MakeShared<TreeVirwWeightItemType::ElementType>(L_Config)};

  if (!L_Has_Parent) CreateCharacterConfigTreeData.Add(L_Ptr);

  L_Ptr->Set(L_UI_Node);
  L_Ptr->Parent = CurrentSelect;

  this->RequestTreeRefresh();
  if (!L_Has_Parent) {
    CreateUITree();
    this->RebuildList();
  }
}

void SCreateCharacterTree::Delete_UiTreeNode() {
  UDoodleCreateCharacterConfig* L_Config = Config.Get();

  if (!L_Config)
    return;

  if (!CurrentSelect)
    return;
  if (!*CurrentSelect) return;

  if (!L_Config->Delete_Ui_Node(CurrentSelect->Get_Index())) return;

  CreateUITree();
  this->RebuildList();
}

// void SCreateCharacterTree::OnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) {
// }
//
// void SCreateCharacterTree::OnDragLeave(const FDragDropEvent& InDragDropEvent) {
// }
//
// FReply SCreateCharacterTree::OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) {
//   return FReply::Handled();
// }
//
// FReply SCreateCharacterTree::OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) {
//   return FReply::Handled();
// }

#undef LOCTEXT_NAMESPACE