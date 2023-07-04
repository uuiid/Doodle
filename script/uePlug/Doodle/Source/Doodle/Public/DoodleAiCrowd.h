// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 这个必须最后导入
#include "DoodleAiCrowd.generated.h"

class USplineComponent;
class UAnimationAsset;
class UDoodleAiMoveToComponent;

UCLASS()
class DOODLE_API ADoodleAiCrowd : public ACharacter {
  GENERATED_BODY()

 public:
  // Sets default values for this character's properties
  ADoodleAiCrowd();

  inline UDoodleAiMoveToComponent* GetDoodleMoveToComponent() {
    return MoveToCom;
  }

 protected:
  UPROPERTY()
  UDoodleAiMoveToComponent* MoveToCom{};
  // Called when the game starts or when spawned
  // virtual void BeginPlay() override;

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
