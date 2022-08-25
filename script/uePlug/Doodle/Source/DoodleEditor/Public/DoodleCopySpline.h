// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DoodleCopySpline.generated.h"

class USplineComponent;
class UInstancedStaticMeshComponent;

UCLASS()
class DOODLEEDITOR_API ADoodleCopySpline : public AActor {
  GENERATED_BODY()

 public:
  // Sets default values for this actor's properties
  ADoodleCopySpline();

  UInstancedStaticMeshComponent* p_preview;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "预览网格体")
  UStaticMesh* p_preview_mesh;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "复制目标")
  AActor* p_actor;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "朝向目标")
  AActor* p_lock_at_actor;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "number", meta = (UIMin = "0"))
  uint8 p_number;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "曲线")
  USplineComponent* p_spline;

  UFUNCTION(BlueprintCallable, meta = (CallInEditor = "true", OverrideNativeName = "特殊复制", Category = "Doodle", Tooltip = "沿曲线复制点"))
  virtual void CopyAActore();

#if WITH_EDITOR
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangeEvent) override;
#endif  // WITH_EDITOR

  // virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangeEvent) override;
  // virtual void PostInterpChange(FProperty* PropertyThatChanged) override;

 protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
