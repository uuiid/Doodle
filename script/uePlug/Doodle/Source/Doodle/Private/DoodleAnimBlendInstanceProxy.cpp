#include "DoodleAnimBlendInstanceProxy.h"

void FAnimBlendInstanceProxy::Initialize(UAnimInstance* InAnimInstance) {
  Super::Initialize(InAnimInstance);

  FAnimationInitializeContext L_InitContext{this};
  SingleNode_A.Initialize_AnyThread(this);
  SingleNode_B.Initialize_AnyThread(this);
  BlendNode.Initialize_AnyThread(this);
}

bool FAnimBlendInstanceProxy::Evaluate(FPoseContext& Output) {
  return true;
}

void FAnimBlendInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) {
}

void FAnimBlendInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const {
}

void FAnimBlendInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) {
}

void FAnimBlendInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance) {
}

void FAnimBlendInstanceProxy::ClearObjects() {
}
