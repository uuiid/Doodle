#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
// 这个必须最后导入
#include "DoodleLightningPost.generated.h"

class UDirectionalLightComponent;

UCLASS()
class DOODLE_API ADoodleLightingPost : public APostProcessVolume {
  GENERATED_BODY()
 public:
  ADoodleLightingPost();

  TObjectPtr<UDirectionalLightComponent> GetLightComponent() const { return LightComponent; }
  // 强度乘数
  UPROPERTY(EditAnywhere, Category = Doodle, meta = (DisplayName = "强度乘数", ShouldShowInViewport = true))
  float IntensityMultiplier;
  // 饱和度乘数
  UPROPERTY(EditAnywhere, Category = Doodle, meta = (DisplayName = "饱和度乘数", ShouldShowInViewport = true))
  float SaturationMultiplier;
  // 对比度乘数
  UPROPERTY(EditAnywhere, Category = Doodle, meta = (DisplayName = "对比度乘数", ShouldShowInViewport = true))
  float ContrastMultiplier;

  UPROPERTY(EditAnywhere, Category = Doodle)
  TObjectPtr<UCurveFloat> IntensityCurve;

 private:
  UPROPERTY(VisibleAnywhere, Category = "Light")
  TObjectPtr<UDirectionalLightComponent> LightComponent;
};