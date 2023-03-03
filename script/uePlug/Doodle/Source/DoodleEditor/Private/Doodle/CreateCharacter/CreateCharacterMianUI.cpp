#include "CreateCharacterMianUI.h"

#include "CharacterEditorViewport.h"
#include "Engine/SkeletalMeshSocket.h"  // 骨骼 Socket
#include "Widgets/Input/SSlider.h"      // 滑动条
#include "SScrubControlPanel.h"         // 时间控制

const FName SCreateCharacterMianUI::Name{"Doodle_CreateCharacterMianUI"};

void SCreateCharacterMianUI::Construct(const FArguments& Arg) {
  // clang-format off
  CharacterEditorViewport = SNew(SCharacterEditorViewport)
    .ToolTipText(FText::FromString(TEXT("render windwos")));
  // clang-format on

  // clang-format off
  ChildSlot
  [
    SNew(SBorder)
    .BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 0.0f))
    .BorderImage(new FSlateBrush())
    .HAlign(HAlign_Fill)
    [
      SNew(SVerticalBox)
      // 渲染槽
      + SVerticalBox::Slot()
      .FillHeight(1.0f)
      [
        CharacterEditorViewport.ToSharedRef()
      ]
    ]
  ];
  // clang-format on
}

void SCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterMianUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterMianUI)];  //
}
