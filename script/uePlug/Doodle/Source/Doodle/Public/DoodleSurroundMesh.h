#pragma once

#include "CoreMinimal.h"
#include "DoodleLightWeight.h"
#include "GameFramework/Actor.h"

// clang-format off
#include "DoodleSurroundMesh.generated.h"
// clang-format on

class UStaticMesh;
class USplineComponent;
class UInstancedStaticMeshComponent;

UCLASS(meta = (ChildCanTick))
class DOODLE_API ADoodleSurroundMeshActor : public AActor {
  GENERATED_BODY()
 public:
  ADoodleSurroundMeshActor();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "网格体")
  UStaticMesh *p_mesh;

  UPROPERTY()
  UInstancedStaticMeshComponent *p_instanced;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "曲线")
  USplineComponent *p_spline;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "个数")
  int32 p_count;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "速度")
  float p_vector = 100.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Doodle")
  float vectorMax = 20.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Doodle")
  float vectorMin = -20.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "半径")
  float radius = 80.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "径向速度")
  float p_radius_vector = 0.7f;

  // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "旋转速度")
  // float rot_size = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "大小")
  float size = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "大小下限")
  float size_min = 0.8f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "大小上限")
  float size_max = 1.2f;

  UPROPERTY()
  TArray<float> p_lens;

  UPROPERTY()
  TArray<float> p_vector_list;

  UPROPERTY()
  TArray<FVector> p_radius_list;

  UPROPERTY()
  TArray<FVector> p_size_list;

#if WITH_EDITOR
  virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangeEvent) override;
#endif  // WITH_EDITOR

  virtual void PostLoad() override;
  virtual void OnConstruction(
      const FTransform &Transform
  ) override;

  // 覆盖用来在编辑器中运行
  virtual bool ShouldTickIfViewportsOnly() const override;

 protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  void SetSurroundMesh();
  void SetVector();
  void SetRadius();
  void SetSize();

 public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
