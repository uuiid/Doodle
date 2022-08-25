// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// 我自己定义的运行时曲线需要的头文件
#include "Curves/CurveFloat.h"

// 这个必须在最后面
#include "fireLight.generated.h"
class ULocalLightComponent;

UCLASS(ClassGroup = Lights, hideCategories = (Input, Collision, Replication), showCategories = ("Input|MouseInput", "Input|TouchInput"), meta = (ChildCanTick))
class DOODLE_API AfireLight : public AActor {
  GENERATED_BODY()

 public:
  // Sets default values for this actor's properties
  AfireLight();

  UPROPERTY(EditAnyWhere, BlueprintReadWrite, Interp, Category = "Doodle_light", meta = (UIMin = "0", UIMax = "100"))
  float Speed = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Doodle_Light", meta = (UIMin = "0"))
  float luminanceMax = 75.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Doodle_Light", meta = (UIMin = "0"))
  float luminanceMin = 5.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Doodle_Light", DisplayName = "LightCurve")
  FRuntimeFloatCurve p_LocalLightCurve;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Interp, Category = "Doodle_Light", DisplayName = "PointLight")
  ULocalLightComponent* p_LocalLight;

  UFUNCTION(BlueprintCallable, meta = (CallInEditor = "true", OverrideNativeName = "切换", Category = "Doodle_Light", Tooltip = "切换点光源和聚光灯"))
  virtual void SearchLight();

  // 覆盖用来在编辑器中运行
  virtual bool ShouldTickIfViewportsOnly() const override;

 protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
