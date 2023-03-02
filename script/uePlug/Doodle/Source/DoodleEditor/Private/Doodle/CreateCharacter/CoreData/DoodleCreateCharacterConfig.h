#pragma once

#include "CoreMinimal.h"
#include "DoodleCreateCharacterConfig.generated.h"

USTRUCT()
struct FDoodleCreateCharacterConfigNode {
  GENERATED_BODY();

  UPROPERTY();
  FTransformCurve WeightCurve{};

  UPROPERTY();
  FName BoneName{};
  UPROPERTY();
  FName ShowUIName{};
};

UCLASS()
class UDoodleCreateCharacterConfig : public UObject {
 public:
  GENERATED_BODY()

  UPROPERTY();
  TArray<FDoodleCreateCharacterConfigNode> ListConfigNode;

  FTransform Evaluate(const FName& In_BoneName, const float InValue) const;
};