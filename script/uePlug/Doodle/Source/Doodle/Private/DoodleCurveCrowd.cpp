// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleCurveCrowd.h"
#include "AIController.h"

#include "Components/SplineComponent.h"
#include "AI/NavigationSystemBase.h"
#include "NavigationSystem.h"

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

  p_spline = CreateDefaultSubobject<USplineComponent>("Spline");
  p_spline->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
  p_spline->SetWorldTransform(GetRootComponent()->GetComponentTransform());
}

// Called when the game starts or when spawned
void ADoodleCurveCrowd::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void ADoodleCurveCrowd::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
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
