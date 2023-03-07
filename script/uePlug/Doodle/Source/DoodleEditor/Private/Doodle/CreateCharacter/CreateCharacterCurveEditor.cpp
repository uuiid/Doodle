#include "CreateCharacterCurveEditor.h"
#include "CurveEditor.h"
#include "RichCurveEditorModel.h"
#include "Animation/AnimSequenceBase.h"
#include "SCurveEditorPanel.h"
#include "Tree/SCurveEditorTreeTextFilter.h"
#include "Tree/SCurveEditorTreeFilterStatusBar.h"
#include "Tree/SCurveEditorTree.h"
#include "Tree/SCurveEditorTreeSelect.h"
#include "Tree/SCurveEditorTreePin.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Tree/ICurveEditorTreeItem.h"
#include "Tree/CurveEditorTreeFilter.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "SCreateCharacterCurveEditor"

class FCreateCharacterCurveEditorBounds : public ICurveEditorBounds {
 public:
  FCreateCharacterCurveEditorBounds() {}

  virtual void GetInputBounds(double& OutMin, double& OutMax) const override {
    // FAnimatedRange ViewRange = ExternalTimeSliderController.Pin()->GetViewRange();
    // OutMin                   = ViewRange.GetLowerBoundValue();
    // OutMax                   = ViewRange.GetUpperBoundValue();
  }

  virtual void SetInputBounds(double InMin, double InMax) override {
    // ExternalTimeSliderController.Pin()->SetViewRange(InMin, InMax, EViewRangeInterpolation::Immediate);
  }

  // TWeakPtr<ITimeSliderController> ExternalTimeSliderController;
};

class FRichCurveEditorModel_CreateCharacter : public FRichCurveEditorModel {
 public:
  FRichCurveEditorModel_CreateCharacter(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, UAnimSequenceBase* InAnimSequence, FCurveEditorTreeItemID InTreeId = FCurveEditorTreeItemID());

  virtual ~FRichCurveEditorModel_CreateCharacter();

  virtual bool IsValid() const override;
  virtual FRichCurve& GetRichCurve() override;
  virtual const FRichCurve& GetReadOnlyRichCurve() const override;

  virtual void SetKeyPositions(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyPosition> InKeyPositions, EPropertyChangeType::Type ChangeType) override;
  virtual void SetKeyAttributes(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyAttributes> InAttributes, EPropertyChangeType::Type ChangeType = EPropertyChangeType::Unspecified) override;
  virtual void SetCurveAttributes(const FCurveAttributes& InCurveAttributes) override;

  void CurveHasChanged();
  void OnModelHasChanged(const EAnimDataModelNotifyType& NotifyType, UAnimDataModel* Model, const FAnimDataModelNotifPayload& Payload);
  void UpdateCachedCurve();

  FSmartName Name;
  TWeakObjectPtr<UAnimSequenceBase> AnimSequence;
  int32 CurveIndex;
  ERawCurveTrackTypes Type;
  FCurveEditorTreeItemID TreeId;

  FAnimationCurveIdentifier CurveId;
  UE::Anim::FAnimDataModelNotifyCollector NotifyCollector;
  FRichCurve CachedCurve;
  bool bCurveRemoved;

  TUniquePtr<IAnimationDataController::FScopedBracket> InteractiveBracket;
};

FRichCurveEditorModel_CreateCharacter::FRichCurveEditorModel_CreateCharacter(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, UAnimSequenceBase* InAnimSequence, FCurveEditorTreeItemID InTreeId /*= FCurveEditorTreeItemID()*/)
    : FRichCurveEditorModel(InAnimSequence), Name(InName), AnimSequence(InAnimSequence), CurveIndex(InCurveIndex), Type(InType), TreeId(InTreeId), CurveId(FAnimationCurveIdentifier(Name, Type)), bCurveRemoved(false) {
  CurveModifiedDelegate.AddRaw(this, &FRichCurveEditorModel_CreateCharacter::CurveHasChanged);

  InAnimSequence->GetDataModel()->GetModifiedEvent().AddRaw(this, &FRichCurveEditorModel_CreateCharacter::OnModelHasChanged);

  if (Type == ERawCurveTrackTypes::RCT_Transform) {
    UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(CurveId, (ETransformCurveChannel)(CurveIndex / 3), (EVectorCurveChannel)(CurveIndex % 3));
  }

  UpdateCachedCurve();
}

FRichCurveEditorModel_CreateCharacter::~FRichCurveEditorModel_CreateCharacter() {
  AnimSequence->GetDataModel()->GetModifiedEvent().RemoveAll(this);
}

bool FRichCurveEditorModel_CreateCharacter::IsValid() const {
  return AnimSequence->GetDataModel()->FindCurve(FAnimationCurveIdentifier(Name, Type)) != nullptr;
}

FRichCurve& FRichCurveEditorModel_CreateCharacter::GetRichCurve() {
  check(AnimSequence.Get() != nullptr);
  return CachedCurve;
}

const FRichCurve& FRichCurveEditorModel_CreateCharacter::GetReadOnlyRichCurve() const {
  return const_cast<FRichCurveEditorModel_CreateCharacter*>(this)->GetRichCurve();
}

void FRichCurveEditorModel_CreateCharacter::SetKeyPositions(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyPosition> InKeyPositions, EPropertyChangeType::Type ChangeType) {
  const bool bInteractiveChange = ChangeType == EPropertyChangeType::Interactive;

  // Open bracket in case this is an interactive change
  if (bInteractiveChange && !InteractiveBracket.IsValid()) {
    IAnimationDataController& Controller = AnimSequence->GetController();
    InteractiveBracket                   = MakeUnique<IAnimationDataController::FScopedBracket>(Controller, LOCTEXT("SetKeyPositions", "Set Key Positions"));
  }

  FRichCurveEditorModel::SetKeyPositions(InKeys, InKeyPositions, ChangeType);

  // Close bracket, if open, in case this is was a non-interactive change
  if (!bInteractiveChange && InteractiveBracket.IsValid()) {
    InteractiveBracket.Reset();
  }
}

void FRichCurveEditorModel_CreateCharacter::SetKeyAttributes(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyAttributes> InAttributes, EPropertyChangeType::Type ChangeType) {
  const bool bInteractiveChange = ChangeType == EPropertyChangeType::Interactive;

  // Open bracket in case this is an interactive change
  if (bInteractiveChange && !InteractiveBracket.IsValid()) {
    IAnimationDataController& Controller = AnimSequence->GetController();
    InteractiveBracket                   = MakeUnique<IAnimationDataController::FScopedBracket>(Controller, LOCTEXT("SetKeyAttributes", "Set Key Attributes"));
  }

  FRichCurveEditorModel::SetKeyAttributes(InKeys, InAttributes, ChangeType);
  // Close bracket, if open, in case this is was a non-interactive change
  if (!bInteractiveChange && InteractiveBracket.IsValid()) {
    InteractiveBracket.Reset();
  }
}

void FRichCurveEditorModel_CreateCharacter::SetCurveAttributes(const FCurveAttributes& InCurveAttributes) {
  Modify();
  IAnimationDataController& Controller = AnimSequence->GetController();
  Controller.SetCurveAttributes(CurveId, InCurveAttributes);
}

void FRichCurveEditorModel_CreateCharacter::CurveHasChanged() {
  IAnimationDataController& Controller = AnimSequence->GetController();

  switch (Type) {
    case ERawCurveTrackTypes::RCT_Vector: {
      ensure(false);
      break;
    }
    case ERawCurveTrackTypes::RCT_Transform:
    case ERawCurveTrackTypes::RCT_Float: {
      Controller.SetCurveKeys(CurveId, CachedCurve.GetConstRefOfKeys());
      break;
    }
  }
}

void FRichCurveEditorModel_CreateCharacter::OnModelHasChanged(const EAnimDataModelNotifyType& NotifyType, UAnimDataModel* Model, const FAnimDataModelNotifPayload& Payload) {
  NotifyCollector.Handle(NotifyType);

  switch (NotifyType) {
    case EAnimDataModelNotifyType::CurveAdded:
    case EAnimDataModelNotifyType::CurveChanged:
    case EAnimDataModelNotifyType::CurveFlagsChanged:
    case EAnimDataModelNotifyType::CurveScaled: {
      const FCurvePayload& TypedPayload = Payload.GetPayload<FCurvePayload>();
      if (TypedPayload.Identifier.InternalName == Name) {
        if (NotifyCollector.IsNotWithinBracket()) {
          UpdateCachedCurve();
        } else {
          // Curve was re-added after removal in same bracket
          if (bCurveRemoved && NotifyType == EAnimDataModelNotifyType::CurveAdded) {
            bCurveRemoved = false;
          }
        }
      }

      break;
    }

    case EAnimDataModelNotifyType::CurveRemoved: {
      // Curve was removed
      const FCurveRemovedPayload& TypedPayload = Payload.GetPayload<FCurveRemovedPayload>();
      if (TypedPayload.Identifier.InternalName == Name) {
        bCurveRemoved = true;
      }
      break;
    }

    case EAnimDataModelNotifyType::CurveRenamed: {
      const FCurveRenamedPayload& TypedPayload = Payload.GetPayload<FCurveRenamedPayload>();
      if (TypedPayload.Identifier == CurveId) {
        Name    = TypedPayload.NewIdentifier.InternalName;
        CurveId = TypedPayload.NewIdentifier;

        if (NotifyCollector.IsNotWithinBracket()) {
          UpdateCachedCurve();
        }
      }

      break;
    }

    case EAnimDataModelNotifyType::BracketClosed: {
      if (NotifyCollector.IsNotWithinBracket()) {
        if (!bCurveRemoved && NotifyCollector.Contains({EAnimDataModelNotifyType::CurveAdded, EAnimDataModelNotifyType::CurveChanged, EAnimDataModelNotifyType::CurveFlagsChanged, EAnimDataModelNotifyType::CurveScaled, EAnimDataModelNotifyType::CurveRenamed})) {
          UpdateCachedCurve();
        }
      }
      break;
    }
  }
}

void FRichCurveEditorModel_CreateCharacter::UpdateCachedCurve() {
  const FAnimCurveBase* CurveBase = AnimSequence->GetDataModel()->FindCurve(CurveId);

  check(CurveBase);  // If this fails lifetime contracts have been violated - this curve should always be present if this model exists

  const FRichCurve* CurveToCopyFrom = [this, CurveBase]() -> const FRichCurve* {
    switch (Type) {
      case ERawCurveTrackTypes::RCT_Vector: {
        ensure(false);
        const FVectorCurve& VectorCurve = *(static_cast<const FVectorCurve*>(CurveBase));
        check(CurveIndex < 3);
        return &VectorCurve.FloatCurves[CurveIndex];
      }
      case ERawCurveTrackTypes::RCT_Transform: {
        const FTransformCurve& TransformCurve = *(static_cast<const FTransformCurve*>(CurveBase));
        check(CurveIndex < 9);
        const int32 SubCurveIndex = CurveIndex % 3;
        switch (CurveIndex) {
          default:
            check(false);
            // fall through
          case 0:
          case 1:
          case 2:
            return &TransformCurve.TranslationCurve.FloatCurves[SubCurveIndex];
          case 3:
          case 4:
          case 5:
            return &TransformCurve.RotationCurve.FloatCurves[SubCurveIndex];
          case 6:
          case 7:
          case 8:
            return &TransformCurve.ScaleCurve.FloatCurves[SubCurveIndex];
        }
      }
      case ERawCurveTrackTypes::RCT_Float:
      default: {
        const FFloatCurve& FloatCurve = *(static_cast<const FFloatCurve*>(CurveBase));
        check(CurveIndex == 0);
        return &FloatCurve.FloatCurve;
      }
    }

    return nullptr;
  }();

  if (ensure(CurveToCopyFrom)) {
    CachedCurve = *CurveToCopyFrom;
  }
}

class FCreateCharacterCurveEditorItem : public ICurveEditorTreeItem {
 public:
  FCreateCharacterCurveEditorItem(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, UAnimSequenceBase* InAnimSequence, const FText& InCurveDisplayName, const FLinearColor& InCurveColor, FSimpleDelegate InOnCurveModified, FCurveEditorTreeItemID InTreeId)
      : Name(InName), Type(InType), CurveIndex(InCurveIndex), AnimSequence(InAnimSequence), CurveDisplayName(InCurveDisplayName), CurveColor(InCurveColor), OnCurveModified(InOnCurveModified), TreeId(InTreeId) {
  }

  virtual TSharedPtr<SWidget> GenerateCurveEditorTreeWidget(const FName& InColumnName, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID, const TSharedRef<ITableRow>& InTableRow) override {
    if (InColumnName == ColumnNames.Label) {
      return SNew(SHorizontalBox) + SHorizontalBox::Slot()
                                        .Padding(FMargin(4.f))
                                        .VAlign(VAlign_Center)
                                        .HAlign(HAlign_Right)
                                        .AutoWidth()
                                            [SNew(STextBlock)
                                                 .Text(CurveDisplayName)
                                                 .ColorAndOpacity(FSlateColor(CurveColor))];
    } else if (InColumnName == ColumnNames.SelectHeader) {
      return SNew(SCurveEditorTreeSelect, InCurveEditor, InTreeItemID, InTableRow);
    } else if (InColumnName == ColumnNames.PinHeader) {
      return SNew(SCurveEditorTreePin, InCurveEditor, InTreeItemID, InTableRow);
    }

    return nullptr;
  }

  virtual void CreateCurveModels(TArray<TUniquePtr<FCurveModel>>& OutCurveModels) override {
    TUniquePtr<FRichCurveEditorModel_CreateCharacter> NewCurveModel = MakeUnique<FRichCurveEditorModel_CreateCharacter>(Name, Type, CurveIndex, AnimSequence.Get(), TreeId);
    NewCurveModel->SetShortDisplayName(CurveDisplayName);
    NewCurveModel->SetLongDisplayName(CurveDisplayName);
    NewCurveModel->SetColor(CurveColor);
    NewCurveModel->OnCurveModified().Add(OnCurveModified);

    OutCurveModels.Add(MoveTemp(NewCurveModel));
  }

  virtual bool PassesFilter(const FCurveEditorTreeFilter* InFilter) const override {
    if (InFilter->GetType() == ECurveEditorTreeFilterType::Text) {
      const FCurveEditorTreeTextFilter* Filter = static_cast<const FCurveEditorTreeTextFilter*>(InFilter);
      for (const FCurveEditorTreeTextFilterTerm& Term : Filter->GetTerms()) {
        for (const FCurveEditorTreeTextFilterToken& Token : Term.ChildToParentTokens) {
          if (Token.Match(*CurveDisplayName.ToString())) {
            return true;
          }
        }
      }

      return false;
    }

    return false;
  }

  FSmartName Name;
  ERawCurveTrackTypes Type;
  int32 CurveIndex;
  FText CurveDisplayName;
  FLinearColor CurveColor;
  FSimpleDelegate OnCurveModified;
  TWeakObjectPtr<UAnimSequenceBase> AnimSequence;
  FCurveEditorTreeItemID TreeId;
};

void SCreateCharacterCurveEditor::Construct(const FArguments& InArgs) {
  CurveEditor                                = MakeShared<FCurveEditor>();
  CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}s");
  CurveEditor->SetBounds(MakeUnique<FCreateCharacterCurveEditorBounds>(/*InArgs._ExternalTimeSliderController*/));

  FCurveEditorInitParams CurveEditorInitParams;
  CurveEditor->InitCurveEditor(CurveEditorInitParams);
  CurveEditor->InputSnapRateAttribute = FFrameRate{1, 25};

  CurveEditorTree                     = SNew(SCurveEditorTree, CurveEditor)
                        .OnContextMenuOpening(this, &SCreateCharacterCurveEditor::OnContextMenuOpening);

  TSharedRef<SCurveEditorPanel> CurveEditorPanel =
      SNew(SCurveEditorPanel, CurveEditor.ToSharedRef())
          .GridLineTint(FLinearColor(0.f, 0.f, 0.f, 0.3f))
          .ExternalTimeSliderController(InArgs._ExternalTimeSliderController)
          .TabManager(InArgs._TabManager)
          .TreeSplitterWidth(0.2f)
          .ContentSplitterWidth(0.8f)
          .TreeContent()
              // clang-format off
           [
             SNew(SVerticalBox) 
             + SVerticalBox::Slot()
               .AutoHeight()
                [
                  SAssignNew(CurveEditorSearchBox, SCurveEditorTreeTextFilter, CurveEditor)
                ] 
             + SVerticalBox::Slot()
               [
                 SNew(SScrollBorder, CurveEditorTree.ToSharedRef())
                 [
                   CurveEditorTree.ToSharedRef()
                 ]
               ] 
             + SVerticalBox::Slot()
               .AutoHeight()
               [
                 SNew(SCurveEditorTreeFilterStatusBar, CurveEditor)
               ] 
           ];
  // clang-format on

  ChildSlot
      // clang-format off
  [
    SNew(SVerticalBox) 
    + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 3.0f)[MakeToolbar(CurveEditorPanel)] 
    + SVerticalBox::Slot().FillHeight(1.0f)[CurveEditorPanel]
  ];
  // clang-format on
}

void SCreateCharacterCurveEditor::ResetCurves() {
  CurveEditor->RemoveAllTreeItems();
  CurveEditor->RemoveAllCurves();
}

void SCreateCharacterCurveEditor::AddCurve(const FText& InCurveDisplayName, const FLinearColor& InCurveColor, const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, FSimpleDelegate InOnCurveModified) {
  // Ensure that curve is not already being edited
  for (const TPair<FCurveModelID, TUniquePtr<FCurveModel>>& CurvePair : CurveEditor->GetCurves()) {
    const TUniquePtr<FCurveModel>& Model = CurvePair.Value;
  }

  FCurveEditorTreeItem* TreeItem = CurveEditor->AddTreeItem(FCurveEditorTreeItemID());
  TreeItem->SetStrongItem(MakeShared<FCreateCharacterCurveEditorItem>(InName, InType, InCurveIndex, nullptr /*AnimSequence*/, InCurveDisplayName, InCurveColor, InOnCurveModified, TreeItem->GetID()));

  // Update selection
  const TMap<FCurveEditorTreeItemID, ECurveEditorTreeSelectionState>& Selection = CurveEditor->GetTreeSelection();
  TArray<FCurveEditorTreeItemID> NewSelection;
  NewSelection.Add(TreeItem->GetID());
  for (const auto& SelectionPair : Selection) {
    if (SelectionPair.Value != ECurveEditorTreeSelectionState::None) {
      NewSelection.Add(SelectionPair.Key);
    }
  }
  CurveEditor->SetTreeSelection(MoveTemp(NewSelection));
}

void SCreateCharacterCurveEditor::RemoveCurve(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex) {
  for (const auto& CurvePair : CurveEditor->GetCurves()) {
    const TUniquePtr<FCurveModel>& Model = CurvePair.Value;
    // Cache ID to prevent use after release
    const FCurveEditorTreeItemID TreeId;
    CurveEditor->RemoveCurve(CurvePair.Key);
    CurveEditor->RemoveTreeItem(TreeId);
    break;
  }
}

void SCreateCharacterCurveEditor::ZoomToFit() {
  CurveEditor->ZoomToFit(EAxisList::Y);
}

TSharedRef<SWidget> SCreateCharacterCurveEditor::MakeToolbar(TSharedRef<SCurveEditorPanel> InEditorPanel) {
  FToolBarBuilder ToolBarBuilder(InEditorPanel->GetCommands(), FMultiBoxCustomization::None, InEditorPanel->GetToolbarExtender(), true);
  ToolBarBuilder.SetStyle(&FAppStyle::Get(), "Sequencer.ToolBar");
  ToolBarBuilder.BeginSection("Asset");
  ToolBarBuilder.EndSection();
  // 我们只是把所有的扩展器作为我们的工具条，我们没有必要创建一个单独的工具条。
  return ToolBarBuilder.MakeWidget();
}

TSharedPtr<SWidget> SCreateCharacterCurveEditor::OnContextMenuOpening() {
  const TArray<FCurveEditorTreeItemID>& Selection = CurveEditorTree->GetSelectedItems();
  if (Selection.Num()) {
    UToolMenus* ToolMenus       = UToolMenus::Get();
    static const FName MenuName = "SAnimSequenceCurveEditor.CurveEditorTreeContextMenu";
    if (!ToolMenus->IsMenuRegistered(MenuName)) {
      ToolMenus->RegisterMenu(MenuName);
    }

    FToolMenuContext Context;
    UToolMenu* Menu           = ToolMenus->GenerateMenu(MenuName, Context);

    FToolMenuSection& Section = Menu->AddSection("Selection", LOCTEXT("SelectionLablel", "Selection"));
    Section.AddMenuEntry(
        "RemoveSelectedCurves",
        LOCTEXT("RemoveCurveLabel", "Stop editing selected curve(s)"),
        LOCTEXT("RemoveCurveTooltip", "Removes the currently selected curve(s) from editing"),
        FSlateIcon{},
        FToolUIActionChoice{
            FExecuteAction::CreateLambda([this]() {
              // Remove all selected tree items, and associated curves
              TArray<FCurveModelID> ModelIDs;
              TArray<FCurveEditorTreeItemID> Selection = CurveEditorTree->GetSelectedItems();
              for (const FCurveEditorTreeItemID& SelectedItem : Selection) {
                ModelIDs.Append(CurveEditor->GetTreeItem(SelectedItem).GetCurves());
                CurveEditor->RemoveTreeItem(SelectedItem);
              }
              CurveEditorTree->ClearSelection();

              for (const FCurveModelID& ID : ModelIDs) {
                CurveEditor->RemoveCurve(ID);
              }
            }
            )

        }
    );

    return ToolMenus->GenerateWidget(Menu);
  }
  return SNullWidget::NullWidget;
}
#undef LOCTEXT_NAMESPACE