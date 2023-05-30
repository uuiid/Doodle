// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleMatrixLight.h"
#include "Components/SpotLightComponent.h"

// 添加编辑器显示图标
#include "Components/ArrowComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Components/SplineComponent.h"
#include "Engine/Scene.h"

ADoodleMatrixLight::ADoodleMatrixLight() {
  PrimaryActorTick.bCanEverTick = true;
  USceneComponent *LRoot        = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  SetRootComponent(LRoot);

  SplineComponen_ =
      CreateDefaultSubobject<USplineComponent>("DefaultSplineComponent");
  SplineComponen_->AttachToComponent(
      GetRootComponent(),
      FAttachmentTransformRules::KeepRelativeTransform
  );
  SplineComponen_->SetClosedLoop(true);
}

void ADoodleMatrixLight::CreateLightSqueue() {
  const float ang{360 / (float)LightSamplesQueue};
  SplineComponen_->ClearSplinePoints();

  for (auto i = 0; i < LightSamplesQueue; ++i) {
    USceneComponent *LSceneComponent =
        NewObject<USceneComponent>(GetRootComponent());

    /// 设置旋转方式
    FTransform l_tran{};
    FVector LLocal{FRotator{0, ang * i, 0}.Vector() * CenterOfInterestLength};
    // UE_LOG(LogTemp, Log, TEXT("vector: %s"), *(LLocal.ToString()));
    LSceneComponent->AddLocalOffset(LLocal + FVector{0, 0, 30});

    /// 添加灯光场景根组件
    // LSceneComponent->SetWorldLocation(LLocal);
    LSceneComponent->AttachToComponent(
        RootComponent,
        FAttachmentTransformRules::KeepRelativeTransform
    );
    LSceneComponent->RegisterComponent();

    /// 添加曲线点
    SplineComponen_->AddSplinePoint(
        LSceneComponent->GetComponentTransform().GetLocation(),
        ESplineCoordinateSpace::World
    );

    /// 添加箭头指示
    UArrowComponent *LArrowComponent = NewObject<UArrowComponent>(LSceneComponent);
    LArrowComponent->AttachToComponent(
        LSceneComponent,
        FAttachmentTransformRules{
            EAttachmentRule::KeepRelative,
            EAttachmentRule::KeepRelative,
            EAttachmentRule::KeepRelative, false}
    );
    LArrowComponent->RegisterComponent();

    /// 旋转组件指向圆心
    FRotator LRotator = FRotationMatrix::MakeFromX(
                            GetActorLocation() - LSceneComponent->GetComponentLocation()
    )
                            .Rotator();
    LSceneComponent->SetWorldRotation(LRotator);

    SceneComponentList_.Add(LSceneComponent);
    ArrowList_.Add(LArrowComponent);
    CreateLightSquare(i);
  }
}

void ADoodleMatrixLight::CreateLightSquare(
    int InSceneComponentIndex
) {
  const float LLen  = 30 / (float)LightSamplesSquared;
  const float LSize = LightSamplesSquared / 2;
  for (auto i = 0; i < LightSamplesSquared; ++i) {
    for (auto j = 0; j < LightSamplesSquared; ++j) {
      USceneComponent *LSceneComponent = SceneComponentList_[InSceneComponentIndex];
      USpotLightComponent *LSpotLightComponent =
          NewObject<USpotLightComponent>(LSceneComponent);
      LSpotLightComponent->AttachToComponent(
          LSceneComponent,
          FAttachmentTransformRules{
              EAttachmentRule::KeepRelative,
              EAttachmentRule::SnapToTarget,
              EAttachmentRule::KeepRelative, false}
      );
      LSpotLightComponent->RegisterComponent();

      const int LInt = FMath::Floor((float)i - LSize);
      const int LJnt = FMath::Floor((float)j - LSize);
      // LSpotLightComponent->AddLocalOffset(FVector{0, LLen * LInt, LLen * LJnt});
      LSpotLightComponent->SetIntensityUnits(ELightUnits::Candelas);
      LightList_.Add(LSpotLightComponent);
    }
  }
}

void ADoodleMatrixLight::SetLightAttr() {
  int L_Light_Size     = FMath::Max(LightSamplesSquared, 1);
  int L_SourceSize     = FMath::Min(LightLength, LightWidth);
  float L_SourceRadius = (FMath::Min(LightLength, LightWidth) / L_Light_Size) * SourceRadiusMult;
  float L_SourceLength = (FMath::Max(LightLength, LightWidth) / L_Light_Size) * SourceRadiusMult;

  float L_Intensity    = Intensity / FMath::Pow(L_Light_Size, 2.0);

  for (auto *LLight : LightList_) {
    LLight->SetIntensity(L_Intensity);
    LLight->SetVisibility(true);
    LLight->SetLightingChannels(Channels.bChannel0, Channels.bChannel1, Channels.bChannel2);
    LLight->SetOuterConeAngle(FocalAngleOuter);
    LLight->SetInnerConeAngle(FocalAngleInner);
    LLight->SetLightColor(LightColor);
    LLight->SetAttenuationRadius(AttenuationDistance);

    LLight->SetSourceRadius(FMath::Min(L_SourceRadius, L_SourceLength));
    LLight->SetSourceLength(FMath::Max(L_SourceRadius, L_SourceLength));

    LLight->SetCastShadows(CastShadows);
    LLight->SetShadowBias(ShadowBias);
    LLight->SetSoftSourceRadius(SoftRadius);
  }
  /// 修正位置
  const float LLen = (FMath::Max(L_SourceRadius, L_SourceLength) +
                      FMath::Min(L_SourceRadius, L_SourceLength) *
                          2) /
                     (float)(LightSamplesSquared - 1);
  const float LSize = LightSamplesSquared / 2;
  int Index         = 0;
  for (auto i = 0; i < LightSamplesQueue; ++i) {
    USceneComponent *LSceneComponent = SceneComponentList_[i];
    // UArrowComponent *LArrowComponent = ArrowList_[i];
    // FTransform L_Tran = LSceneComponent->GetComponentTransform();
    // /// 先获取曲线的位置
    // FVector LLocal = SplineComponen_->GetLocationAtSplineInputKey(i, ESplineCoordinateSpace::World);
    FRotator LRotator                = FRotationMatrix::MakeFromX(
                            GetActorLocation() - LSceneComponent->GetComponentLocation()
    )
                            .Rotator();
    // LSceneComponent->SetWorldRotation(LRotator);
    // LSceneComponent->SetWorldLocationAndRotation(LLocal, LRotator);
    // LArrowComponent->SetWorldLocationAndRotation(LLocal, LRotator);
    for (auto j = 0; j < LightSamplesSquared; ++j) {
      for (auto k = 0; k < LightSamplesSquared; ++k) {
        const int LJnt = FMath::Floor((float)j - LSize);
        const int LKnt = FMath::Floor((float)k - LSize);
        USpotLightComponent *LLight =
            LightList_[Index];

        LLight->SetRelativeLocation(FVector{0, 0, 0});
        LLight->AddLocalOffset(FVector{0, LLen * LJnt, LLen * LKnt});
        ++Index;
      }
    }
  }
}

void ADoodleMatrixLight::OnConstruction(const FTransform &Transform) {
  AActor::OnConstruction(Transform);
}
void ADoodleMatrixLight::PostActorCreated() {
  AActor::PostActorCreated();
  CreateLightSqueue();
  SetLightAttr();
}

// void ADoodleMatrixLight::TEST()
// {
//     for (auto *l_un_reg : SceneComponentList_)
//     {
//         l_un_reg->UnregisterComponent();
//         l_un_reg->DestroyComponent();
//     }
//     for (auto *l_un_reg : ArrowList_)
//     {
//         l_un_reg->UnregisterComponent();
//         l_un_reg->DestroyComponent();
//     }
//     for (auto *l_un_reg : LightList_)
//     {
//         l_un_reg->UnregisterComponent();
//         l_un_reg->DestroyComponent();
//     }
//     SceneComponentList_.Empty();
//     ArrowList_.Empty();
//     LightList_.Empty();
//     CreateLightSqueue();
//     SetLightAttr();
// }

#if WITH_EDITOR
void ADoodleMatrixLight::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangeEvent
) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  auto name2 = PropertyChangeEvent.GetPropertyName();
  auto name  = PropertyChangeEvent.MemberProperty ? PropertyChangeEvent.MemberProperty->GetFName() : NAME_None;
  // UE_LOG(LogTemp, Log, TEXT("chick name: %s"), *(name.ToString()));
  // UE_LOG(LogTemp, Log, TEXT("chick MemberProperty: %s"), *(name2.ToString()));
  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, Intensity) ||               //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, LightColor) ||              //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, FocalAngleOuter) ||         //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, FocalAngleInner) ||         //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, AttenuationDistance) ||     //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, LightWidth) ||              //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, LightLength) ||             //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, CastShadows) ||             //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, SourceRadiusMult) ||        //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, CenterOfInterestLength) ||  //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, Channels) ||                //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, SoftRadius) ||              //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, ShadowBias)) {
    SetLightAttr();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, Enabled) || name == GET_MEMBER_NAME_CHECKED(ThisClass, LightSamplesSquared) ||  //
             name == GET_MEMBER_NAME_CHECKED(ThisClass, LightSamplesQueue)) {
    LightSamplesSquared = FMath::Max(LightSamplesSquared, 1);
    LightSamplesQueue   = FMath::Max(LightSamplesQueue, 1);
    for (auto *l_un_reg : SceneComponentList_) {
      l_un_reg->UnregisterComponent();
      l_un_reg->DestroyComponent();
    }
    for (auto *l_un_reg : ArrowList_) {
      l_un_reg->UnregisterComponent();
      l_un_reg->DestroyComponent();
    }
    for (auto *l_un_reg : LightList_) {
      l_un_reg->UnregisterComponent();
      l_un_reg->DestroyComponent();
    }
    SceneComponentList_.Empty();
    ArrowList_.Empty();
    LightList_.Empty();
    if (Enabled) {
      CreateLightSqueue();
      SetLightAttr();
    }
  }
}
#endif  // WITH_EDITOR
