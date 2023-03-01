#include "DoodleCreateCharacterInstance.h"

#include "Animation/AnimNodeBase.h"

void UDoodleCreateCharacterInstance::SetCreateCharacterConfig(const TObjectPtr<UDoodleCreateCharacterConfig>& InConfig
) {
  CurrentConfig = InConfig;
}

FAnimInstanceProxy* UDoodleCreateCharacterInstance::CreateAnimInstanceProxy() {
  return new FDoodleCreateCharacterProxy{this};
}

void FDoodleCreateCharacterProxy::Initialize(UAnimInstance* InAnimInstance) {
  Super::Initialize(InAnimInstance);
  if (UDoodleCreateCharacterInstance* Instance = Cast<UDoodleCreateCharacterInstance>(GetAnimInstanceObject())) {
    instance = Instance;  // Cache for GC
  }
}

void FDoodleCreateCharacterProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) {
  Super::PreUpdate(InAnimInstance, DeltaSeconds);
}

bool FDoodleCreateCharacterProxy::Evaluate(FPoseContext& Output) {
  // Output.Curve.
  if (StoredTransforms.Num() == 0 && StoredCurves.Num() == 0) {
    return false;
  }

  FComponentSpacePoseContext CSContext(this);
  CSContext.Pose.InitPose(Output.Pose);

  TArray<FCompactPoseBoneIndex> ModifiedBones;
  for (TPair<int32, FTransform>& StoredTransform : StoredTransforms) {
    const int32 BoneIndexToModify = StoredTransform.Key;
    FCompactPoseBoneIndex CompactIndex =
        Output.Pose.GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(BoneIndexToModify);
    if (CompactIndex.GetInt() != INDEX_NONE) {
      CSContext.Pose.SetComponentSpaceTransform(CompactIndex, StoredTransform.Value);
      ModifiedBones.Add(CompactIndex);
    }
  }

  FCompactPose CompactPose(Output.Pose);
  FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(CSContext.Pose, CompactPose);

  // reset to ref pose before setting the pose to ensure if we don't have any missing bones
  Output.ResetToRefPose();

  for (const FCompactPoseBoneIndex& ModifiedBone : ModifiedBones) {
    Output.Pose[ModifiedBone] = CompactPose[ModifiedBone];
  }

  for (TPair<SmartName::UID_Type, float> Pair : StoredCurves) {
    Output.Curve.Set(Pair.Key, Pair.Value);
  }

  return true;
  // return Super::Evaluate(Output);
}

void FDoodleCreateCharacterProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) {
  Super::UpdateAnimationNode(InContext);
}
