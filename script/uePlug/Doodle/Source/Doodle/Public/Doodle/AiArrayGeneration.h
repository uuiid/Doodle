#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
// 这个必须最后导入
#include "AiArrayGeneration.generated.h"

class USplineComponent;
class UArrowComponent;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class DOODLE_API ADoodleAiArrayGeneration : public AActor {
  GENERATED_BODY()
 public:
  ADoodleAiArrayGeneration();

 private:
  UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
  USplineComponent *SplineComponent;

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
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机范围", meta = (ClampMin = 0.0, Max = 200)
  )
  float RandomRadius;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "上下偏移值")
  float OffsetValue;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "启用集群",
      meta = (ClampMin = 1, ClampMax = 100)
  )
  bool bCluster;
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "集群点", meta = (ClampMin = 1, ClampMax = 100)
  )
  int32 ClusterPointNum;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "迭代次数",
      meta = (ClampMin = 1, ClampMax = 100)
  )
  int32 ClusterIter;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机方向范围",
      meta = (ClampMin = -1.0, ClampMax = 1.0)
  )
  FVector2D RandomOrient;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机动画速率")
  FVector2D RandomAnimSpeed;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle)
  TArray<UAnimationAsset *> AnimAssets;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle)
  TArray<USkeletalMesh *> SkinAssets;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle)
  UStaticMesh *Preview_Mesh;

  UPROPERTY()
  UInstancedStaticMeshComponent *Preview_InstancedStaticMeshComponent;

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

 private:
  UPROPERTY()
  FRandomStream RandomStream_Orient;
  UPROPERTY()
  FRandomStream RandomStream_Anim;
  UPROPERTY()
  FRandomStream RandomStream_Skin;

  UPROPERTY()
  TArray<FTransform> Points;

  UPROPERTY()
  TArray<UArrowComponent *> ArrowComponents;

  UPROPERTY(EditDefaultsOnly, Category = "Doodle")
  UStaticMeshComponent *Target;
  /** Component to control Pitch. */
  UPROPERTY(EditDefaultsOnly, Category = "Doodle")
  USceneComponent *SceneComponentTarget;

  FVector Target_Transform;

  void GenPoint();
  FQuat GetRandomOrient(const FVector &In_Origin);
  FQuat GetRandomOrient(const FVector &In_Origin, const FVector &In_Look);

  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);

  virtual void OnConstruction(const FTransform &Transform) override;
};