#pragma once

#include "CoreMinimal.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// 这个必须在最后面
#include "DoodleMatrixLight.generated.h"

class USpotLightComponent;
class USceneComponent;
class UArrowComponent;
class USplineComponent;

UCLASS()
class DOODLE_API ADoodleMatrixLight : public AActor {
  GENERATED_BODY()
 public:
  ADoodleMatrixLight();
  UPROPERTY(BlueprintReadOnly, interp, Category = Light)
  bool Enabled{true};

  /**
   * Total energy that the light emits.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "Intensity", UIMin = "0.0", UIMax = "2000000.0"))
  float Intensity{200};

  /**
   * Filter color of the light.
   * Note that this can change the light's effective intensity.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (HideAlphaChannel))
  FColor LightColor{FColor::White};

  /**
   *
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "FocalAngleOuter"))
  float FocalAngleOuter{15.0};

  /**
   *
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "FocalAngleInner"))
  float FocalAngleInner{1.0};

  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "AttenuationDistance"))
  float AttenuationDistance{1000.0};

  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "LightWidth"))
  float LightWidth{20.0};
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "LightLength"))
  float LightLength{20.0};
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "CastShadows"))
  bool CastShadows{true};

  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "LightSamplesSquared", UIMin = "1", UIMax = "8"))
  int LightSamplesSquared{3};
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "LightSamplesQueue", UIMin = "1", UIMax = "16"))
  int LightSamplesQueue{8};

  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "SourceRadiusMult"))
  float SourceRadiusMult{1.0};
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "CenterOfInterestLength"))
  float CenterOfInterestLength{500.0};

  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "Channels"))
  FLightingChannels Channels;
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "SoftRadius"))
  float SoftRadius{0};
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "ShadowBias"))
  float ShadowBias{0.025};

  // UFUNCTION(BlueprintCallable,
  // 		  meta = (CallInEditor = "true", OverrideNativeName = "测试"))
  // void TEST();

  UPROPERTY(EditAnywhere, Category = Light)
  TArray<USpotLightComponent *> LightList_;
  UPROPERTY(EditAnywhere, Category = Light)
  TArray<UArrowComponent *> ArrowList_;

  UPROPERTY(EditAnywhere, Category = Light)
  TArray<USceneComponent *> SceneComponentList_;

  UPROPERTY(EditAnywhere, Category = Light)
  USplineComponent *SplineComponen_;

  void OnConstruction(const FTransform &Transform) override;
  void PostActorCreated() override;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent &PropertyChangeEvent
  ) override;
#endif  // WITH_EDITOR

 private:
  void CreateLightSqueue();

  void CreateLightSquare(int InSceneComponentIndex);

  void SetLightAttr();
};
