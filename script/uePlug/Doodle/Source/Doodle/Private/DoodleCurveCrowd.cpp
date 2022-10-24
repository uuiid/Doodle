// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleCurveCrowd.h"
#include "AIController.h"

#include "Components/SplineComponent.h"
#include "AI/NavigationSystemBase.h"
#include "NavigationSystem.h"

#include "Animation/AnimSingleNodeInstance.h"          //动画实例
#include "GameFramework/CharacterMovementComponent.h"  //角色移动组件
#include "DoodleAIController.h"
// Sets default values
ADoodleCurveCrowd::ADoodleCurveCrowd()
    : ACharacter() {
  // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

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
void ADoodleCurveCrowd::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  // USkeletalMeshComponent *SkeletalMeshComponent = FindComponentByClass<USkeletalMeshComponent>();
  // if (!SkeletalMeshComponent)
  //   return;
  // auto Anim = Cast<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance());
  // if (Anim) {
  //   auto Velocity = GetVelocity();
  //   FVector Blend{Velocity.Size(), Anim->CalculateDirection(Velocity, GetBaseAimRotation()), 0.0f};
  //   Anim->SetBlendSpaceInput(Blend);
  // }
}
