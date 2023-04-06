// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 这个必须最后导入
#include "DoodleGenerateCluster.generated.h"

class USplineComponent;
class UAnimationAsset;

UCLASS()
class DOODLE_API ADoodleGenerateCluster : public AActor {
  GENERATED_BODY()

 public:
  // Sets default values for this character's properties
  ADoodleGenerateCluster();

 protected:
  // Called when the game starts or when spawned
  //virtual void BeginPlay() override;

 public:
  // Called every frame
  //virtual void Tick(float DeltaTime) override;
};
