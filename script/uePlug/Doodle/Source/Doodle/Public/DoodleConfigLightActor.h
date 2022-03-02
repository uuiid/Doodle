// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// clang-format off
#include "DoodleConfigLightActor.generated.h"
// clang-format on

class ASkeletalMeshActor;
class ALight;
class UDoodleConfigLight;

UCLASS()
class DOODLE_API ADoodleConfigLightActor : public AActor {
  GENERATED_BODY()

#if WITH_EDITOR
  bool OpenSaveDialog(const FString& InDefaultPath,
                      const FString& InNewNameSuggestion,
                      FString& OutPackageName);
  UObject* OpenDialog(const FString& InDefaultPath,
                  const FString& InNewNameSuggestion);
#endif  // WITH_EDITOR
 public:
  ADoodleConfigLightActor();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle",
            DisplayName = "骨骼物体")
  ASkeletalMeshActor* p_skin_mesh;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle",
            DisplayName = "骨骼插槽名称")
  FName p_solt;

  UFUNCTION(BlueprintCallable,
            meta = (CallInEditor = "true", OverrideNativeName = "保存",
                    Category = "Doodle", Tooltip = "保存灯光预设"))
  virtual void SaveConfig();

  UFUNCTION(BlueprintCallable,
            meta = (CallInEditor = "true", OverrideNativeName = "加载",
                    Category = "Doodle", Tooltip = "加载灯光预设"))
  virtual void LoadConfig();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle",
            DisplayName = "重新附加时清除")
  bool use_clear;


  UPROPERTY(EditAnywhere, Category = "Doodle", DisplayName = "灯光组")
  TArray<TWeakObjectPtr<ALight>> p_light_list;
  // TWeakObjectPtr<ALight> _p;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent& PropertyChangeEvent) override;
#endif  // WITH_EDITOR
};
