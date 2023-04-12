#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"

// 这个必须最后导入
#include "AiArrayGeneration.generated.h"

class USplineComponent;
class UArrowComponent;

UCLASS()
class DOODLE_API ADoodleAiArrayGeneration : public AActor {
  GENERATED_BODY()
 public:
  ADoodleAiArrayGeneration();

 private:
  UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
  TObjectPtr<class USplineComponent> SplineComponent;

 public:
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "行", meta = (ClampMin = 1, ClampMax = 1000)
  )
  int32 Row;
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "列", meta = (ClampMin = 1, ClampMax = 1000)
  )
  int32 Column;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机范围",
      meta = (ClampMin = 0.0, ClampMax = 1000)
  )
  float RandomRadius;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle
  )
  TArray<TObjectPtr<UAnimationAsset>> AnimAssets;
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle
  )
  TArray<TObjectPtr<USkeletalMesh>> SkinAssets;

  virtual void Tick(float DeltaTime) override;
  virtual void PostActorCreated() override;

  virtual void BeginPlay() override;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent &PropertyChangeEvent
  ) override;
#endif  // WITH_EDITOR

 private:
  UPROPERTY()
  FRandomStream RandomStream_Orient;
  UPROPERTY()
  FRandomStream RandomStream_Anim;
  UPROPERTY()
  FRandomStream RandomStream_Skin;
  UPROPERTY()
  FRandomStream RandomStream_Anim_Rate;
  UPROPERTY()
  TArray<FTransform> Points;

  UPROPERTY()
  TArray<TObjectPtr<UArrowComponent>> ArrowComponents;

  UPROPERTY(EditDefaultsOnly)
  TObjectPtr<UStaticMeshComponent> Target;
  /** Component to control Pitch. */
  UPROPERTY(EditDefaultsOnly, Category = "Crane Components")
  TObjectPtr<USceneComponent> SceneComponentTarget;

  UPROPERTY()
  TObjectPtr<UInstancedStaticMeshComponent> Preview_InstancedStaticMeshComponent;

  void GenPoint();
  FQuat GetRandomOrient(const FVector &In_Origin);

  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);

  virtual void OnConstruction(const FTransform &Transform) override;
};