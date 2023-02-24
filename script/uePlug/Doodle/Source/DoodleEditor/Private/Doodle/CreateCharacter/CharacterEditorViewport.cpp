#include "CharacterEditorViewport.h"

#include "Animation/SkeletalMeshActor.h"       // 创建骨骼网格体使用
#include "Components/PoseableMeshComponent.h"  // 骨骼网格体
#include "GameFramework/WorldSettings.h"
FCharacterEditorPreviewScene::FCharacterEditorPreviewScene()
    : FAdvancedPreviewScene(

          FAdvancedPreviewScene::ConstructionValues{}.SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)

      ) {
  SetFloorVisibility(true);
  // 时间场景设置
  GetWorld()->GetWorldSettings()->NotifyBeginPlay();
  GetWorld()->GetWorldSettings()->NotifyMatchStarted();
  GetWorld()->GetWorldSettings()->SetActorHiddenInGame(false);
  GetWorld()->bBegunPlay = true;
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

void FCharacterEditorPreviewScene::Tick(float InDeltaTime) {
  FAdvancedPreviewScene::Tick(InDeltaTime);

  if (!GIntraFrameDebuggingGameThread) {
    GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
  }
}

void FCharacterEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas) {
  FEditorViewportClient::Draw(InViewport, Canvas);
}

void FCharacterEditorViewportClient::Tick(float DeltaSeconds) {
  // GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
  FEditorViewportClient::Tick(DeltaSeconds);
  // if (DeltaSeconds > 0) {
  //}
}

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

void SCharacterEditorViewport::doodle_test(const FName& In_Bone, float in_value) {
  UPoseableMeshComponent* L_SK_Com = PreviewActor->FindComponentByClass<UPoseableMeshComponent>();
  //// TArray<FTransform> LTrans        = L_SK_Com->GetBoneSpaceTransforms();
  //// auto L_index                     =
  ///L_SK_Com->GetSkinnedAsset()->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(In_Bone); /
  ///LTrans[L_index].AddToTranslation(FVector{0, in_value, 0});
  static FVector L_Tran            = L_SK_Com->GetBoneLocationByName(In_Bone, EBoneSpaces::ComponentSpace);
  FVector L_TMP                    = L_Tran + FVector{0, in_value, 0};
  L_SK_Com->SetBoneLocationByName(In_Bone, L_TMP, EBoneSpaces::ComponentSpace);
  L_SK_Com->MarkRefreshTransformDirty();
  this->GetViewportClient()->Invalidate();
}

TSharedRef<FEditorViewportClient> SCharacterEditorViewport::MakeEditorViewportClient() {
  if (!AdvancedPreviewScene) AdvancedPreviewScene = MakeShared<FCharacterEditorPreviewScene>();

  if (!LevelViewportClient)
    LevelViewportClient =
        MakeShared<FCharacterEditorViewportClient>(nullptr, AdvancedPreviewScene.Get(), SharedThis(this));

  if (!PreviewActor) {
    PreviewActor                   = LevelViewportClient->GetWorld()->SpawnActor<AActor>();
    UActorComponent* L_Sk_Poseable = PreviewActor->AddComponentByClass(
        UPoseableMeshComponent::StaticClass(),  // 创建的类
        true,                                   // 自动附加
        FTransform{},                           // 附加变换
        true                                    // 自动注册
    );

    UPoseableMeshComponent* L_Sk_Poseable_Com            = CastChecked<UPoseableMeshComponent>(L_Sk_Poseable);
    L_Sk_Poseable_Com->PrimaryComponentTick.bCanEverTick = true;
    L_Sk_Poseable_Com->PrimaryComponentTick.SetTickFunctionEnable(true);
    L_Sk_Poseable->RegisterComponent();
  }

  if (ShowSkeletaMesh) {
    PreviewActor->FindComponentByClass<UPoseableMeshComponent>()->SetSkeletalMesh(ShowSkeletaMesh);
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
