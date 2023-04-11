// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DoodleAiCrowd.h"

// 这个必须最后导入
#include "DoodleAiSplineCrowd.generated.h"

class USplineComponent;
class UAnimationAsset;

class UDoodleAiSplineMoveToComponent;
UCLASS()
class DOODLE_API ADoodleAiSplineCrowd : public ADoodleAiCrowd {
  GENERATED_BODY()

 public:
  // Sets default values for this character's properties
  ADoodleAiSplineCrowd();

  UPROPERTY(Category = Doodle, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UDoodleAiSplineMoveToComponent> SplineMoveToComponent;

  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

  FTransform TranRoot{};

 protected:
  // Called when the game starts or when spawned
  // virtual void BeginPlay() override;

 public:
  // Called every frame
  // virtual void Tick(float DeltaTime) override;
};
