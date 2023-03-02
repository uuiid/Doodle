#include "CharacterEditorViewport.h"

#include "AnimPreviewInstance.h"                // 动画预览实例
#include "Animation/DebugSkelMeshComponent.h"   // debug sk mesh
#include "Animation/SkeletalMeshActor.h"        // 创建骨骼网格体使用
#include "AnimationRecorder.h"                  // 镜头录制
#include "AnimationUtils.h"                     // 动画压缩设置
#include "AssetRegistry/AssetRegistryModule.h"  // 广播包更改
#include "Components/PoseableMeshComponent.h"   // 骨骼网格体
#include "CoreData/DoodleCreateCharacterAnimAsset.h"
#include "CoreData/DoodleCreateCharacterInstance.h"
#include "GameFramework/WorldSettings.h"
#include "SequenceRecorderSettings.h"  // 镜头录制设置
#include "ThumbnailHelpers.h"          // 动画场景播放
#include "AssetEditorModeManager.h"    // 工具编辑
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

FCharacterEditorViewportClient::FCharacterEditorViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
    : FEditorViewportClient{new FAssetEditorModeManager{}, InPreviewScene, InEditorViewportWidget} {
  SetViewMode(VMI_Lit);
  SetViewportType(LVT_Perspective);

  SetInitialViewTransform(LVT_Perspective, EditorViewportDefs::DefaultPerspectiveViewLocation, EditorViewportDefs::DefaultPerspectiveViewRotation, EditorViewportDefs::DefaultPerspectiveFOVAngle);
   
  // 重要!:  在这里设置实时
  SetRealtime(true);

}

void FCharacterEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas) {
  FEditorViewportClient::Draw(InViewport, Canvas);
}

void FCharacterEditorViewportClient::Tick(float DeltaSeconds) {
  // GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
  FEditorViewportClient::Tick(DeltaSeconds);
  GetPreviewScene()->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
  // if (DeltaSeconds > 0) {
  //}
}

FName SCharacterEditorViewport::G_Name{"DoodleCreateCharacter"};

void SCharacterEditorViewportToolBar::Construct(
    const FArguments& InArgs, TSharedPtr<SCharacterEditorViewport> InRealViewport
) {
  SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), InRealViewport);
}

void SCharacterEditorViewport::Construct(const FArguments& Arg) {
  SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<class SEditorViewport> SCharacterEditorViewport::GetViewportWidget() { return SharedThis(this); }

TSharedPtr<FExtender> SCharacterEditorViewport::GetExtenders() const {
  TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
  return Result;
}

void SCharacterEditorViewport::OnFloatingButtonClicked() {}

void SCharacterEditorViewport::doodle_test(const FName& In_Bone, float in_value) {
#define DOODLE_TEST

#ifndef DOODLE_TEST
  if (!ShowSkeletaMesh->GetSkeletalMeshAsset()) return;

  UAnimPreviewInstance* L_AnimAnimInstance = Cast<UAnimPreviewInstance>(ShowSkeletaMesh->GetAnimInstance());
  if (!L_AnimAnimInstance) return;
  UAnimSequence* L_Anim_Ass = Cast<UAnimSequence>(L_AnimAnimInstance->GetCurrentAsset());

  if (!L_Anim_Ass) return;

  // L_Anim_Ass->AddKeyToSequence(0.0f, In_Bone, FTransform{FVector::OneVector * in_value});
  ShowSkeletaMesh->RefreshBoneTransforms();
  ShowSkeletaMesh->UpdateBounds();
  ShowSkeletaMesh->RecreateRenderState_Concurrent();
  // USkeleton* L_Skeleton = ShowSkeletaMesh->GetSkeletalMeshAsset()->GetSkeleton();

  // FSmartName L_Out_Name{};
  // if (!L_Skeleton->GetSmartNameByName(USkeleton::AnimTrackCurveMappingName, In_Bone, L_Out_Name)) {
  //   L_Skeleton->AddSmartNameAndModify(USkeleton::AnimTrackCurveMappingName, In_Bone, L_Out_Name);
  // }

  // UDoodleCreateCharacterInstance* L_Anim =
  // CastChecked<UDoodleCreateCharacterInstance>(ShowSkeletaMesh->GetAnimInstance());
  // L_Anim->GetCreateCharacterProxyOnGameThread()->StoredCurves.Add(L_Out_Name.UID, 1);

  // UPoseableMeshComponent* L_SK_Com = PreviewActor->FindComponentByClass<UPoseableMeshComponent>();
  ////// TArray<FTransform> LTrans        = L_SK_Com->GetBoneSpaceTransforms();
  ////// auto L_index                     =
  ///// L_SK_Com->GetSkinnedAsset()->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(In_Bone); /
  ///// LTrans[L_index].AddToTranslation(FVector{0, in_value, 0});
  // static FVector L_Tran            = L_SK_Com->GetBoneLocationByName(In_Bone, EBoneSpaces::ComponentSpace);
  // FVector L_TMP                    = L_Tran + FVector{0, in_value, 0};
  // L_SK_Com->SetBoneLocationByName(In_Bone, L_TMP, EBoneSpaces::ComponentSpace);
  // L_SK_Com->MarkRefreshTransformDirty();
  this->GetViewportClient()->Invalidate();

#endif  // !DOODLE_TEST

#ifdef DOODLE_TEST

  UAnimSequence* L_Ass = LoadObject<UAnimSequence>(
      GetTransientPackage(), TEXT("/Script/Engine.AnimSequence'/Game/Mannequin/Animations/ThirdPersonWalk.ThirdPersonWalk'")
  );
  // ShowSkeletaMesh->Play(true);
  // ShowSkeletaMesh->PrimaryComponentTick.bCanEverTick = true;
  // ShowSkeletaMesh->PrimaryComponentTick.SetTickFunctionEnable(true);
  // ShowSkeletaMesh->PreviewInstance->SetPosition(0.0f);
  // ShowSkeletaMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
  // L_Ass->SetPreviewMesh(ShowSkeletaMesh->GetSkeletalMeshAsset());
  L_Ass->SetSkeleton(SkeletalMesh->GetSkeleton());
  // ShowSkeletaMesh->PushSelectionToProxy();
  // ShowSkeletaMesh->InitArticulated(GetWorld()->GetPhysicsScene());

  ShowSkeletaMesh->EnablePreview(true, L_Ass);
  // ShowSkeletaMesh->PreviewInstance->SetPlaying(true);

  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));

  // ShowSkeletaMesh->SetAnimInstanceClass(UAnimPreviewInstance::StaticClass());
  // ShowSkeletaMesh->Stop();

  this->GetViewportClient()->Invalidate();
#endif  // !DOODLE_TEST
}

void SCharacterEditorViewport::SetViewportSkeletal(USkeletalMesh* InSkeletaMesh) {
  SkeletalMesh = InSkeletaMesh;
  if (!ShowSkeletaMesh) return;

  ShowSkeletaMesh->SetSkeletalMesh(InSkeletaMesh);
#ifdef DOODLE_DIS1

  USkeleton* L_Skeleton                  = ShowSkeletaMesh->GetSkeletalMeshAsset()->GetSkeleton();
  UDoodleCreateCharacterAnimAsset* L_Ass = NewObject<UDoodleCreateCharacterAnimAsset>();
  L_Ass->SetSkeleton(L_Skeleton);
  // 通知资产创建
  FAssetRegistryModule::AssetCreated(L_Ass);

  if (!L_Ass->BoneCompressionSettings) {
    L_Ass->BoneCompressionSettings = FAnimationUtils::GetDefaultAnimationRecorderBoneCompressionSettings();
  }
  if (L_Ass->CurveCompressionSettings)
    L_Ass->CurveCompressionSettings = FAnimationUtils::GetDefaultAnimationCurveCompressionSettings();

#endif  // DOODLE_DIS1

#ifdef DOODLE_DIS2

  // 使用录制器修正资产
  TSharedPtr<FAnimationRecorder> L_Recorder    = MakeShared<FAnimationRecorder>();
  const FAnimationRecordingSettings& L_Setting = GetDefault<USequenceRecorderSettings>()->DefaultAnimationSettings;
  L_Recorder->SetSampleRateAndLength(L_Setting.SampleFrameRate, L_Setting.Length);
  L_Recorder->bRecordLocalToWorld        = L_Setting.bRecordInWorldSpace;
  L_Recorder->Interpolation              = L_Setting.Interpolation;
  L_Recorder->InterpMode                 = L_Setting.InterpMode;
  L_Recorder->TangentMode                = L_Setting.TangentMode;
  L_Recorder->bAutoSaveAsset             = L_Setting.bAutoSaveAsset;
  L_Recorder->bRemoveRootTransform       = L_Setting.bRemoveRootAnimation;
  L_Recorder->bCheckDeltaTimeAtBeginning = L_Setting.bCheckDeltaTimeAtBeginning;
  L_Recorder->bRecordTransforms          = L_Setting.bRecordTransforms;
  L_Recorder->bRecordMorphTargets        = L_Setting.bRecordMorphTargets;
  L_Recorder->bRecordAttributeCurves     = L_Setting.bRecordAttributeCurves;
  L_Recorder->bRecordMaterialCurves      = L_Setting.bRecordMaterialCurves;
  L_Recorder->IncludeAnimationNames      = L_Setting.IncludeAnimationNames;
  L_Recorder->ExcludeAnimationNames      = L_Setting.ExcludeAnimationNames;

  L_Recorder->TriggerRecordAnimation(ShowSkeletaMesh);
  // L_Recorder->StartRecord(ShowSkeletaMesh, L_Ass);
  // L_Recorder->UpdateRecord(ShowSkeletaMesh, 0.1);
  UAnimSequence* L_Anim_Ass = L_Recorder->GetAnimationObject();
  L_Recorder->StopRecord(false);

  ShowSkeletaMesh->EnablePreview(true, L_Anim_Ass);
  // ShowSkeletaMesh->SetPosition(AnimPosition, false);
#endif  // DOODLE_DIS2

#ifdef DOODLE_DIS3
  UAnimSequence* L_Ass = LoadObject<UAnimSequence>(
      GetTransientPackage(), TEXT("/Script/Engine.AnimSequence'/Game/Mannequin/Animations/ThirdPersonWalk.ThirdPersonWalk'")
  );
  ShowSkeletaMesh->Play(true);
  ShowSkeletaMesh->PrimaryComponentTick.bCanEverTick = true;
  ShowSkeletaMesh->PrimaryComponentTick.SetTickFunctionEnable(true);
  ShowSkeletaMesh->PreviewInstance->SetPosition(0.0f);
  ShowSkeletaMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
  L_Ass->SetPreviewMesh(ShowSkeletaMesh->GetSkeletalMeshAsset());

  ShowSkeletaMesh->PushSelectionToProxy();
  ShowSkeletaMesh->InitArticulated(GetWorld()->GetPhysicsScene());

  ShowSkeletaMesh->EnablePreview(true, L_Ass);
  ShowSkeletaMesh->PreviewInstance->SetPlaying(true);

  // ShowSkeletaMesh->SetAnimInstanceClass(UAnimPreviewInstance::StaticClass());
  // ShowSkeletaMesh->Stop();

  this->GetViewportClient()->Invalidate();
#endif  // DOODLE_DIS3
}

TSharedRef<FEditorViewportClient> SCharacterEditorViewport::MakeEditorViewportClient() {
  if (!AdvancedPreviewScene) AdvancedPreviewScene = MakeShared<FCharacterEditorPreviewScene>();

  if (!LevelViewportClient)
    LevelViewportClient =
        MakeShared<FCharacterEditorViewportClient>(AdvancedPreviewScene.Get(), SharedThis(this));
  LevelViewportClient->ViewportType         = LVT_Perspective;
  LevelViewportClient->bSetListenerPosition = false;
  LevelViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
  LevelViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
  LevelViewportClient->EngineShowFlags.SetCompositeEditorPrimitives(true);
  // 设置实时
  // LevelViewportClient->SetRealtime(true);

  if (!PreviewActor) {
    PreviewActor                   = LevelViewportClient->GetWorld()->SpawnActor<AActor>();
    UActorComponent* L_Sk_Poseable = PreviewActor->AddComponentByClass(
        UDebugSkelMeshComponent::StaticClass(),  // 创建的类
        true,                                    // 自动附加
        FTransform{},                            // 附加变换
        true                                     // 自动注册
    );
    L_Sk_Poseable->RegisterComponent();
    ShowSkeletaMesh = CastChecked<UDebugSkelMeshComponent>(L_Sk_Poseable);
    PreviewActor->SetRootComponent(ShowSkeletaMesh);
    AdvancedPreviewScene->AddComponent(ShowSkeletaMesh, FTransform::Identity);
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
