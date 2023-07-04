#include "Doodle/AiArrayGenerationMoveSpline.h"

#include "AI/NavigationSystemBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Doodle/DoodleEigenHelper.h"
#include "DoodleAiCrowd.h"
#include "DoodleAiMoveToComponent.h"
#include "DoodleAiSplineCrowd.h"
#include "DoodleAiSplineMoveToCom.h"
#include "GameFramework/CharacterMovementComponent.h"  //角色移动组件
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

ADoodleAiArrayGenerationMoveSpline::ADoodleAiArrayGenerationMoveSpline() {
  PrimaryActorTick.bCanEverTick = true;
  SplineComponent               = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
  SplineComponent->Mobility     = EComponentMobility::Movable;
  SplineComponent->SetGenerateOverlapEvents(false);
  SplineComponent->SetCanEverAffectNavigation(false);
  SplineComponent->SetClosedLoop(true);

  SplineComponent->SetSplinePoints(
      {FVector{100, 0, 0}, FVector{0, 100, 0}, FVector{-100, 0, 0}, FVector{0, -100, 0}}, ESplineCoordinateSpace::World
  );

  RootComponent = SplineComponent;
  SetHidden(true);
  SetCanBeDamaged(false);
  bCollideWhenPlacing          = true;
  SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
  Row                          = 10;
  Column                       = 10;

  TargetSpline                 = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponentPath"));
  TargetSpline->SetupAttachment(RootComponent);
#if WITH_EDITOR
  TargetSpline->SetIsVisualizationComponent(true);
#endif
  TargetSpline->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
  TargetSpline->SetCanEverAffectNavigation(false);
  TargetSpline->bHiddenInGame          = true;
  TargetSpline->CastShadow             = false;

  Preview_InstancedStaticMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>("InstancedStaticMeshComponent");
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Preview_InstancedStaticMeshComponent_Mesh(TEXT("/Niagara/DefaultAssets/S_Gnomon.S_Gnomon"));
  Preview_Mesh = Preview_InstancedStaticMeshComponent_Mesh.Object;
  Preview_InstancedStaticMeshComponent->SetStaticMesh(Preview_InstancedStaticMeshComponent_Mesh.Object);
#if WITH_EDITOR
  Preview_InstancedStaticMeshComponent->SetIsVisualizationComponent(true);
#endif
  Preview_InstancedStaticMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
  Preview_InstancedStaticMeshComponent->SetCanEverAffectNavigation(false);
  Preview_InstancedStaticMeshComponent->bHiddenInGame = true;
  Preview_InstancedStaticMeshComponent->CastShadow    = false;

  RandomAnimSpeed                                     = {150.0f, 250.0f};
  MaxAcceleration                                     = 300.0f;
}

void ADoodleAiArrayGenerationMoveSpline::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
}

void ADoodleAiArrayGenerationMoveSpline::PostActorCreated() {
  Super::PostActorCreated();
  UMaterial* L_CraneBaseMaterial       = LoadObject<UMaterial>(this, TEXT("/ControlRig/Controls/ControlRigGizmoMaterial.ControlRigGizmoMaterial"));
  UMaterialInstanceDynamic* L_Material = UMaterialInstanceDynamic::Create(L_CraneBaseMaterial, this);
  L_Material->SetVectorParameterValue(TEXT("Color"), FColor::Red);
  // GenPoint();
}

bool ADoodleAiArrayGenerationMoveSpline::ShouldTickIfViewportsOnly() const {
  return true;
}

void ADoodleAiArrayGenerationMoveSpline::BeginPlay() {
  if (AnimAssets.Num() == 0 || SkinAssets.Num() == 0) return;

  int32 L_Max_Skin = FMath::Max(0, SkinAssets.Num() - 1);
  TMap<USkeleton*, TArray<UAnimationAsset*>> L_Map{};
  for (auto&& i : AnimAssets) {
    if (i && i->GetSkeleton()) {
      if (!L_Map.Find(i->GetSkeleton())) {
        L_Map.Add(i->GetSkeleton(), TArray<UAnimationAsset*>{});
      }
      L_Map[i->GetSkeleton()].Add(i);
    }
  }
  FActorSpawnParameters L_ActorSpawnParameters{};
  L_ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
  for (auto&& i : Points) {
    auto L_Loc = i.GetLocation();
    // L_Loc.Z += 70;
    ADoodleAiSplineCrowd* L_Actor =
        GetWorld()->SpawnActor<ADoodleAiSplineCrowd>(L_Loc, i.GetRotation().Rotator(), L_ActorSpawnParameters);
    UDoodleAiMoveToComponent* L_Com = L_Actor->GetDoodleMoveToComponent();
    if (L_Com) {
      L_Com->RandomRadius = RandomRadius_Move;
    }
    auto L_tran = i;
    L_tran.SetScale3D(FVector::OneVector);
    L_tran.SetRotation(GetActorQuat());
    L_Actor->SplineMoveToComponent->ReplaceSplineCurve(TargetSpline, L_tran);

    USkeletalMesh* L_Skin = SkinAssets[RandomStream_Skin.RandRange(0, L_Max_Skin)];
    auto L_Array          = L_Map.Find(L_Skin->GetSkeleton());
    if (!L_Array) continue;
    if (L_Array->Num() != 0) continue;
    UAnimationAsset* L_Anim          = (*L_Array)[RandomStream_Anim.RandRange(0, L_Array->Num() - 1)];
    USkeletalMeshComponent* L_Sk_Com = L_Actor->GetMesh();
    FVector::ZAxisVector;
    L_Sk_Com->SetRelativeLocationAndRotation({0.f, 0.f, -85.f}, FQuat{FVector::ZAxisVector, -90.f});
    L_Sk_Com->SetSkeletalMesh(L_Skin);
    L_Sk_Com->PlayAnimation(L_Anim, true);
    // L_Sk_Com->LightingChannels = LightingChannels;
    L_Sk_Com->SetLightingChannels(LightingChannels.bChannel0, LightingChannels.bChannel1, LightingChannels.bChannel2);

    UCharacterMovementComponent* CharacterMovementComponent =
        Cast<UCharacterMovementComponent>(L_Actor->GetMovementComponent());
    if (CharacterMovementComponent) {
      CharacterMovementComponent->MaxAcceleration = MaxAcceleration;
      CharacterMovementComponent->MaxWalkSpeed    = RandomStream_Anim.RandRange(RandomAnimSpeed.X, RandomAnimSpeed.Y);
      CharacterMovementComponent->GroundFriction  = 0.2f;
      CharacterMovementComponent->RotationRate    = {0.0f, 180.0f, 0.0f};
      CharacterMovementComponent->bOrientRotationToMovement    = true;
      CharacterMovementComponent->bUseRVOAvoidance             = true;
      CharacterMovementComponent->AvoidanceConsiderationRadius = 100.0f;
      CharacterMovementComponent->AvoidanceWeight              = 5.0f;
    }
  }
}

void ADoodleAiArrayGenerationMoveSpline::GenPoint() {
  UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (!NavSys) {
    return;
  }
  if (NavSys->MainNavData == nullptr) return;
  // RandomStream.Initialize();
  // RandomStream.RandRange(0, 10);
  Points.Empty(Row * Column);
  Preview_InstancedStaticMeshComponent->ClearInstances();
  Preview_InstancedStaticMeshComponent->SetStaticMesh(Preview_Mesh);

  FBox L_Box_        = SplineComponent->Bounds.GetBox();
  FVector L_Row_Step = L_Box_.GetSize();
  float L_Row_Step_X = L_Row_Step.X / FMath::Clamp((float)Row, 1.0f, 1000.0f);
  float L_Row_Step_Y = L_Row_Step.Y / FMath::Clamp((float)Column, 1.0f, 1000.0f);

  // DrawDebugLine(GetWorld(), GetActorTransform().GetLocation(), GetActorTransform().GetLocation() + L_Vector * 50,
  // FColor::Red, false, 10.f);

  for (auto x = 0.0f; x <= FMath::Clamp(Row, 1, 1000); ++x) {
    for (auto y = 0.0f; y <= FMath::Clamp(Column, 1, 1000); ++y) {
      FVector L_Point     = L_Box_.Min + FVector{L_Row_Step_X * x, L_Row_Step_Y * y, 0};
      const float L_Param = SplineComponent->FindInputKeyClosestToWorldLocation(L_Point);
      FVector2D L_RightVector_2D{
          SplineComponent->GetRightVectorAtSplineInputKey(L_Param, ESplineCoordinateSpace::World)};
      L_RightVector_2D.Normalize();
      FVector2D L_Vector_2D{
          SplineComponent->GetLocationAtSplineInputKey(L_Param, ESplineCoordinateSpace::World) - L_Point};
      L_Vector_2D.Normalize();
      if (FVector2D::DotProduct(L_Vector_2D, L_RightVector_2D) < 0.0) {
        FHitResult L_HitR{};
        if (UKismetSystemLibrary::LineTraceSingleForObjects(
                GetWorld(), L_Point + FVector{0, 0, 1000}, L_Point - FVector{0, 0, 1000},
                {UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic)}, false, {},
                EDrawDebugTrace::None, L_HitR, true
            )) {
          FVector L_Point_Out{};
          // DrawDebugPoint(GetWorld(), L_HitR.ImpactPoint, 10.0f, FColor::Red, false, 1.0f);
          if (GetRandomPointInRadius(L_HitR.ImpactPoint, L_Point_Out)) {
            L_Point_Out.Z += OffsetValue;
            // TargetSpline->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::Type::World).ToOrientationQuat();
            FTransform L_Ftran{TargetSpline->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::Type::World).ToOrientationQuat(), L_Point_Out};
            Points.Add(L_Ftran);

            Preview_InstancedStaticMeshComponent->AddInstanceWorldSpace(L_Ftran);
            // DrawDebugPoint(GetWorld(), L_Point_Out, 10.0f, FColor::Green, false, 1.0f);
          }
        }

        // DrawDebugPoint(GetWorld(), L_Point, 10.0f, FColor::Red, false, 1.0f);
      }
    }
  }
}

bool ADoodleAiArrayGenerationMoveSpline::GetRandomPointInRadius(const FVector& Origin, FVector& OutResult) {
  UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (!NavSys) {
    return false;
  }

  FNavLocation Result;
  bool bSuccess = NavSys->GetRandomPointInNavigableRadius(Origin, RandomRadius, Result);
  // Out
  OutResult     = Result;

  return bSuccess;
}

void ADoodleAiArrayGenerationMoveSpline::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);
  // if (SplineComponent->bSplineHasBeenEdited)
  GenPoint();

  Preview_InstancedStaticMeshComponent->SetLightingChannels(LightingChannels.bChannel0, LightingChannels.bChannel1, LightingChannels.bChannel2);
}
