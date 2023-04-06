
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"  //定时器
#include "Animation/AnimInstance.h"

// 这个必须最后导入
#include "DoodleAnimBlendInstance.generated.h"

UCLASS(Blueprintable)
class DOODLE_API UDoodleAnimBlendInstance : public UAnimInstance {
 public:
  GENERATED_BODY()
  UDoodleAnimBlendInstance();

  // void NativeBeginPlay() override;
  // virtual void NativeInitializeAnimation() override;

  // void NativeUpdateAnimation(float DeltaTimeX) override;

  void DoodleCalculateSpeed();

  //~ Begin UAnimInstance Interface
  // virtual void NativeInitializeAnimation() override;
  // virtual void NativePostEvaluateAnimation() override;
  // virtual void OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) override;

 protected:
  // virtual void Montage_Advance(float DeltaTime) override;
  //~ End UAnimInstance Interface

  float VelocityAttr{0.f};

  int RandomAttr{};
  // UAnimInstance interface
  virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};