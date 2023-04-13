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
      meta = (ClampMin = 0.0, ClampMax = 200)
  )
  float RandomRadius;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "启用集群", meta = (ClampMin = 1, ClampMax = 100)
  )
  bool bCluster;
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "集群点", meta = (ClampMin = 1, ClampMax = 100)
  )
  int32 ClusterPointNum;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "迭代次数", meta = (ClampMin = 1, ClampMax = 100)
  )
  int32 ClusterIter;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机方向范围", meta = (ClampMin = -1.0, ClampMax = 1.0))
  FVector2D RandomOrient;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle)
  TArray<TObjectPtr<UAnimationAsset>> AnimAssets;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle)
  TArray<TObjectPtr<USkeletalMesh>> SkinAssets;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle)
  TObjectPtr<UStaticMesh> Preview_Mesh;

  UPROPERTY()
  TObjectPtr<UInstancedStaticMeshComponent> Preview_InstancedStaticMeshComponent;

  /**
   * Channels that this light should affect.
   * These channels only apply to opaque materials, direct lighting, and dynamic lighting and shadowing.
   */
  UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = Light)
  FLightingChannels LightingChannels;

  virtual void Tick(float DeltaTime) override;
  virtual void PostActorCreated() override;
  virtual bool ShouldTickIfViewportsOnly() const override;
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

  FVector Target_Transform;

  void GenPoint();
  FQuat GetRandomOrient(const FVector &In_Origin);
  FQuat GetRandomOrient(const FVector &In_Origin, const FVector &In_Look);

  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);

  virtual void OnConstruction(const FTransform &Transform) override;
  void K_Means_Clustering();
};