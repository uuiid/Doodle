// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleAiCrowd.h"

#include "AI/NavigationSystemBase.h"
#include "AIController.h"
#include "Animation/AnimSingleNodeInstance.h"  //动画实例
#include "Components/SplineComponent.h"
#include "DoodleAIController.h"
#include "DoodleAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"  //角色移动组件
#include "NavigationSystem.h"
#include "DoodleAiDownToComponent.h"
#include "DoodleAiMoveToComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
ADoodleAiCrowd::ADoodleAiCrowd() : ACharacter() {
  // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
  AIControllerClass             = ADoodleAIController::StaticClass();
  AutoPossessAI                 = EAutoPossessAI::PlacedInWorldOrSpawned;
  MoveToCom                     = CreateDefaultSubobject<UDoodleAiMoveToComponent>(TEXT("DoodleAiMoveToComponent"));

  // p_spline = NewObject<USplineComponent>(this, "Spline");
  // p_spline->RegisterComponent();
  // p_spline->SetWorldTransform(GetRootComponent()->GetComponentTransform());

  // p_instanced = NewObject<UInstancedStaticMeshComponent>(this, "InstancedStaticMeshComponent");
  // p_instanced->RegisterComponent();
  // p_instanced->SetWorldTransform(GetRootComponent()->GetComponentTransform());

  /// 调整默认值
  // UCharacterMovementComponent *CharacterMovementComponent =
  // Cast<UCharacterMovementComponent>(GetMovementComponent()); if (CharacterMovementComponent) {
  //   CharacterMovementComponent->MaxAcceleration              = 150.f;
  //   CharacterMovementComponent->MaxWalkSpeed                 = 200.f;
  //   CharacterMovementComponent->GroundFriction               = 0.2f;
  //   CharacterMovementComponent->RotationRate                 = {0.0f, 180.0f, 0.0f};
  //   CharacterMovementComponent->bOrientRotationToMovement    = true;
  //   CharacterMovementComponent->bUseRVOAvoidance             = true;
  //   CharacterMovementComponent->AvoidanceConsiderationRadius = 100.0f;
  //   CharacterMovementComponent->AvoidanceWeight              = 5.0f;
  // }
}

// Called every frame
void ADoodleAiCrowd::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
  if (!SkeletalMeshComponent)
    return;

  auto Anim = Cast<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance());
  if (!Anim)
    return;
  Anim->SetBlendSpacePosition(FVector{this->GetVelocity().Size(), .0f, .0f});
}
