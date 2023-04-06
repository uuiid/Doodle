
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"  //定时器
#include "Animation/AnimInstance.h"

#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"

// 这个必须最后导入
#include "DoodleAnimBlendInstanceProxy.generated.h"

USTRUCT()
struct FAnimBlendInstanceProxy : public FAnimInstanceProxy {
  GENERATED_BODY()
  FAnimBlendInstanceProxy(){};
  FAnimBlendInstanceProxy(UAnimInstance* InAnimInstance)
      : FAnimInstanceProxy(InAnimInstance),
        SingleNode_A(),
        SingleNode_B(),
        BlendNode() {}

  FAnimNode_SingleNode SingleNode_A;
  FAnimNode_SingleNode SingleNode_B;
  FAnimNode_TwoWayBlend BlendNode;

  virtual void Initialize(UAnimInstance* InAnimInstance) override;
  virtual bool Evaluate(FPoseContext& Output) override;
  virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
  virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;
  virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
  virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
  virtual void ClearObjects() override;
};