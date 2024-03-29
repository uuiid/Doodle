#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

// 这个必须最后导入
#include "AiArrayGenerationMoveSpline.generated.h"

class USplineComponent;
class UArrowComponent;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class DOODLE_API ADoodleAiArrayGenerationMoveSpline : public AActor {
  GENERATED_BODY()
 public:
  ADoodleAiArrayGenerationMoveSpline();

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
      EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机范围", meta = (ClampMin = 0.0, Max = 200)
  )
  float RandomRadius;

  UPROPERTY(EditAnywhere, Category = "Doodle", DisplayName = "接收贴花")
  bool bReceivesDecals;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "生成点上下偏移值")
  float OffsetValue;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "网格体上下偏移值")
  float OffsetValueMesh;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "旋转偏移值")
  float SkinOffsetQuatValue;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Interp, Category = Doodle, DisplayName = "随机最大速度")
  FVector2D RandomAnimSpeed;

  UPROPERTY(Category = Doodle, EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
  float MaxAcceleration;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "随机寻址范围")
  float RandomRadius_Move{500};

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
  TArray<TObjectPtr<UArrowComponent>> ArrowComponents;

  UPROPERTY(EditDefaultsOnly, Category = "Doodle")
  TObjectPtr<USplineComponent> TargetSpline;

  FVector Target_Transform;

  TArray<TObjectPtr<AActor>> MoveActors;

  void GenPoint();

  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);

  virtual void OnConstruction(const FTransform &Transform) override;
};