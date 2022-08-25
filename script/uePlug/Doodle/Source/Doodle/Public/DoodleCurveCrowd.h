// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 这个必须最后导入
#include "DoodleCurveCrowd.generated.h"

class USplineComponent;
class UAnimationAsset;

UCLASS()
class DOODLE_API ADoodleCurveCrowd : public ACharacter {
  GENERATED_BODY()

 public:
  // Sets default values for this character's properties
  ADoodleCurveCrowd();

 protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;

  // Called to bind functionality to input
  virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "Anim")
  UAnimationAsset *AnimationAsset;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "Org")
  FVector Direction;

 private:
  FVector p_Point;
};
