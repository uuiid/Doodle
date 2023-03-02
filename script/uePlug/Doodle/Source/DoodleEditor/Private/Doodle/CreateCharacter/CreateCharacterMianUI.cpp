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
      // 临时按钮
       + SVerticalBox::Slot()
      .AutoHeight()
      .VAlign(VAlign_Center)
      .Padding(2.0f)
      [
        SNew(SButton)
        .OnClicked_Lambda([=](){
          USkeletalMesh* L_Sk = LoadObject<USkeletalMesh>(
          nullptr, TEXT("/Script/Engine.SkeletalMesh'/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin'")
                  );
          CharacterEditorViewport->SetViewportSkeletal(L_Sk);
          return FReply::Handled();
          
        })
        .Text(FText::FromString(TEXT("test")))
      ]
      
      // 临时调整槽
      + SVerticalBox::Slot()
      .AutoHeight()
      .VAlign(VAlign_Center)
      .Padding(2.0f)
      [
        SNew(SSlider)
        .MinValue(0.f)
        .MaxValue(100.f)
        .OnValueChanged_Lambda([this](float in_int){
          CharacterEditorViewport->doodle_test(TEXT("neck_01"),in_int);
        })
      ]

      // 渲染槽
      + SVerticalBox::Slot()
      .FillHeight(1.0f)
      [
        CharacterEditorViewport.ToSharedRef()
      ]
      //+ SVerticalBox::Slot()
      //.FillHeight(1.0f)
      //[
      //  SNew(SScrubControlPanel)
      //  .ToolTipText(FText::FromString(TEXT("render windwos")))
      //]
    ]
  ];
  // clang-format on
}

void SCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterMianUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterMianUI)];  //
}
