#include "CreateCharacterMianUI.h"

#include "CharacterEditorViewport.h"
const FName SCreateCharacterMianUI::Name{"Doodle_CreateCharacterMianUI"};

void SCreateCharacterMianUI::Construct(const FArguments& Arg) {
  USkeletalMesh* L_Sk = LoadObject<USkeletalMesh>(
      nullptr, TEXT("/Script/Engine.SkeletalMesh'/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin'")
  );
  // clang-format off
  ChildSlot
  [
    SNew(SCharacterEditorViewport,L_Sk)
    .ToolTipText(FText::FromString(TEXT("render windwos")))
  ];
  // clang-format on
}

void SCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterMianUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterMianUI)];  // ���ﴴ�������Լ��Ľ���
}
