// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//我自己定义的运行时曲线需要的头文件
#include "Curves/CurveFloat.h"

//这个必须在最后面
#include "fireLight.generated.h"
class ULocalLightComponent;

UCLASS(ClassGroup = Lights, hideCategories = (Input, Collision, Replication), showCategories = ("Input|MouseInput", "Input|TouchInput"), meta = (ChildCanTick))
class DOODLE_API AfireLight : public AActor {
  GENERATED_BODY()

 public:
  // Sets default values for this actor's properties
  AfireLight();

  UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Doodle_light")
  float Speed = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doodle_Light")
  float luminanceMax = 75.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doodle_Light")
  float luminanceMin = 5.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doodle_Light", DisplayName = "LightCurve")
  FRuntimeFloatCurve p_LocalLightCurve;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doodle_Light", DisplayName = "PointLight")
  ULocalLightComponent* p_LocalLight;

  virtual bool ShouldTickIfViewportsOnly() const override;

 protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
