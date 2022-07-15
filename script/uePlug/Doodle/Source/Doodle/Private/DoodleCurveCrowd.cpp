// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleCurveCrowd.h"
#include "AIController.h"

#include "Components/SplineComponent.h"
#include "AI/NavigationSystemBase.h"
#include "NavigationSystem.h"

#include "Animation/AnimSingleNodeInstance.h"         //动画实例
#include "GameFramework/CharacterMovementComponent.h" //角色移动组件
#include "DoodleAIController.h"
// Sets default values
ADoodleCurveCrowd::ADoodleCurveCrowd()
    : ACharacter()
{
  // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  // p_spline = NewObject<USplineComponent>(this, "Spline");
  // p_spline->RegisterComponent();
  // p_spline->SetWorldTransform(GetRootComponent()->GetComponentTransform());

  // p_instanced = NewObject<UInstancedStaticMeshComponent>(this, "InstancedStaticMeshComponent");
  // p_instanced->RegisterComponent();
  // p_instanced->SetWorldTransform(GetRootComponent()->GetComponentTransform());

  AIControllerClass = ADoodleAIController::StaticClass();

  UCharacterMovementComponent *CharacterMovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());

  if (CharacterMovementComponent)
  {
    CharacterMovementComponent->MaxAcceleration = 150.f;
    CharacterMovementComponent->MaxWalkSpeed = 150.f;
    CharacterMovementComponent->GroundFriction = 1.f;
    CharacterMovementComponent->RotationRate = {0.0f, 0.0f, 180.0f};
    CharacterMovementComponent->bOrientRotationToMovement = true;

    CharacterMovementComponent->bUseRVOAvoidance = true;
    CharacterMovementComponent->AvoidanceConsiderationRadius = 100.0f;
    CharacterMovementComponent->AvoidanceWeight = 5.0f;
  }
}

// Called when the game starts or when spawned
void ADoodleCurveCrowd::BeginPlay()
{
  Super::BeginPlay();
  // USkeletalMeshComponent *SkeletalMeshComponent = FindComponentByClass<USkeletalMeshComponent>();
  // if (!SkeletalMeshComponent)
  //   return;
  // SkeletalMeshComponent->SetAnimationMode(EAnimationMode::Type::AnimationBlueprint);
  // SkeletalMeshComponent->SetAnimInstanceClass(UAnimSingleNodeInstance::StaticClass());
  // auto Anim = CastChecked<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance());
  // if (AnimationAsset)
  // {
  //   Anim->SetAnimationAsset(AnimationAsset);
  //   Anim->PlayAnim(true, 1.f, 0.f);
  // }
}

// Called every frame
void ADoodleCurveCrowd::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
  USkeletalMeshComponent *SkeletalMeshComponent = FindComponentByClass<USkeletalMeshComponent>();
  if (!SkeletalMeshComponent)
    return;
  // auto Anim = CastChecked<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance());
  // Anim->SetBlendSpaceInput(FVector{GetVelocity().Size(), 0.0f, 0.0f});
  // auto controller = Cast<AAIController>(GetController());
  // UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  // FVector Result;
  // bool bSuccess = NavSys->K2_GetRandomReachablePointInRadius(GetWorld(), GetActorLocation(), Result, 600);
  // if (controller && bSuccess)
  // {
  //   FAIMoveRequest l_m_q{Result};
  //   controller->MoveTo(l_m_q);
  // }
}
// Called to bind functionality to input
void ADoodleCurveCrowd::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);
}
