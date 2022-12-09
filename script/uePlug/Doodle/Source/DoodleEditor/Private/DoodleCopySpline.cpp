// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleCopySpline.h"

#include "Components/SplineComponent.h"

// 这个是实例集合体, 用来做预览
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
// Sets default values
ADoodleCopySpline::ADoodleCopySpline()
    : Super(),
      p_preview(),
      p_preview_mesh(),
      p_actor(),
      p_lock_at_actor(),
      p_number(1),
      p_spline() {
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = false;
  auto k_rootComponent          = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
  k_rootComponent->SetMobility(EComponentMobility::Stationary);
  k_rootComponent->SetupAttachment(RootComponent);

  SetRootComponent(k_rootComponent);
  // 添加预览网格体 这里我们在游戏时不编译
  //  p_preview_mesh = CreateDefaultSubobject<UStaticMesh>("preview_mesh");
  static ConstructorHelpers::FObjectFinder<UStaticMesh> k_mesh(TEXT("/Doodle/Doodle_indicator"));
  p_preview_mesh = k_mesh.Object;

  // 添加实例化集合体组件
  p_preview      = CreateDefaultSubobject<UInstancedStaticMeshComponent>("preview");
  // p_preview->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
  p_preview->SetupAttachment(RootComponent);
  p_preview->SetStaticMesh(p_preview_mesh);

  // 添加曲线
  p_spline = CreateDefaultSubobject<USplineComponent>("Spline");
  p_spline->SetupAttachment(RootComponent);
  p_spline->SetWorldTransform(GetRootComponent()->GetComponentTransform());
}

#if WITH_EDITOR

void ADoodleCopySpline::PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangeEvent) {
  Super::PostEditChangeProperty(PropertyChangeEvent);

  if (!p_actor)
    return;
  if (!p_lock_at_actor)
    return;

  auto name = PropertyChangeEvent.MemberProperty ? PropertyChangeEvent.MemberProperty->GetFName() : NAME_None;
  UE_LOG(LogTemp, Log, TEXT("%s"), *(name.ToString()));
  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_spline) ||  //
      name == GET_MEMBER_NAME_CHECKED(ThisClass, p_number)) {
    auto k_length   = p_spline->GetSplineLength();
    auto k_segments = k_length / (float)p_number;
    auto k_word     = GetWorld();
    auto k_lock_at  = p_lock_at_actor->GetActorLocation();
    p_preview->ClearInstances();

    // 循环添加静态网格体
    for (int i = 0; i < p_number; ++i) {
      auto k_ver = p_spline->GetWorldLocationAtDistanceAlongSpline((float)i * k_segments);
      auto k_tmp = (k_lock_at - k_ver);
      k_tmp.Z    = 0;
      auto k_rot = k_tmp.Rotation();

      FTransform k_tran_{k_rot, k_ver, p_actor->GetActorScale()};
      // 添加预览网格体
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27
      p_preview->AddInstanceWorldSpace(k_tran_);
#else if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
      p_preview->AddInstance(k_tran_, true);
#endif
    }
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_preview_mesh)) {
    p_preview->SetStaticMesh(p_preview_mesh);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, p_lock_at_actor)) {
    auto k_num      = p_preview->GetInstanceCount();
    auto k_length   = p_spline->GetSplineLength();
    auto k_segments = k_length / (float)p_number;
    auto k_lock_at  = p_lock_at_actor->GetActorLocation();
    for (int i = 0; i < k_num; ++i) {
      auto k_ver = p_spline->GetWorldLocationAtDistanceAlongSpline((float)i * k_segments);
      auto k_tmp = (k_lock_at - k_ver);
      k_tmp.Z    = 0;
      auto k_rot = k_tmp.Rotation();

      FTransform k_tran_{k_rot, k_ver, p_actor->GetActorScale()};
      p_preview->UpdateInstanceTransform(i, k_tran_, true);
    }
  }
}
#endif  // WITH_EDITOR
// void ADoodleCopySpline::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangeEvent) {
//   Super::PostEditChangeProperty(PropertyChangeEvent);
//   if (!p_actor) return;
//   if (!p_lock_at_actor) return;
//   auto name = PropertyChangeEvent.MemberProperty ? PropertyChangeEvent.MemberProperty->GetFName() : NAME_None;
//   UE_LOG(LogTemp, Log, TEXT("%s"), *(name.ToString()));
// }

// void ADoodleCopySpline::PostInterpChange(FProperty* PropertyThatChanged) {
//   auto name = PropertyThatChanged ? PropertyThatChanged->GetFName() : NAME_None;
//   UE_LOG(LogTemp, Log, TEXT("%s"), *(name.ToString()));
// }

// Called when the game starts or when spawned
void ADoodleCopySpline::BeginPlay() {
  Super::BeginPlay();
}

// Called every frame
void ADoodleCopySpline::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
}

void ADoodleCopySpline::CopyAActore() {
  // auto test = NewObject<UInstancedStaticMeshComponent>(this, "preview");
  // test->SetStaticMesh();

  if (!p_actor)
    return;
  if (!p_lock_at_actor)
    return;

  auto k_length   = p_spline->GetSplineLength();
  auto k_segments = k_length / (float)p_number;
  auto k_word     = GetWorld();
  auto k_lock_at  = p_lock_at_actor->GetActorLocation();

  for (int i = 0; i < p_number; ++i) {
    auto k_ver = p_spline->GetWorldLocationAtDistanceAlongSpline((float)i * k_segments);
    auto k_tmp = (k_lock_at - k_ver);
    k_tmp.Z    = 0;
    auto k_rot = k_tmp.Rotation();

    FTransform k_tran_{k_rot, k_ver, p_actor->GetActorScale()};
    UE_LOG(LogTemp, Log, TEXT("%s"), *(k_tran_.ToString()));
    FActorSpawnParameters k_para{};
    // k_para.Name     = FName{p_actor->GetName() + FString::FromInt(i)};
    k_para.Template = p_actor;

    auto k_actor    = k_word->SpawnActor(p_actor->GetClass(), &k_tran_, k_para);
    if (!k_actor)
      continue;
    k_actor->SetActorTransform(k_tran_);
    // 添加预览网格体
  }
}
