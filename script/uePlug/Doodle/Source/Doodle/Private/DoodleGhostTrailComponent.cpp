#include "DoodleGhostTrailComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UDoodleGhostTrailComponent::UDoodleGhostTrailComponent(
    const FObjectInitializer &ObjectInitializer
)
    : UActorComponent(ObjectInitializer) {
  BoneName                          = TEXT("Spine1_M");
  PrimaryComponentTick.bCanEverTick = true;
  FRichCurve *RichCurve             = TransparentCurve.GetRichCurve();
  RichCurve->AutoSetTangents();
  RichCurve->SetKeyTangentMode(
      RichCurve->AddKey(0.0f, 0.0f), ERichCurveTangentMode::RCTM_Auto
  );
  RichCurve->SetKeyTangentMode(
      RichCurve->AddKey(1.0f, 1.0f), ERichCurveTangentMode::RCTM_Auto
  );
}

void UDoodleGhostTrailComponent::BeginPlay() {
  Super::BeginPlay();
  PrimaryComponentTick.SetTickFunctionEnable(true);

  SkeletalMeshComponent_P =
      GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
  if (SkeletalMeshComponent_P) {
    PreviousLocation =
        SkeletalMeshComponent_P->GetBoneLocation(BoneName);
    PreviousTransform         = SkeletalMeshComponent_P->GetBoneSpaceTransforms();
    PreviousLocationTransform = SkeletalMeshComponent_P->GetComponentTransform();
  }
}

void UDoodleGhostTrailComponent::TickComponent(
    float DeltaTime,
    enum ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction
) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  if (SkeletalMeshComponent_P) {
    FVector LLocation =
        SkeletalMeshComponent_P->GetBoneLocation(BoneName);

    CreateGhost(LLocation, DeltaTime);
    UpdataGhost(DeltaTime);
    ClearGhost();

    PreviousLocation =
        SkeletalMeshComponent_P->GetBoneLocation(BoneName);
    PreviousTransform         = SkeletalMeshComponent_P->GetBoneSpaceTransforms();
    PreviousLocationTransform = SkeletalMeshComponent_P->GetComponentTransform();
  }
}

void UDoodleGhostTrailComponent::CreateGhost(FVector InLocation, float DeltaTime) {
  float LLen =
      FMath::Abs(
          FVector::Distance(InLocation, PreviousLocation)
      );

  if (LLen > Distance) {
    const int LSize               = FMath::DivideAndRoundDown(LLen, Distance);
    TArray<FTransform> LTrans     = SkeletalMeshComponent_P->GetBoneSpaceTransforms();
    FTransform LSkeletalTransform = SkeletalMeshComponent_P->GetComponentTransform();

    FVector Stepping              = (PreviousLocation - InLocation) / LSize;

    for (int i = 0;
         i < FGenericPlatformMath::Min(LSize, MaxCount);
         ++i) {
      UPoseableMeshComponent *LPoseableMeshComponent =
          Cast<UPoseableMeshComponent>(
              GetOwner()
                  ->AddComponentByClass(
                      UPoseableMeshComponent::StaticClass(),  // 创建的类
                      true,                                   // 自动附加
                      LSkeletalTransform,                     // 附加变换
                      true                                    // 自动注册
                  )
          );
      TArray<FTransform> LTransInter;
      if (!LPoseableMeshComponent) {
        return;
      }
      // 设置骨骼网格体
      LPoseableMeshComponent->SetSkeletalMesh(
          SkeletalMeshComponent_P->SkeletalMesh
      );
      LPoseableMeshComponent->RegisterComponent();

      for (size_t j = 0; j < LTrans.Num(); ++j) {
        FTransform LL{};
        LL.Blend(PreviousTransform[j], LTrans[j], i / (float)LSize);
        LTransInter.Add(LL);
      }
      if (LPoseableMeshComponent->BoneSpaceTransforms.Num() ==
          LTransInter.Num()) {
        // LPoseableMeshComponent->BoneSpaceTransforms = LTransInter;
        Exchange(LPoseableMeshComponent->BoneSpaceTransforms, LTransInter);
        LPoseableMeshComponent->MarkRefreshTransformDirty();

        FTransform LLF{};
        LLF.Blend(PreviousLocationTransform, LSkeletalTransform, i / (float)LSize);
        LPoseableMeshComponent->SetWorldTransform(LLF);

        FDoodleGhostTrailInfo LDoodleGhostTrailInfo{};
        LDoodleGhostTrailInfo.Ghost                = LPoseableMeshComponent;
        LDoodleGhostTrailInfo.Life                 = Life;
        LDoodleGhostTrailInfo.Age                  = i * (DeltaTime / (float)LSize);
        LPoseableMeshComponent->bRenderCustomDepth = bRenderCustomDepth;

        SetMaterial_Doodle(LPoseableMeshComponent);

        GhostInfos.Add(LDoodleGhostTrailInfo);
      } else {
        LPoseableMeshComponent->UnregisterComponent();
      }
    }
  }
}

void UDoodleGhostTrailComponent::UpdataGhost(float DeltaTime) {
  for (auto &&i : GhostInfos) {
    i.Age += DeltaTime;
    FRichCurve *RichCurve = TransparentCurve.GetRichCurve();
    if (RichCurve) {
      float LValue = RichCurve->Eval(i.Age / i.Life);
      UpdataMaterial_Doodle(i.Ghost, LValue);
    }
  }
}
void UDoodleGhostTrailComponent::SetMaterial_Doodle(UPoseableMeshComponent *InGhost) {
  TArray<UMaterialInterface *> LMaterials = SkeletalMeshComponent_P->GetMaterials();
  for (size_t i = 0; i < LMaterials.Num(); ++i) {
    UMaterialInstanceDynamic *MaterialInstanceDynamic =
        UMaterialInstanceDynamic::Create(LMaterials[i], InGhost);
    InGhost->SetMaterial(i, MaterialInstanceDynamic);
  }
}

void UDoodleGhostTrailComponent::UpdataMaterial_Doodle(UPoseableMeshComponent *InGhost, float InValue) {
  TArray<UMaterialInterface *> LMaterials = InGhost->GetMaterials();
  for (auto LM : LMaterials) {
    UMaterialInstanceDynamic *MaterialInstanceDynamic =
        Cast<UMaterialInstanceDynamic>(LM);
    if (MaterialInstanceDynamic) {
      MaterialInstanceDynamic->SetScalarParameterValue(TransparentName, InValue);
    }
  }
}

void UDoodleGhostTrailComponent::ClearGhost() {
  for (auto &&i : GhostInfos) {
    if (i.Age > i.Life) {
      i.Ghost->UnregisterComponent();
      GetOwner()->RemoveOwnedComponent(i.Ghost);
      i.Ghost->DestroyComponent();
    }
  }
  GhostInfos.RemoveAll([](const FDoodleGhostTrailInfo &In) -> bool { return In.Age > In.Life; });

  auto L_Surplus = GhostInfos.Num() - MaxCount;
  if (L_Surplus >= 1) {
    for (auto i = 0; i < L_Surplus; ++i) {
      GhostInfos[i].Ghost->UnregisterComponent();
      GetOwner()->RemoveOwnedComponent(GhostInfos[i].Ghost);
      GhostInfos[i].Ghost->DestroyComponent();
    }
    GhostInfos.RemoveAt(0, L_Surplus);
  }

  UE_LOG(LogTemp, Warning, TEXT("GhostInfos num %d"), GhostInfos.Num());
}
