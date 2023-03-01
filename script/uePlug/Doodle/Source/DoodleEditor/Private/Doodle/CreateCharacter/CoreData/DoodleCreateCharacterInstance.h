#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CoreMinimal.h"
#include "DoodleCreateCharacterInstance.generated.h"

class UDoodleCreateCharacterConfig;
class UDoodleCreateCharacterInstance;

/** 评估动画类 */
USTRUCT()
struct FDoodleCreateCharacterProxy : public FAnimInstanceProxy {
 public:
  GENERATED_BODY()

  FDoodleCreateCharacterProxy() {}

  FDoodleCreateCharacterProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

  virtual void Initialize(UAnimInstance* InAnimInstance) override;
  virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
  virtual bool Evaluate(FPoseContext& Output) override;
  virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;

  TMap<int32, FTransform> StoredTransforms;
  TMap<SmartName::UID_Type, float> StoredCurves;

 private:
  UDoodleCreateCharacterInstance* instance;
};

UCLASS(transient, NotBlueprintable)
class UDoodleCreateCharacterInstance : public UAnimInstance {
  GENERATED_BODY()
 public:
  void SetCreateCharacterConfig(const TObjectPtr<UDoodleCreateCharacterConfig>& InConfig);

  FDoodleCreateCharacterProxy* GetCreateCharacterProxyOnGameThread() {
    return &GetProxyOnGameThread<FDoodleCreateCharacterProxy>();
  }

 protected:
  virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

 private:
  friend FDoodleCreateCharacterProxy;

  // Cache for GC
  UPROPERTY(transient)
  TObjectPtr<UDoodleCreateCharacterConfig> CurrentConfig;
};