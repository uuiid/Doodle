#include "Doodle/AiArrayGeneration.h"

#include "AI/NavigationSystemBase.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavigationSystem.h"
#include "Components/InstancedStaticMeshComponent.h"

ADoodleAiArrayGeneration::ADoodleAiArrayGeneration() {
  SplineComponent           = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
  SplineComponent->Mobility = EComponentMobility::Static;
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

  SceneComponentTarget         = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
  SceneComponentTarget->SetupAttachment(RootComponent);
  SceneComponentTarget->SetRelativeLocation(FVector{200.0, 0.0, 0.0});

  static ConstructorHelpers::FObjectFinder<UStaticMesh> CraneBaseMesh(TEXT("/ControlRig/Controls/ControlRig_Sphere_3mm.ControlRig_Sphere_3mm"));

  Target = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
  Target->SetupAttachment(SceneComponentTarget);

  Target->SetStaticMesh(CraneBaseMesh.Object);
  Target->SetIsVisualizationComponent(true);
  Target->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
  Target->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
  Target->SetCanEverAffectNavigation(false);
  Target->bHiddenInGame                = true;
  Target->CastShadow                   = false;

  Preview_InstancedStaticMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>("InstancedStaticMeshComponent");
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Preview_InstancedStaticMeshComponent_Mesh(TEXT("/Niagara/DefaultAssets/S_Gnomon.S_Gnomon"));
  Preview_InstancedStaticMeshComponent->SetStaticMesh(Preview_InstancedStaticMeshComponent_Mesh.Object);

  Preview_InstancedStaticMeshComponent->SetIsVisualizationComponent(true);
  Preview_InstancedStaticMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
  Preview_InstancedStaticMeshComponent->SetCanEverAffectNavigation(false);
  Preview_InstancedStaticMeshComponent->bHiddenInGame = true;
  Preview_InstancedStaticMeshComponent->CastShadow    = false;
}

void ADoodleAiArrayGeneration::Tick(float DeltaTime) {}

void ADoodleAiArrayGeneration::PostActorCreated() {
  Super::PostActorCreated();
  UMaterial* L_CraneBaseMaterial       = LoadObject<UMaterial>(this, TEXT("/ControlRig/Controls/ControlRigGizmoMaterial.ControlRigGizmoMaterial"));
  UMaterialInstanceDynamic* L_Material = UMaterialInstanceDynamic::Create(L_CraneBaseMaterial, this);
  L_Material->SetVectorParameterValue(TEXT("Color"), FColor::Red);
  Target->SetMaterial(0, L_Material);
  /// Script/Engine.StaticMesh'/Niagara/DefaultAssets/S_Gnomon.S_Gnomon'
}

void ADoodleAiArrayGeneration::GenPoint() {
  // RandomStream.Initialize();
  // RandomStream.RandRange(0, 10);
  Points.Empty(Row * Column);
  Preview_InstancedStaticMeshComponent->ClearInstances();
  FBox L_Box_                  = SplineComponent->GetLocalBounds().TransformBy(GetTransform()).GetBox();
  FVector L_Row_Step           = L_Box_.GetSize();
  double L_Row_Step_X          = L_Row_Step.X / FMath::Clamp((double)Row, 1.0, 1000.0);
  double L_Row_Step_Y          = L_Row_Step.Y / FMath::Clamp((double)Column, 1.0, 1000.0);
  double L_Max                 = L_Row_Step.GetAbsMax() / FMath::Clamp((double)FMath::Max(Row, Column), 1.0, 1000.0);

  const FVector L_Preview_Size = FVector{L_Max, L_Max, L_Max} / 20;
  UE_LOG(LogTemp, Log, TEXT("L_Preview_Size %s, L_Max %f"), *L_Preview_Size.ToString(), L_Max);

  // DrawDebugLine(GetWorld(), GetActorTransform().GetLocation(), GetActorTransform().GetLocation() + L_Vector * 50,
  // FColor::Red, false, 10.f);

  for (auto x = 0.0; x <= FMath::Clamp(Row, 1, 1000); ++x) {
    for (auto y = 0.0; y <= FMath::Clamp(Column, 1, 1000); ++y) {
      FVector L_Point       = L_Box_.Min + FVector{L_Row_Step_X * x, L_Row_Step_Y * y, 0};
      const float L_Param   = SplineComponent->FindInputKeyClosestToWorldLocation(L_Point);
      FVector L_RightVector = SplineComponent->GetRightVectorAtSplineInputKey(L_Param, ESplineCoordinateSpace::World);
      FVector L_Vector      = SplineComponent->GetLocationAtSplineInputKey(L_Param, ESplineCoordinateSpace::World) - L_Point;
      if (L_Vector.Dot(L_RightVector) < 0.0) {
        FHitResult L_HitR{};
        if (UKismetSystemLibrary::LineTraceSingleForObjects(
                GetWorld(), L_Point, L_Point - FVector{0, 0, 1000},
                {UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic)}, false, {},
                EDrawDebugTrace::None, L_HitR, true
            )) {
          FVector L_Point_Out{};
          // DrawDebugPoint(GetWorld(), L_HitR.ImpactPoint, 10.0f, FColor::Red, false, 1.0f);
          if (GetRandomPointInRadius(L_HitR.ImpactPoint, L_Point_Out)) {
            Points.Add(L_Point_Out);

            Preview_InstancedStaticMeshComponent->AddInstance(FTransform{FQuat::Identity, L_Point_Out, L_Preview_Size}, true);
            DrawDebugPoint(GetWorld(), L_Point_Out, 10.0f, FColor::Green, false, 1.0f);
          }
        }

        // DrawDebugPoint(GetWorld(), L_Point, 10.0f, FColor::Red, false, 1.0f);
      }
    }
  }
}

bool ADoodleAiArrayGeneration::GetRandomPointInRadius(const FVector& Origin, FVector& OutResult) {
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

void ADoodleAiArrayGeneration::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);

  GenPoint();
}
