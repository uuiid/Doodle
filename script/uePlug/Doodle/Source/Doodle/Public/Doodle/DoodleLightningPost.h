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

 private:
  UPROPERTY(VisibleAnywhere, Category = "Light")
  TObjectPtr<UDirectionalLightComponent> LightComponent;
};