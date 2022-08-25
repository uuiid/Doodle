#include "DoodleSurroundMesh.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"

ADoodleSurroundMeshActor::ADoodleSurroundMeshActor()
    : Super(),
      p_count(10) {
  PrimaryActorTick.bCanEverTick = true;

  auto k_rootComponent          = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  k_rootComponent->SetMobility(EComponentMobility::Movable);
  k_rootComponent->SetupAttachment(RootComponent);

  SetRootComponent(k_rootComponent);
  ConstructorHelpers::FObjectFinder<UStaticMesh> k_mesh(
      TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder")
  );

  p_mesh      = k_mesh.Object;

  p_instanced = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
      "InstancedStaticMeshComponent"
  );
  p_instanced->SetupAttachment(GetRootComponent());

  p_spline = CreateDefaultSubobject<USplineComponent>("SplineComponent");
  p_spline->SetupAttachment(GetRootComponent());
  // p_spline->bAllowDiscontinuousSpline = false;
  p_spline->SetClosedLoop(true);

  p_spline->ClearSplinePoints(false);

  p_spline->AddSplineLocalPoint({1000, 0, 0});
  p_spline->AddSplineLocalPoint({0, 1000, 0});
  p_spline->AddSplineLocalPoint({-1000, 0, 0});
  p_spline->AddSplineLocalPoint({0, -1000, 0});
}

#if WITH_EDITOR
void ADoodleSurroundMeshActor::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangeEvent
) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  auto name = PropertyChangeEvent.MemberProperty ? PropertyChangeEvent.MemberProperty->GetFName() : NAME_None;
  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_count)) {
    SetSurroundMesh();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_vector) || name == GET_MEMBER_NAME_CHECKED(ThisClass, vectorMax) || name == GET_MEMBER_NAME_CHECKED(ThisClass, vectorMin)) {
    SetVector();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, radius)) {
    SetRadius();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, size)) {
    SetSize();
  }
}
#endif  // WITH_EDITOR
void ADoodleSurroundMeshActor::PostLoad() {
  Super::PostLoad();
}
void ADoodleSurroundMeshActor::OnConstruction(
    const FTransform &Transform
) {
  Super::OnConstruction(Transform);
  p_instanced->SetStaticMesh(p_mesh);

  SetSurroundMesh();
  SetVector();
  SetRadius();
  SetSize();
}
bool ADoodleSurroundMeshActor::ShouldTickIfViewportsOnly() const { return true; }
void ADoodleSurroundMeshActor::BeginPlay() {
  Super::BeginPlay();
}
void ADoodleSurroundMeshActor::SetSurroundMesh() {
  p_lens.Empty();

  p_instanced->ClearInstances();
  auto k_length   = p_spline->GetSplineLength();
  auto k_segments = k_length / (float)p_count;

  FTransformArrayA2 l_trans{};
  // 循环添加静态网格体
  for (int i = 0; i < p_count; ++i) {
    auto l_tran = p_spline->GetTransformAtDistanceAlongSpline((float)i * k_segments, ESplineCoordinateSpace::Type::Local, false);
    l_trans.Add(l_tran);
    p_lens.Add((float)i * k_segments);
  }
  // 添加预览网格体
  p_instanced->AddInstances(l_trans, false);
}

void ADoodleSurroundMeshActor::SetVector() {
  p_vector_list.Empty();
  // 循环添加静态网格体
  for (int i = 0; i < p_count; ++i) {
    // 生成噪波函数
    auto noise = FMath::RandRange(vectorMin, vectorMax);
    p_vector_list.Add(noise);
  }
}

void ADoodleSurroundMeshActor::SetRadius() {
  p_radius_list.Empty();
  for (int i = 0; i < p_count; ++i) {
    p_radius_list.Add(FMath::VRand() * radius);
  }
}

void ADoodleSurroundMeshActor::SetSize() {
  p_size_list.Empty();
  for (int i = 0; i < p_count; ++i) {
    auto noise = FMath::RandRange(size_min, size_max);
    p_size_list.Add(FVector{noise * size, noise * size, noise * size});
  }
}

void ADoodleSurroundMeshActor::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  auto time     = GetGameTimeSinceCreation();
  auto k_length = p_spline->GetSplineLength();

  for (int l_i = 0; l_i < p_count; ++l_i) {
    auto &&i     = p_lens[l_i];
    auto &&noise = p_vector_list[l_i];

    i += (DeltaTime * (p_vector + noise));
    i = i > k_length ? (i - k_length) : i;
  }
  FTransformArrayA2 l_trans{};
  // 循环添加静态网格体
  for (int i = 0; i < p_count; ++i) {
    auto l_tran  = p_spline->GetTransformAtDistanceAlongSpline(p_lens[i], ESplineCoordinateSpace::Type::Local, false);
    auto noise_x = FMath::PerlinNoise1D(time * p_radius_vector + p_radius_list[i].X) * radius;
    auto noise_y = FMath::PerlinNoise1D(time * p_radius_vector + p_radius_list[i].Y) * radius;
    auto noise_z = FMath::PerlinNoise1D(time * p_radius_vector + p_radius_list[i].Z) * radius;
    l_tran.AddToTranslation(p_radius_list[i]);
    l_tran.AddToTranslation(FVector{noise_x, noise_y, noise_z});
    l_tran.SetScale3D(p_size_list[i]);
    // auto noise_r_x = FMath::PerlinNoise1D(time * rot_size + p_radius_list[i].X) * 180;
    // auto noise_r_y = FMath::PerlinNoise1D(time * rot_size + p_radius_list[i].Y) * 180;
    // auto noise_r_z = FMath::PerlinNoise1D(time * rot_size + p_radius_list[i].Z) * 180;

    // l_tran.SetRotation(FVector{noise_r_x,
    //                            noise_r_y,
    //                            noise_r_z}
    //                        .ToOrientationQuat());

    l_trans.Add(l_tran);
  }
  // 添加预览网格体
  p_instanced->BatchUpdateInstancesTransforms(0, l_trans, false, true, false);
}
