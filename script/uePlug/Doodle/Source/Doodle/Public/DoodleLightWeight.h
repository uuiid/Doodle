#pragma once

#include "CoreMinimal.h"
#include "DoodleLightWeight.generated.h"

class ALight;

USTRUCT(BlueprintType)
struct FDoodleLightWeightWeak {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, Category = FDoodleLightWeight, DisplayName = "灯光")
  TWeakObjectPtr<ALight> light;
  UPROPERTY(EditAnywhere, Category = FDoodleLightWeight, DisplayName = "权重",
            meta = (ClampMin = 0, ClampMax = 10))
  float weight;

  FDoodleLightWeightWeak();
  FDoodleLightWeightWeak(ALight* in_light, float in_weight);

  bool operator==(const FDoodleLightWeightWeak& in_l) const;
  bool operator!=(const FDoodleLightWeightWeak& in_l) const;
};

USTRUCT(BlueprintType)
struct FDoodleLightWeight {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, Category = FDoodleLightWeight, DisplayName = "灯光")
  ALight* light;
  UPROPERTY(EditAnywhere, Category = FDoodleLightWeight, DisplayName = "权重",
            meta = (ClampMin = 0, ClampMax = 10))
  float weight;

  FDoodleLightWeight();

  FDoodleLightWeight(ALight* in_light, float in_weight);
};
