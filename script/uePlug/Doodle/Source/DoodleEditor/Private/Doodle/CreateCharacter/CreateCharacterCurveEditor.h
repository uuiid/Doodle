#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "EditorUndoClient.h"
#include "RichCurveEditorModel.h"
#include "Animation/SmartName.h"
#include "Animation/AnimCurveTypes.h"
#include "CurveEditorTypes.h"
#include "Animation/AnimSequenceBase.h"

class FCurveEditor;
class ITimeSliderController;
class SCurveEditorTree;
class IPersonaPreviewScene;
class SCurveEditorPanel;
class FTabManager;

class SCreateCharacterCurveEditor : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SCreateCharacterCurveEditor) {}
  SLATE_ARGUMENT(TSharedPtr<ITimeSliderController>, ExternalTimeSliderController)

  SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  void ResetCurves();
  void AddCurve(const FText& InCurveDisplayName, const FLinearColor& InCurveColor, const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex, FSimpleDelegate InOnCurveModified);
  void RemoveCurve(const FSmartName& InName, ERawCurveTrackTypes InType, int32 InCurveIndex);
  void ZoomToFit();

 private:
  // 为这个曲线编辑器建立工具条
  TSharedRef<SWidget> MakeToolbar(TSharedRef<SCurveEditorPanel> InEditorPanel);
  TSharedPtr<SWidget> OnContextMenuOpening();

 private:
  /** The actual curve editor */
  TSharedPtr<FCurveEditor> CurveEditor;

  /** The search widget for filtering curves in the Curve Editor tree. */
  TSharedPtr<SWidget> CurveEditorSearchBox;

  /** The tree widget in the curve editor */
  TSharedPtr<SCurveEditorTree> CurveEditorTree;
};