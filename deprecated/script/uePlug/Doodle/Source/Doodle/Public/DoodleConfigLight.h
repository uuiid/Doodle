// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DoodleLightWeight.h"
#include "GameFramework/Actor.h"

// clang-format off
#include "DoodleConfigLight.generated.h"
// clang-format on

class ASkeletalMeshActor;
class ULightComponent;
class ALight;

UCLASS(EditInlineNew)
class DOODLE_API UDoodleConfigLight : public UObject {
  GENERATED_BODY()
 public:
  UDoodleConfigLight();

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "actor")
  AActor* p_Actor;
};
