#include "DoodleAiSplineCrowd.h"
#include "DoodleAiSplineMoveToCom.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
ADoodleAiSplineCrowd::ADoodleAiSplineCrowd() {
  SplineMoveToComponent = CreateDefaultSubobject<UDoodleAiSplineMoveToComponent>("DoodleAiSplineMoveToComponent");
  SplineMoveToComponent->AttachToComponent(
      GetRootComponent(),
      FAttachmentTransformRules::KeepRelativeTransform
  );
  SplineMoveToComponent->SetRelativeLocation(FVector{0.f, 0.f, -85.f});

  SplineMoveToComponent->SetClosedLoop(true);
  UCharacterMovementComponent* L_Move = Cast<UCharacterMovementComponent>(GetMovementComponent());
  if (L_Move) {
    L_Move->MaxWalkSpeed               = 120.0f;
    L_Move->BrakingDecelerationWalking = 10.f;
  }
  USkeletalMeshComponent* SkeletalMeshComponent = FindComponentByClass<USkeletalMeshComponent>();
  if (!SkeletalMeshComponent)
    return;

  SkeletalMeshComponent->SetRelativeLocation(FVector{0.000000, 0.000000, -80.000000});
  SkeletalMeshComponent->SetRelativeRotation(FRotator{0.000000, -90.000000, 0.000000});


}

void ADoodleAiSplineCrowd::BeginPlay() {
  TranRoot = GetActorTransform();
  // FDetachmentTransformRules L_Rules{EDetachmentRule::KeepWzorld, true};
  // SplineMoveToComponent->DetachFromComponent(L_Rules);
  // SplineMoveToComponent->AttachToComponent(
  //     GetRootComponent(),
  //     FAttachmentTransformRules::KeepWorldTransform
  //);
  SplineMoveToComponent->SetAbsolute(true, true, true);
  SplineMoveToComponent->SetWorldTransform(TranRoot);
  Super::BeginPlay();
}

void ADoodleAiSplineCrowd::Tick(float DeltaTime) {
  // SetActorTransform(TranRoot);
  Super::Tick(DeltaTime);

  USkeletalMeshComponent* SkeletalMeshComponent = FindComponentByClass<USkeletalMeshComponent>();
  if (!SkeletalMeshComponent)
    return;
  auto Anim = Cast<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance());
  if (!Anim)
    return;

  Anim->SetBlendSpacePosition(FVector{this->GetVelocity().Size(), .0f, .0f});
}
