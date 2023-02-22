#include "CreateCharacterMianUI.h"

#include "CharacterEditorViewport.h"
#include "Widgets/Input/SSlider.h"  // 滑动条

const FName SCreateCharacterMianUI::Name{"Doodle_CreateCharacterMianUI"};

namespace {
void set_sk_(USkeletalMesh* in_sk, float in_value) { USkeleton* L_Sk_Bone = in_sk->GetSkeleton(); };
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
        .OnValueChanged_Lambda([L_Sk](float in_int){
           set_sk_(L_Sk,in_int);
        })
      ]
      // 渲染槽
      + SVerticalBox::Slot()
      .FillHeight(1.0f)
      [
        SNew(SCharacterEditorViewport, L_Sk)
        .ToolTipText(FText::FromString(TEXT("render windwos")))
      ]
    ]
  ];
  // clang-format on
}

void SCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterMianUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterMianUI)];  // ���ﴴ�������Լ��Ľ���
}
