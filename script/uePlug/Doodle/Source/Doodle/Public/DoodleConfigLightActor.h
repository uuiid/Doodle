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
  bool OpenSaveDialog(const FString& InDefaultPath,
                      const FString& InNewNameSuggestion,
                      FString& OutPackageName);
  UObject* OpenDialog(const FString& InDefaultPath,
                      const FString& InNewNameSuggestion);
#endif  // WITH_EDITOR
 public:
  ADoodleConfigLightActor();

  UFUNCTION(BlueprintCallable,
            meta = (CallInEditor = "true", OverrideNativeName = "保存",
                    Category = "Doodle", Tooltip = "保存灯光预设"))
  virtual void SaveConfig();

  UFUNCTION(BlueprintCallable,
            meta = (CallInEditor = "true", OverrideNativeName = "加载",
                    Category = "Doodle", Tooltip = "加载灯光预设"))
  virtual void LoadConfig();

  UPROPERTY(EditAnywhere, Category = "Doodle", DisplayName = "灯光组")
  TArray<FDoodleLightWeight> p_light_list;

  /// 这个底下是提升的参数, 复制来源是 ULightComponentBase
  /**
   * Total energy that the light emits.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light,
            meta = (DisplayName = "Intensity", UIMin = "0.0", UIMax = "20.0"))
  float Intensity;

  /**
   * Filter color of the light.
   * Note that this can change the light's effective intensity.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light,
            meta = (HideAlphaChannel))
  FColor LightColor;

  /**
   * Color temperature in Kelvin of the blackbody illuminant.
   * White (D65) is 6500K.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light,
            meta = (UIMin = "1700.0", UIMax = "12000.0"))
  float Temperature;

  /** false: use white (D65) as illuminant. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light,
            meta = (DisplayName = "Use Temperature"))
  uint32 bUseTemperature : 1;

  /**
   * Angle subtended by light source in degrees (also known as angular
   * diameter). Defaults to 0.5357 which is the angle for our sun.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light,
            meta = (UIMin = "0", UIMax = "5"), DisplayName = "Source Angle")
  float LightSourceAngle;

  /**
   * Angle subtended by soft light source in degrees.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light,
            meta = (UIMin = "0", UIMax = "5"),
            DisplayName = "Source Soft Angle")
  float LightSourceSoftAngle;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light)
  uint32 bAffectsWorld : 1;

  /**
   * Whether the light should cast any shadows.
   **/
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light)
  uint32 CastShadows : 1;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent& PropertyChangeEvent) override;
#endif  // WITH_EDITOR
};
