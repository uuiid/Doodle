// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoodleLightWeight.h"

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

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle",
            DisplayName = "灯光组")
  TArray<FDoodleLightWeight> p_light;
};
