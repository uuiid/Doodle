#include "DoodleCreateCharacterInstance.h"

void UDoodleCreateCharacterInstance::SetCreateCharacterConfig(const TObjectPtr<UDoodleCreateCharacterConfig>& InConfig
) {
  CurrentConfig = InConfig;
}

FAnimInstanceProxy* UDoodleCreateCharacterInstance::CreateAnimInstanceProxy() {
  return new FDoodleCreateCharacterProxy{this};
}

void FDoodleCreateCharacterProxy::Initialize(UAnimInstance* InAnimInstance) {}

void FDoodleCreateCharacterProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) {}

bool FDoodleCreateCharacterProxy::Evaluate(FPoseContext& Output) { return false; }

void FDoodleCreateCharacterProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) {}
