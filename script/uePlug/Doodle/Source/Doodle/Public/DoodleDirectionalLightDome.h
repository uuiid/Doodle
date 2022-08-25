// Fill out your copyright notice in the Description page of Project
// Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// 我自己定义的运行时曲线需要的头文件
#include "Curves/CurveFloat.h"

// 这个必须在最后面
#include "DoodleDirectionalLightDome.generated.h"
class UDirectionalLightComponent;
UCLASS()
class DOODLE_API ADoodleDirectionalLightDome : public AActor {
  GENERATED_BODY()

 public:
  // Sets default values for this actor's properties
  ADoodleDirectionalLightDome();
  // 经度曲线  竖线
  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "longitudeCurve")
  FRuntimeFloatCurve p_longitude;

  // 纬度曲线  横线
  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "latitudeCurve", meta = (UIMin = "0"))
  FRuntimeFloatCurve p_latitude;

  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "light max", meta = (UIMin = "0"))
  float p_light_max;

  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "light min", meta = (UIMin = "0"))
  float p_light_min;

  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "night", meta = (UIMin = "0"))
  float p_night;

  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "specular min", meta = (UIMin = "0"))
  float p_specular_min;

  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "投射阴影")
  bool p_castShadow;

  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "阴影量", meta = (UIMin = "0", UIMax = "1"))
  float p_shadowAmout;

  // 镜像强度映射
  UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "light to specular", meta = (UIMin = "0"))
  FRuntimeFloatCurve p_specular_curve;

  /**
   * Angle subtended by light source in degrees (also known as angular
   * diameter). Defaults to 0.5357 which is the angle for our sun.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Interp, Category = Light, meta = (UIMin = "0", UIMax = "5"), DisplayName = "Source Angle")
  float LightSourceAngle;

  /**
   * Angle subtended by soft light source in degrees.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Interp, Category = Light, meta = (UIMin = "0", UIMax = "5"), DisplayName = "Source Soft Angle")
  float LightSourceSoftAngle;

  /**
   * Channels that this light should affect.
   * These channels only apply to opaque materials, direct lighting, and dynamic
   * lighting and shadowing.
   */
  UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = Light)
  FLightingChannels LightingChannels;

  /**
   * Filter color of the light.
   * Note that this can change the light's effective intensity.
   */
  UPROPERTY(BlueprintReadOnly, interp, Category = Light, meta = (HideAlphaChannel))
  FColor LightColor;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle_Light", DisplayName = "lightArray")
  TArray<UDirectionalLightComponent *> p_array_light;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent &PropertyChangeEvent
  ) override;
#endif  // WITH_EDITOR

 protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

 private:
  void set_light();

 public:
  // Called every frame
  // virtual void Tick(float DeltaTime) override;
};
