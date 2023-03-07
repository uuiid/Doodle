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

void SCreateCharacterCurveEditor::Construct(const FArguments& InArgs) {
  CurveEditor                                = MakeShared<FCurveEditor>();
  CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}s");
  CurveEditor->SetBounds(MakeUnique<FCreateCharacterCurveEditorBounds>(InArgs._ExternalTimeSliderController));

  FCurveEditorInitParams CurveEditorInitParams;
  CurveEditor->InitCurveEditor(CurveEditorInitParams);
  CurveEditor->InputSnapRateAttribute = FFrameRate{1, 25};

  AnimSequence->GetDataModel()->GetModifiedEvent().AddRaw(this, &SCreateCharacterCurveEditor::OnModelHasChanged);

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

void SCreateCharacterCurveEditor::ResetCurves() {
}

void SCreateCharacterCurveEditor::AddCurve(const FText& InCurveDisplayName, const FLinearColor& InCurveColor, const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, FSimpleDelegate InOnCurveModified) {
}

void SCreateCharacterCurveEditor::RemoveCurve(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex) {
}

void SCreateCharacterCurveEditor::ZoomToFit() {
}

void SCreateCharacterCurveEditor::OnModelHasChanged(const EAnimDataModelNotifyType& NotifyType, UAnimDataModel* Model, const FAnimDataModelNotifPayload& Payload) {
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
  return SNullWidget::NullWidget;
}
