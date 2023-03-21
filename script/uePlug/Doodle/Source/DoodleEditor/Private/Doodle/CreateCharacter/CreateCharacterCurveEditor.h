#pragma once

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/SmartName.h"
#include "CurveEditorTypes.h"
#include "EditorUndoClient.h"
#include "RichCurveEditorModel.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FCurveEditor;
class ITimeSliderController;
class SCurveEditorTree;
class IPersonaPreviewScene;
class SCurveEditorPanel;
class FTabManager;
class UDoodleCreateCharacterConfig;

struct FDoodleCreateCharacterConfigNode;

class UCreateCharacterMianTreeItem;

class SCreateCharacterCurveEditor : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SCreateCharacterCurveEditor)
      : _CreateCharacterConfigConfig(),
        _TabManager() {}

  SLATE_ARGUMENT(UDoodleCreateCharacterConfig*, CreateCharacterConfigConfig)
  SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  void EditCurve(const TSharedPtr<UCreateCharacterMianTreeItem>& In_Node);

  void ZoomToFit();

 private:
  void ResetCurves();
  void AddCurve(
      const FRichCurveEditInfo& In_Info
  );
  // 为这个曲线编辑器建立工具条
  TSharedRef<SWidget> MakeToolbar(TSharedRef<SCurveEditorPanel> InEditorPanel);
  TSharedPtr<SWidget> OnContextMenuOpening();

 private:
  TObjectPtr<UDoodleCreateCharacterConfig> CreateCharacterConfigConfig;

  /** The actual curve editor */
  TSharedPtr<FCurveEditor> CurveEditor;

  /** The search widget for filtering curves in the Curve Editor tree. */
  TSharedPtr<SWidget> CurveEditorSearchBox;

  /** The tree widget in the curve editor */
  TSharedPtr<SCurveEditorTree> CurveEditorTree;

  TSharedPtr<UCreateCharacterMianTreeItem> CurrentSelect;
};