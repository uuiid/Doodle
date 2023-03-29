#include "DoodleAiDownToComponent.h"
#include "DoodleAnimInstance.h"
UDoodleAiDownToComponent::UDoodleAiDownToComponent(const FObjectInitializer& ObjectInitializer)
    : UActorComponent(ObjectInitializer) {
  PrimaryComponentTick.bCanEverTick = true;

  auto k_curve                      = CrouchCurve.GetRichCurve();
  k_curve->AutoSetTangents();
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.0f, 0.0f), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(0.5f, 70.0f), ERichCurveTangentMode::RCTM_Auto);
  k_curve->SetKeyTangentMode(k_curve->AddKey(1.0f, 100.0f), ERichCurveTangentMode::RCTM_Auto);
}

void UDoodleAiDownToComponent::BeginPlay() {
  if (auto L_W = GetWorld())
    L_W->GetTimerManager().SetTimer(TimerHandle, this, &UDoodleAiDownToComponent::On_Crouch, CrouchBegineTime + FMath::RandRange(-RandomRadius, RandomRadius), false);
  PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UDoodleAiDownToComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
  if (!Begine_Crouch_B)
    return;

  AActor* Actor = GetOwner<AActor>();
  if (!Actor)
    return;
  USkeletalMeshComponent* SkeletalMeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>();
  if (!SkeletalMeshComponent)
    return;
  auto Anim = Cast<UDoodleAnimInstance>(SkeletalMeshComponent->GetAnimInstance());
  if (!Anim)
    return;

  auto L_Down = CrouchCurve.GetRichCurve()->Eval(Buffer_Time);
  Buffer_Time += DeltaTime;
  Anim->SetDirectionAttrZ(L_Down);
}

void UDoodleAiDownToComponent::On_Crouch() {
  Begine_Crouch_B = true;
  PrimaryComponentTick.SetTickFunctionEnable(true);
}
