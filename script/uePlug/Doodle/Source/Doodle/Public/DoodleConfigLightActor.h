// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DoodleLightWeight.h"
#include "GameFramework/Actor.h"

// clang-format off
#include "DoodleConfigLightActor.generated.h"
// clang-format on

class ASkeletalMeshActor;
class ALight;
class UDoodleConfigLight;
class ULightComponent;

UCLASS()
class DOODLE_API ADoodleConfigLightActor : public AActor {
  GENERATED_BODY()
 public:
#if WITH_EDITOR
  bool OpenSaveDialog(const FString& InDefaultPath, const FString& InNewNameSuggestion, FString& OutPackageName);
  UObject* OpenDialog(const FString& InDefaultPath, const FString& InNewNameSuggestion);
#endif  // WITH_EDITOR
 public:
  ADoodleConfigLightActor();

  UFUNCTION(BlueprintCallable, meta = (CallInEditor = "true", OverrideNativeName = "保存", Category = "Doodle", Tooltip = "保存灯光预设"))
  virtual void SaveConfig();

  virtual void LoadConfig();

  UPROPERTY(EditAnywhere, Category = "Doodle", DisplayName = "灯光权重")
  TArray<float> p_light_list;

  /// 这个底下是提升的参数, 复制来源是 ULightComponentBase
  /**
   * Total energy that the light emits.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (DisplayName = "Intensity", UIMin = "0.0", UIMax = "20.0"))
  float Intensity;

  /**
   * Filter color of the light.
   * Note that this can change the light's effective intensity.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (HideAlphaChannel))
  FColor LightColor;

  /**
   * Color temperature in Kelvin of the blackbody illuminant.
   * White (D65) is 6500K.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light, meta = (UIMin = "1700.0", UIMax = "12000.0"))
  float Temperature;

  /** false: use white (D65) as illuminant. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, meta = (DisplayName = "Use Temperature"))
  uint32 bUseTemperature : 1;

  /**
   * Angle subtended by light source in degrees (also known as angular
   * diameter). Defaults to 0.5357 which is the angle for our sun.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light, meta = (UIMin = "0", UIMax = "5"), DisplayName = "Source Angle")
  float LightSourceAngle;

  /**
   * Angle subtended by soft light source in degrees.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light, meta = (UIMin = "0", UIMax = "5"), DisplayName = "Source Soft Angle")
  float LightSourceSoftAngle;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light)
  uint32 bAffectsWorld : 1;

  /**
   * Whether the light should cast any shadows.
   **/
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light)
  uint32 CastShadows : 1;

  /**
   * Channels that this light should affect.
   * These channels only apply to opaque materials, direct lighting, and dynamic
   * lighting and shadowing.
   */
  UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = Light)
  FLightingChannels LightingChannels;

  /** Whether the light affects translucency or not.  Disabling this can save
   * GPU time when there are many small lights. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, AdvancedDisplay)
  uint32 bAffectTranslucentLighting : 1;

  /**
   * How far Cascaded Shadow Map dynamic shadows will cover for a movable light,
   * measured from the camera. A value of 0 disables the dynamic shadow.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "20000", DisplayName = "Dynamic Shadow Distance MovableLight"))
  float DynamicShadowDistanceMovableLight;

  /**
   * How far Cascaded Shadow Map dynamic shadows will cover for a stationary
   * light, measured from the camera. A value of 0 disables the dynamic shadow.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "20000", DisplayName = "Dynamic Shadow Distance StationaryLight"))
  float DynamicShadowDistanceStationaryLight;

  /**
   * Number of cascades to split the view frustum into for the whole scene
   * dynamic shadow. More cascades result in better shadow resolution, but adds
   * significant rendering cost.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "4", DisplayName = "Num Dynamic Shadow Cascades"))
  int32 DynamicShadowCascades;

  /**
   * Controls whether the cascades are distributed closer to the camera (larger
   * exponent) or further from the camera (smaller exponent). An exponent of 1
   * means that cascade transitions will happen at a distance proportional to
   * their resolution.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "1", UIMax = "4", DisplayName = "Distribution Exponent"))
  float CascadeDistributionExponent;

  /**
   * Proportion of the fade region between cascades.
   * Pixels within the fade region of two cascades have their shadows blended to
   * avoid hard transitions between quality levels. A value of zero eliminates
   * the fade region, creating hard transitions. Higher values increase the size
   * of the fade region, creating a more gradual transition between cascades.
   * The value is expressed as a percentage proportion (i.e. 0.1 = 10% overlap).
   * Ideal values are the smallest possible which still hide the transition.
   * An increased fade region size causes an increase in shadow rendering cost.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "0.3", DisplayName = "Transition Fraction"))
  float CascadeTransitionFraction;

  /**
   * Controls the size of the fade out region at the far extent of the dynamic
   * shadow's influence. This is specified as a fraction of
   * DynamicShadowDistance.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "1.0", DisplayName = "Distance Fadeout Fraction"))
  float ShadowDistanceFadeoutFraction;

  /**
   * Stationary lights only: Whether to use per-object inset shadows for movable
   * components, even though cascaded shadow maps are enabled. This allows
   * dynamic objects to have a shadow even when they are outside of the cascaded
   * shadow map, which is important when DynamicShadowDistanceStationaryLight is
   * small. If DynamicShadowDistanceStationaryLight is large (currently > 8000),
   * this will be forced off. Disabling this can reduce shadowing cost
   * significantly with many movable objects.
   */
  UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = CascadedShadowMaps, DisplayName = "Inset Shadows For Movable Objects")
  uint32 bUseInsetShadowsForMovableObjects : 1;

  /** 0: no DistantShadowCascades, otherwise the count of cascades between
   * WholeSceneDynamicShadowRadius and DistantShadowDistance that are covered by
   * distant shadow cascades. */
  UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "4"), DisplayName = "Far Shadow Cascade Count")
  int32 FarShadowCascadeCount;

  /**
   * Distance at which the far shadow cascade should end.  Far shadows will
   * cover the range between 'Dynamic Shadow Distance' and this distance.
   */
  UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = CascadedShadowMaps, meta = (UIMin = "0", UIMax = "800000"), DisplayName = "Far Shadow Distance")
  float FarShadowDistance;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent& PropertyChangeEvent
  ) override;
#endif  // WITH_EDITOR
};
