#pragma once

#include "CoreMinimal.h"
#include "DoodleCreateCharacterConfig.generated.h"

USTRUCT()
struct FDoodleCreateCharacterConfigNode {
  GENERATED_BODY();

  // 权重曲线
  UPROPERTY();
  FTransformCurve WeightCurve{};

  // 目标骨骼名称
  UPROPERTY();
  FName BoneName{};
  // 显示ui名称
  UPROPERTY();
  FString ShowUIName{};

  // 调整最大值
  UPROPERTY()
  float MaxValue;
  // 调整最小值
  UPROPERTY()
  float MInValue;
};

UCLASS()
class UDoodleCreateCharacterConfig : public UObject {
 public:
  GENERATED_BODY()

  // 权重配置
  UPROPERTY();
  TArray<FDoodleCreateCharacterConfigNode> ListConfigNode;

  FTransform Evaluate(const FName& In_BoneName, const float InValue) const;

  USkeletalMesh* GetSkeletalMesh() {
    return SkeletalMesh;
  }

  void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh) {
    SkeletalMesh = InSkeletalMesh;
  }

 private:
  // 骨骼网格体引用
  UPROPERTY();
  TObjectPtr<USkeletalMesh> SkeletalMesh;
};