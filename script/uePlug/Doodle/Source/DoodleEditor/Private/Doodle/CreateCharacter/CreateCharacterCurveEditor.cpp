#include "CreateCharacterCurveEditor.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimSequenceBase.h"
#include "CurveEditor.h"
#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RichCurveEditorModel.h"
#include "SCurveEditorPanel.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Tree/CurveEditorTreeFilter.h"
#include "Tree/ICurveEditorTreeItem.h"
#include "Tree/SCurveEditorTree.h"
#include "Tree/SCurveEditorTreeFilterStatusBar.h"
#include "Tree/SCurveEditorTreePin.h"
#include "Tree/SCurveEditorTreeSelect.h"
#include "Tree/SCurveEditorTreeTextFilter.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "CreateCharacterTree.h"
#include "FrameNumberNumericInterface.h"
#include "Preferences/PersonaOptions.h"

#include "CreateCharacterSliderController.h"

#include "CreateCharacterTree.h"

#define LOCTEXT_NAMESPACE "SCreateCharacterCurveEditor"

class FCreateCharacterCurveEditorBounds : public ICurveEditorBounds {
 public:
  FCreateCharacterCurveEditorBounds() {}

  virtual void GetInputBounds(double& OutMin, double& OutMax) const override {
    // FAnimatedRange ViewRange = ExternalTimeSliderController.Pin()->GetViewRange();
    // OutMin                   = ViewRange.GetLowerBoundValue();
    // OutMax                   = ViewRange.GetUpperBoundValue();
    OutMax = Max_P;
    OutMin = Min_P;
  }

  virtual void SetInputBounds(double InMin, double InMax) override {
    if (CreateCharacterMianTreeItem.IsValid()) {
      TSharedPtr<UCreateCharacterMianTreeItem> L_Config = CreateCharacterMianTreeItem.Pin();
      L_Config->Get().MaxValue                          = InMax;
      L_Config->Get().MinValue                          = InMin;

      Min_P                                             = InMin;
      Max_P                                             = InMax;
    }
    // ExternalTimeSliderController.Pin()->SetViewRange(InMin, InMax, EViewRangeInterpolation::Immediate);
  }

  void Set_TreeItem(const TSharedPtr<UCreateCharacterMianTreeItem>& In_Work) {
    CreateCharacterMianTreeItem = In_Work;
    Min_P                       = In_Work->Get().MinValue;
    Max_P                       = In_Work->Get().MaxValue;
  }

  TWeakPtr<UCreateCharacterMianTreeItem> CreateCharacterMianTreeItem;
  double Min_P;
  double Max_P;
  // TWeakPtr<ITimeSliderController> ExternalTimeSliderController;
};

class FCreateCharacterCurveEditorItem : public ICurveEditorTreeItem {
 public:
  FCreateCharacterCurveEditorItem(
      FRichCurveEditInfo InCurve, UDoodleCreateCharacterConfig* In_Config, FLinearColor In_Color = FLinearColor{}
  )
      : CreateCharacterConfig(In_Config),
        Curve_Data(InCurve),
        Curve_Color(In_Color),
        CurveName(FText::FromName(Curve_Data.CurveName)) {}

  virtual TSharedPtr<SWidget> GenerateCurveEditorTreeWidget(
      const FName& InColumnName, TWeakPtr<FCurveEditor> InCurveEditor, FCurveEditorTreeItemID InTreeItemID,
      const TSharedRef<ITableRow>& InTableRow
  ) override {
    if (InColumnName == ColumnNames.Label) {
      return SNew(SHorizontalBox) +
             SHorizontalBox::Slot()
                 .Padding(FMargin(4.f))
                 .VAlign(VAlign_Center)
                 .HAlign(HAlign_Right)
                 .AutoWidth()[SNew(STextBlock).Text(CurveName).ColorAndOpacity(FSlateColor(Curve_Color))];
    } else if (InColumnName == ColumnNames.SelectHeader) {
      return SNew(SCurveEditorTreeSelect, InCurveEditor, InTreeItemID, InTableRow);
    } else if (InColumnName == ColumnNames.PinHeader) {
      return SNew(SCurveEditorTreePin, InCurveEditor, InTreeItemID, InTableRow);
    }

    return nullptr;
  }

  virtual void CreateCurveModels(TArray<TUniquePtr<FCurveModel>>& OutCurveModels) override {
    if (!CreateCharacterConfig.IsValid()) return;

    TUniquePtr<FRichCurveEditorModelRaw> NewCurveModel =
        MakeUnique<FRichCurveEditorModelRaw>(static_cast<FRichCurve*>(Curve_Data.CurveToEdit), CreateCharacterConfig.Get());

    NewCurveModel->SetLongDisplayName(CurveName);
    NewCurveModel->SetColor(Curve_Color);

    OutCurveModels.Add(MoveTemp(NewCurveModel));
  }

  virtual bool PassesFilter(const FCurveEditorTreeFilter* InFilter) const override {
    if (InFilter->GetType() == ECurveEditorTreeFilterType::Text) {
      const FCurveEditorTreeTextFilter* Filter = static_cast<const FCurveEditorTreeTextFilter*>(InFilter);
      for (const FCurveEditorTreeTextFilterTerm& Term : Filter->GetTerms()) {
        for (const FCurveEditorTreeTextFilterToken& Token : Term.ChildToParentTokens) {
          if (Token.Match(*CurveName.ToString())) {
            return true;
          }
        }
      }

      return false;
    }

    return false;
  }

  // int32 CurveIndex;
  TWeakObjectPtr<UDoodleCreateCharacterConfig> CreateCharacterConfig;
  FRichCurveEditInfo Curve_Data;
  FLinearColor Curve_Color;
  FText CurveName;
};

void SCreateCharacterCurveEditor::Construct(const FArguments& InArgs) {
  CreateCharacterConfigConfig                            = InArgs._CreateCharacterConfigConfig;

  TAttribute<EFrameNumberDisplayFormats> DisplayFormat   = MakeAttributeLambda([]() {
    return GetDefault<UPersonaOptions>()->TimelineDisplayFormat;
  });

  TAttribute<FFrameRate> TickResolution                  = MakeAttributeLambda([]() {
    return FFrameRate{25, 1};
  });

  TAttribute<FFrameRate> DisplayRate                     = MakeAttributeLambda([]() {
    return FFrameRate{25, 1};
  });

  TSharedPtr<FFrameNumberInterface> NumericTypeInterface = MakeShareable(new FFrameNumberInterface{EFrameNumberDisplayFormats::Seconds, 0, TickResolution, DisplayRate});
  TAttribute<FAnimatedRange> ViewRange                   = MakeAttributeLambda([this]() {
    if (*this->CurrentSelect) {
      return FAnimatedRange{this->CurrentSelect->Get().MinValue, this->CurrentSelect->Get().MaxValue};
    } else {
      return FAnimatedRange{-2.0f, 2.0f};
    }
  });

  FTimeSliderArgs TimeSliderArgs;
  {
    TimeSliderArgs.ScrubPosition         = MakeAttributeLambda([]() { return FFrameTime(0); });
    TimeSliderArgs.ViewRange             = ViewRange;
    TimeSliderArgs.PlaybackRange         = MakeAttributeLambda([]() { return TRange<FFrameNumber>(0, 0); });
    TimeSliderArgs.ClampRange            = MakeAttributeLambda([]() { return FAnimatedRange(0.0, 0.0); });
    TimeSliderArgs.DisplayRate           = FFrameRate{27857, 1};
    TimeSliderArgs.TickResolution        = FFrameRate{25, 1};
    // TimeSliderArgs.OnViewRangeChanged     = FOnViewRangeChanged::CreateSP(&InModel.Get(), &FAnimModel::HandleViewRangeChanged);
    // TimeSliderArgs.OnClampRangeChanged    = FOnTimeRangeChanged::CreateSP(&InModel.Get(), &FAnimModel::HandleWorkingRangeChanged);
    TimeSliderArgs.IsPlaybackRangeLocked = true;
    TimeSliderArgs.PlaybackStatus        = EMovieScenePlayerStatus::Stopped;
    TimeSliderArgs.NumericTypeInterface  = NumericTypeInterface;
    // TimeSliderArgs.OnScrubPositionChanged = FOnScrubPositionChanged::CreateSP(this, &SAnimTimeline::HandleScrubPositionChanged);
  }

  auto CreateCharacterSliderController = MakeShared<FCreateCharacterSliderController>(TimeSliderArgs);

  CurveEditor                          = MakeShared<FCurveEditor>();
  FCurveEditorInitParams CurveEditorInitParams;
  CurveEditor->InitCurveEditor(CurveEditorInitParams);
  CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}s");
  CurveEditor->SetBounds(MakeUnique<FCreateCharacterCurveEditorBounds>(/*InArgs._ExternalTimeSliderController*/));

  // CurveEditor->InputSnapRateAttribute = FFrameRate{1, 25};

  CurveEditorTree = SNew(SCurveEditorTree, CurveEditor)
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

void SCreateCharacterCurveEditor::EditCurve(const TSharedPtr<UCreateCharacterMianTreeItem>& In_Node) {
  if (!CreateCharacterConfigConfig)
    return;

  ResetCurves();

  CurrentSelect = In_Node;

  for (auto&& L_Key : In_Node->Get().Keys) {
    auto& L_Tran      = CreateCharacterConfigConfig->ListConfigNode[L_Key].WeightCurve.TranslationCurve;
    auto& L_Size      = CreateCharacterConfigConfig->ListConfigNode[L_Key].WeightCurve.ScaleCurve;
    auto& L_Rot       = CreateCharacterConfigConfig->ListConfigNode[L_Key].WeightCurve.RotationCurve;
    FString Bone_Name = CreateCharacterConfigConfig->ListConfigNode[L_Key].BoneName.ToString();

    FVectorCurve::EIndex::X;

#define DOODLE_ADD_CURVE_IMPL(Owner, Index)                                                                                                                                                 \
  {                                                                                                                                                                                         \
    FRichCurveEditInfo L_Info{&Owner.FloatCurves[(int32)FVectorCurve::EIndex::Index], FName{Bone_Name + "." + Owner.Name.DisplayName.ToString() + TEXT(PREPROCESSOR_TO_STRING(.##Index))}}; \
    AddCurve(L_Info);                                                                                                                                                                       \
  }

#define DOODLE_ADD_CURVE(Owner)    \
  DOODLE_ADD_CURVE_IMPL(Owner, X); \
  DOODLE_ADD_CURVE_IMPL(Owner, Y); \
  DOODLE_ADD_CURVE_IMPL(Owner, Z);

    DOODLE_ADD_CURVE(L_Tran);
    DOODLE_ADD_CURVE(L_Size);
    DOODLE_ADD_CURVE(L_Rot);

#undef DOODLE_ADD_CURVE
#undef DOODLE_ADD_CURVE_IMPL
  }

  static_cast<FCreateCharacterCurveEditorBounds&>(CurveEditor->GetBounds()).Set_TreeItem(In_Node);
}

void SCreateCharacterCurveEditor::ResetCurves() {
  CurveEditor->RemoveAllTreeItems();
  CurveEditor->RemoveAllCurves();
}

void SCreateCharacterCurveEditor::AddCurve(
    const FRichCurveEditInfo& In_Info
) {
  FCurveEditorTreeItem* TreeItem = CurveEditor->AddTreeItem(FCurveEditorTreeItemID::Invalid());

  TreeItem->SetStrongItem(MakeShared<FCreateCharacterCurveEditorItem>(
      In_Info, CreateCharacterConfigConfig, FLinearColor::MakeFromHSV8((uint8)(FRandomStream{In_Info.CurveName}.FRand() * 255.0f), (uint8)196, (uint8)196)
  )
  );

  // Update selection
  // const TMap<FCurveEditorTreeItemID, ECurveEditorTreeSelectionState>& Selection = CurveEditor->GetTreeSelection();
  // TArray<FCurveEditorTreeItemID> NewSelection;
  // NewSelection.Add(TreeItem->GetID());
  // for (const auto& SelectionPair : Selection) {
  //  if (SelectionPair.Value != ECurveEditorTreeSelectionState::None) {
  //    NewSelection.Add(SelectionPair.Key);
  //  }
  //}
  // CurveEditor->SetTreeSelection(MoveTemp(NewSelection));
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