#include "DoodleAnimBlendInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"

#include "DoodleAnimBlendInstanceProxy.h"
UDoodleAnimBlendInstance::UDoodleAnimBlendInstance() {
}

void UDoodleAnimBlendInstance::DoodleCalculateSpeed() {
}

FAnimInstanceProxy* UDoodleAnimBlendInstance::CreateAnimInstanceProxy() {
  return new FAnimBlendInstanceProxy(this);
}

//void UDoodleAnimBlendInstance::NativeInitializeAnimation() {
//  USkeletalMeshComponent* SkelComp = GetSkelMeshComponent();
//  SkelComp->AnimationData.Initialize(this);
//}
//
//void UDoodleAnimBlendInstance::NativePostEvaluateAnimation() {
//}
//
//void UDoodleAnimBlendInstance::OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) {
//}
//
//void UDoodleAnimBlendInstance::Montage_Advance(float DeltaTime) {
//}
