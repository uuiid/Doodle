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
  // virtual void BeginPlay() override;

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;

  /** 这个是看向或者走向的物体 */
  UPROPERTY(EditAnywhere, Category = "Doodle")
  AActor* DoodleLockAtObject;

  /** 这个是看向(false)或者走向(true)的物体 */
  UPROPERTY(EditAnywhere, Category = "Doodle")
  bool MoveTo{};

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "Org")
  FVector Direction;

 private:
  FVector p_Point;
};
