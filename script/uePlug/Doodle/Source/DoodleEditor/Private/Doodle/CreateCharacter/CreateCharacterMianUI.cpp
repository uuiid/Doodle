#include "CreateCharacterMianUI.h"

#include "CharacterEditorViewport.h"
#include "Engine/SkeletalMeshSocket.h"  // 骨骼 Socket
#include "Widgets/Input/SSlider.h"      // 滑动条
#include "SScrubControlPanel.h"         // 时间控制
const FName SCreateCharacterMianUI::Name{"Doodle_CreateCharacterMianUI"};

namespace {
void set_sk_(USkeletalMesh* in_sk, float in_value) {
  USkeleton* L_Sk_Bone      = in_sk->GetSkeleton();
  // auto L_index         = L_Sk_Bone->GetReferenceSkeleton().FindBoneIndex(TEXT("neck_01"));
  // const_cast<TArray<FTransform>&>(L_Sk_Bone->GetReferenceSkeleton().GetRefBonePose())[L_index].AddToTranslation(FVector{0,
  // in_value, 0});
  USkeletalMeshSocket* L_Sk = in_sk->FindSocket(TEXT("neck_01"));

  if (L_Sk) {
    UE_LOG(LogTemp, Log, TEXT("Bone tran %s"), *L_Sk->GetName());
    L_Sk->RelativeLocation = FVector{0, in_value, 0};
  }
}
}  // namespace

void SCreateCharacterMianUI::Construct(const FArguments& Arg) {
  USkeletalMesh* L_Sk = LoadObject<USkeletalMesh>(
      nullptr, TEXT("/Script/Engine.SkeletalMesh'/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin'")
  );

  // clang-format off
  ChildSlot
  [
    SNew(SBorder)
    .BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 0.0f))
    .BorderImage(new FSlateBrush())
    .HAlign(HAlign_Fill)
    [
      SNew(SVerticalBox)
      // 临时调整槽
      + SVerticalBox::Slot()
      .AutoHeight()
      .VAlign(VAlign_Center)
      .Padding(2.0f)
      [
        SNew(SSlider)
        .MinValue(0.f)
        .MaxValue(100.f)
        .OnValueChanged_Lambda([this,L_Sk](float in_int){
          //set_sk_(L_Sk,in_int);
          CharacterEditorViewport->doodle_test(TEXT("neck_01"),in_int);
        })
      ]
      // 渲染槽
      + SVerticalBox::Slot()
      .FillHeight(1.0f)
      [
        SAssignNew(CharacterEditorViewport, SCharacterEditorViewport)
        .ToolTipText(FText::FromString(TEXT("render windwos")))
      ]
      + SVerticalBox::Slot()
      .FillHeight(1.0f)
      [
        SNew(SScrubControlPanel)
        .ToolTipText(FText::FromString(TEXT("render windwos")))
      ]
    ]
  ];
  // clang-format on

  CharacterEditorViewport->SetViewportSkeletal(L_Sk);
}

void SCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterMianUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterMianUI)];  //
}
