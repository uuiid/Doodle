#pragma once

#include "CoreMinimal.h"
#include "DoodleLightWeight.generated.h"

class ALight;
class ULightComponent;

USTRUCT(BlueprintType)
struct FDoodleLightWeight {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, Category = FDoodleLightWeight, DisplayName = "灯光")
  ULightComponent* light;
  UPROPERTY(EditAnywhere, Category = FDoodleLightWeight, DisplayName = "权重", meta = (ClampMin = 0, ClampMax = 10))
  float weight;

  FDoodleLightWeight();

  FDoodleLightWeight(ULightComponent* in_light, float in_weight);
};
