#include "CharacterEditorViewport.h"

#include "Animation/SkeletalMeshActor.h"  // 创建骨骼网格体使用
#include "GameFramework/WorldSettings.h"
FCharacterEditorPreviewScene::FCharacterEditorPreviewScene()
    : FAdvancedPreviewScene(

          FAdvancedPreviewScene::ConstructionValues{}.SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)

      ) {
  SetFloorVisibility(true);
  // 时间场景设置

  // 创建预览场景描述

  // UClass* L_Sk = LoadClass<AActor>(NULL,
  // TEXT("/Script/Engine.SkeletalMesh'/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin'")); if (L_Sk)
  // {
  //   GetWorld()->SpawnActor<AActor>(L_Sk, FVector{}, FRotator{});
  // }
  //  AWorldSettings* L_Setting = GetWorld()->GetWorldSettings();
  //  L_Setting->NotifyBeginPlay();
  //  L_Setting->NotifyMatchStarted();

  // GetWorld()->bBegunPlay         = true;

  // 灯光设置

  // 创建天空光
  // UStaticMeshComponent* L_SkyCom = NewObject<UStaticMeshComponent>();

  // AddComponent(L_SkyCom, FTransform{FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1000.0f)});
  //  添加骨骼网格体
}

void FCharacterEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas) {
  FEditorViewportClient::Draw(InViewport, Canvas);
}

void FCharacterEditorViewportClient::Tick(float DeltaSeconds) { FEditorViewportClient::Tick(DeltaSeconds); }

void SCharacterEditorViewportToolBar::Construct(
    const FArguments& InArgs, TSharedPtr<SCharacterEditorViewport> InRealViewport
) {
  SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), InRealViewport);
}

void SCharacterEditorViewport::Construct(const FArguments& Arg, USkeletalMesh* InSkeletaMesh) {
  ShowSkeletaMesh = InSkeletaMesh;
  SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<class SEditorViewport> SCharacterEditorViewport::GetViewportWidget() { return SharedThis(this); }

TSharedPtr<FExtender> SCharacterEditorViewport::GetExtenders() const {
  TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
  return Result;
}

void SCharacterEditorViewport::OnFloatingButtonClicked() {}

TSharedRef<FEditorViewportClient> SCharacterEditorViewport::MakeEditorViewportClient() {
  if (!AdvancedPreviewScene) AdvancedPreviewScene = MakeShared<FCharacterEditorPreviewScene>();

  if (!LevelViewportClient)
    LevelViewportClient =
        MakeShared<FCharacterEditorViewportClient>(nullptr, AdvancedPreviewScene.Get(), SharedThis(this));

  if (!PreviewActor) PreviewActor = LevelViewportClient->GetWorld()->SpawnActor<ASkeletalMeshActor>();

  // USkeletalMesh* L_Sk = LoadObject<USkeletalMesh>(nullptr,
  // TEXT("/Script/Engine.SkeletalMesh'/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin'"));
  if (ShowSkeletaMesh) {
    PreviewActor->GetSkeletalMeshComponent()->SetSkeletalMesh(ShowSkeletaMesh);
  }
  // LevelViewportClient                       = MakeShareable(new FTestViewportClient(*scene, context,
  // SharedThis(this)));

  LevelViewportClient->ViewportType         = LVT_Perspective;
  LevelViewportClient->bSetListenerPosition = false;
  return LevelViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SCharacterEditorViewport::MakeViewportToolbar() {
  return SNew(SCharacterEditorViewportToolBar, SharedThis(this)).Cursor(EMouseCursor::Default);
}
